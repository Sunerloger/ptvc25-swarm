#include "Sprinter.h"

#include <iostream>

namespace physics {

	Sprinter::Sprinter(SprinterCreationSettings sprinterCreationSettings, JPH::PhysicsSystem& physics_system) : 
		sprinterSettings(sprinterCreationSettings.sprinterSettings), characterSettings(sprinterCreationSettings.characterSettings), physics_system(physics_system) {
		this->character = std::unique_ptr<JPH::Character>(new JPH::Character(&this->characterSettings, sprinterCreationSettings.position, JPH::Quat::sIdentity(), sprinterCreationSettings.inUserData, &this->physics_system));

		this->currentHealth = sprinterSettings.maxHealth;
	}

	Sprinter::~Sprinter() {
		removePhysicsBody();
	}

	// doesn't move if the enemy doesn't approximately face the player
	void Sprinter::update(float cPhysicsDeltaTime) {

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

		bool isLockedOnPlayer = std::fabs(angleDiff) <= this->sprinterSettings.movementAngle;

		// std::cout << "  isLockedOnPlayer=" << (isLockedOnPlayer ? "true" : "false") << std::endl;

		if (isLockedOnPlayer) {
			JPH::Vec3 currentVelocity = character->GetLinearVelocity();
			JPH::Vec3 directionToCharacter = getDirectionToCharacter();
			
			// Create a horizontal-only direction vector
			JPH::Vec3 horizontalDirection = directionToCharacter;
			horizontalDirection.SetY(0.0f);  // Zero out Y component for horizontal movement
			
			// Handle slopes - similar to Player class
			JPH::Character::EGroundState ground_state = this->character->GetGroundState();
			if (ground_state == JPH::Character::EGroundState::OnSteepGround ||
				ground_state == JPH::Character::EGroundState::NotSupported) {
				
				// Get ground normal and project it to horizontal plane
				JPH::Vec3 normal = character->GetGroundNormal();
				JPH::Vec3 horizontalNormal = normal;
				horizontalNormal.SetY(0.0f);
				
				float normal_length_sq = horizontalNormal.LengthSq();
				if (normal_length_sq > 0.0f) {
					// Calculate dot product to see if we're moving into the slope
					float dot = horizontalNormal.Dot(horizontalDirection);
					if (dot < 0.0f) {
						// Adjust direction to slide along the slope instead of into it
						horizontalDirection -= (dot * horizontalNormal) / normal_length_sq;
					}
				}
			}
			
			// Re-normalize after adjustments
			if (horizontalDirection.LengthSq() > 0.001f) {
				horizontalDirection = horizontalDirection.Normalized();
			}
			
			// Calculate desired horizontal velocity
			JPH::Vec3 desiredVelocity = horizontalDirection * sprinterSettings.maxMovementSpeed;
			
			// Preserve current Y velocity (gravity)
			desiredVelocity.SetY(currentVelocity.GetY());
			
			// Blend current and desired velocity (with acceleration)
			JPH::Vec3 newVelocity = currentVelocity + cPhysicsDeltaTime *
				this->sprinterSettings.accelerationToMaxSpeed * (desiredVelocity - currentVelocity);
			
			// Apply a small upward force when on ground to help with slopes
			if (ground_state == JPH::Character::EGroundState::OnGround &&
				glm::abs(newVelocity.GetX()) + glm::abs(newVelocity.GetZ()) > 0.1f) {
				newVelocity.SetY(newVelocity.GetY() + 0.5f); // Small upward boost
			}
			
			this->character->SetLinearVelocity(newVelocity);
		}

		// rad/s
		float rotationSpeed = glm::pi<float>() * sprinterSettings.rotationTime;

		float updatedAngle;

		// s = s0 + t * v
		if (angleDiff >= 0) {
			updatedAngle = currentHorizontalAngle + cPhysicsDeltaTime * rotationSpeed;
			if (updatedAngle > targetAngle) { updatedAngle = targetAngle; }
		}
		else { // angleDiff < 0
			updatedAngle = currentHorizontalAngle - cPhysicsDeltaTime * rotationSpeed;
			if (updatedAngle < targetAngle) { updatedAngle = targetAngle; }
		}

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

		// Calculate direction vector to player
		glm::vec3 direction = playerPosition - enemyPosition;
		
		// Slightly increase the Y component to help with upward movement
		// This makes enemies try to move slightly upward toward the player
		direction.y += 0.5f;

		if (glm::length(direction) <= 0.001) {
			return JPH::Vec3::sZero();
		}

		// Convert to JPH vector and normalize
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
		character->PostSimulation(sprinterSettings.maxFloorSeparationDistance);
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

		JPH::RVec3 position = character->GetPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "Enemy (Sprinter) [" << this->id <<"] : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << "), health = " << currentHealth << "/" << getMaxHealth() << std::endl;
	}

	float Sprinter::getMaxHealth() const {
		return this->sprinterSettings.maxHealth;
	}

	float Sprinter::getCurrentHealth() const {
		return this->currentHealth;
	}

	std::shared_ptr<vk::Model> Sprinter::getModel() const {
		return this->sprinterSettings.model;
	}
}