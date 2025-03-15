#include "Sprinter.h"

#include <iostream>

namespace physics {

	Sprinter::Sprinter(std::unique_ptr<SprinterCreationSettings> sprinterCreationSettings, std::shared_ptr<JPH::PhysicsSystem> physics_system) {
		this->sprinterSettings = std::move(sprinterCreationSettings->sprinterSettings);
		this->characterSettings = std::move(sprinterCreationSettings->characterSettings);
		this->physics_system = physics_system;
		this->character = std::unique_ptr<JPH::Character>(new JPH::Character(this->characterSettings.get(), sprinterCreationSettings->position, JPH::Quat::sIdentity(), sprinterCreationSettings->inUserData, this->physics_system.get()));

		this->currentHealth = this->sprinterSettings->maxHealth;
	}

	Sprinter::~Sprinter() {
		removePhysicsBody();
	}

	// doesn't move if the enemy doesn't approximately face the player
	void Sprinter::update() {

		// TODO deltaTime is handled by the physics system ?

		float targetAngle = calculateTargetAngle();
		float currentHorizontalAngle = this->character->GetRotation().GetRotationAngle({0,1,0});

		// std::cout << "Enemy [" << this->id << "] currentAngle=" << currentHorizontalAngle
		//	 	<< " targetAngle=" << targetAngle
		//		<< " rotationSpeed=" << this->sprinterSettings->rotationSpeed
		//	 	<< " movementAngle=" << this->sprinterSettings->movementAngle
		//		<< std::endl;

		// Normalize angles to [0, 2*pi]
		while (targetAngle > 2 * glm::pi<float>()) targetAngle -= 2 * glm::pi<float>();
		while (targetAngle < 0) targetAngle += 2 * glm::pi<float>();
		while (currentHorizontalAngle > 2 * glm::pi<float>()) currentHorizontalAngle -= 2 * glm::pi<float>();
		while (currentHorizontalAngle < 0) currentHorizontalAngle += 2 * glm::pi<float>();

		float angleDiff = targetAngle - currentHorizontalAngle;

		// Find shortest angleDiff [-pi,pi]
		while (angleDiff > glm::pi<float>()) angleDiff -= 2.0f * glm::pi<float>();
		while (angleDiff < -glm::pi<float>()) angleDiff += 2.0f * glm::pi<float>();

		// std::cout << "  normalized angles: current=" << currentHorizontalAngle
		// 		<< " target=" << targetAngle
		// 		<< " diff=" << angleDiff
		// 		<< std::endl;

		bool isLockedOnPlayer = std::fabs(angleDiff) <= this->sprinterSettings->movementAngle;

		// std::cout << "  isLockedOnPlayer=" << (isLockedOnPlayer ? "true" : "false") << std::endl;

		if (isLockedOnPlayer) {
			JPH::Vec3 currentVelocity = character->GetLinearVelocity();
			JPH::Vec3 directionToCharacter = getDirectionToCharacter();
			JPH::Vec3 desiredVelocity = sprinterSettings->movementSpeed * directionToCharacter;

			// std::cout << "  directionToCharacter=("
			// 		<< directionToCharacter.GetX() << ", "
			// 		<< directionToCharacter.GetY() << ", "
			// 		<< directionToCharacter.GetZ() << ")" << std::endl;
			// 
			// std::cout << "  desiredVelocity=("
			//		<< desiredVelocity.GetX() << ", "
			// 		<< desiredVelocity.GetY() << ", "
			// 		<< desiredVelocity.GetZ() << ")" << std::endl;

			JPH::Vec3 newVelocity = 0.2f * currentVelocity + 0.8f * desiredVelocity;

			this->character->SetLinearVelocity(newVelocity);
		}

		float t = std::clamp(this->sprinterSettings->rotationSpeed, 0.0f, 1.0f);
		float updatedAngle = currentHorizontalAngle + t * angleDiff;

		// Normalize to [-pi, pi]
		while (updatedAngle > glm::pi<float>()) updatedAngle -= 2.0f * glm::pi<float>();
		while (updatedAngle < -glm::pi<float>()) updatedAngle += 2.0f * glm::pi<float>();

		// std::cout << "  updatedAngle=" << updatedAngle << std::endl;

		JPH::Quat quatRotation = JPH::Quat::sRotation(JPH::Vec3(0,1,0), updatedAngle);
		
		this->character->SetRotation(quatRotation);
	}

	float Sprinter::calculateTargetAngle() {
		std::shared_ptr<ISceneManagerInteraction> sceneManager = this->sceneManagerInteraction.lock();

		if (!sceneManager) {
			return this->character->GetRotation().GetRotationAngle({0,1,0});
		}

		glm::vec3 playerPosition = sceneManager->getPlayer()->getPosition();
		glm::vec3 enemyPosition = this->getPosition();
		return std::atan2(enemyPosition.z - playerPosition.z, playerPosition.x - enemyPosition.x);
	}

	JPH::Vec3 Sprinter::getDirectionToCharacter() {
		std::shared_ptr<ISceneManagerInteraction> sceneManager = this->sceneManagerInteraction.lock();

		if (!sceneManager) {
			return JPH::Vec3::sZero();
		}

		glm::vec3 playerPosition = sceneManager->getPlayer()->getPosition();
		glm::vec3 enemyPosition = this->getPosition();

		glm::vec3 direction = playerPosition - enemyPosition;

		if (glm::length(direction) <= 0.001) {
			return JPH::Vec3::sZero();
		}

		JPH::Vec3 returnValue = GLMToRVec3(direction).Normalized();

		return returnValue;
	}

	void Sprinter::addPhysicsBody() {
		character->AddToPhysicsSystem();
	}

	void Sprinter::removePhysicsBody() {
		character->RemoveFromPhysicsSystem();
	}

	void Sprinter::postSimulation() {
		character->PostSimulation(sprinterSettings->maxFloorSeparationDistance);
	}

	glm::mat4 Sprinter::computeModelMatrix() const {
		return RMat44ToGLM(character->GetWorldTransform());
	}

	glm::mat4 Sprinter::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	bool Sprinter::subtractHealth(float healthToSubtract) {
		this->currentHealth -= healthToSubtract;

		if (this->currentHealth <= 0) {
			this->markForDeletion();
			return true;
		}
		return false;
	}

	glm::vec3 Sprinter::getPosition() const {
		return RVec3ToGLM(this->character->GetPosition());
	}

	JPH::BodyID Sprinter::getBodyID() {
		return this->character->GetBodyID();
	}

	void Sprinter::printInfo(int iterationStep) const {

		// Output current position and velocity of the enemy

		JPH::RVec3 position = character->GetCenterOfMassPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "Enemy (Sprinter) [" << this->id <<"] : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << "), health = " << currentHealth << "/" << getMaxHealth() << std::endl;
	}

	float Sprinter::getMaxHealth() const {
		return this->sprinterSettings->maxHealth;
	}

	float Sprinter::getCurrentHealth() const {
		return this->currentHealth;
	}

	std::shared_ptr<vk::Model> Sprinter::getModel() const {
		return this->sprinterSettings->model;
	}
}