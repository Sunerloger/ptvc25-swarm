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

		// deltaTime is handled by the physics system

		bool isLockedOnPlayer = false;

		float targetAngle = calculateTargetAngle();
		float currentHorizontalAngle = this->character->GetRotation().GetEulerAngles().GetY();

		if (std::fabs(currentHorizontalAngle - targetAngle) <= this->sprinterSettings->movementAngle) {
			isLockedOnPlayer = true;
		}

		if (isLockedOnPlayer) {
			JPH::Vec3 currentVelocity = character->GetLinearVelocity();
			JPH::Vec3 directionToCharacter = getDirectionToCharacter();
			JPH::Vec3 desiredVelocity = sprinterSettings->movementSpeed * directionToCharacter;

			JPH::Vec3 newVelocity = 0.75f * currentVelocity + 0.25f * desiredVelocity;

			this->character->SetLinearVelocity(newVelocity);
		}
		
		// TODO correct agle wrap around
		float t = std::clamp(this->sprinterSettings->rotationSpeed, 0.0f, 1.0f);
		float updatedAngle = (1 - t) * currentHorizontalAngle + t * targetAngle;

		JPH::Vec3Arg eulerAngles = this->character->GetRotation().GetEulerAngles();
		eulerAngles.SetY(updatedAngle);

		JPH::QuatArg quatRotation = JPH::QuatArg::sEulerAngles(eulerAngles);
		
		this->character->SetRotation(quatRotation);
	}

	float Sprinter::calculateTargetAngle() {
		std::shared_ptr<ISceneManagerInteraction> sceneManager = this->sceneManagerInteraction.lock();

		if (!sceneManager) {
			return this->character->GetRotation().GetEulerAngles().GetY();
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
			this->destroyInScene();
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

	void Sprinter::printPosition(int iterationStep) const {

		// Output current position and velocity of the enemy

		JPH::RVec3 position = character->GetCenterOfMassPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "Enemy (Sprinter) [" << this->id <<"] : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << std::endl;
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