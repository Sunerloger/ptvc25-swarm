#include "Sprinter.h"

#include "../../../../scene/SceneManager.h"
#include "../../../../AudioSystem.h"

#include <iostream>
#include <string>

namespace physics {

	Sprinter::Sprinter(SprinterCreationSettings sprinterCreationSettings, JPH::PhysicsSystem& physics_system) : sprinterSettings(sprinterCreationSettings.sprinterSettings), characterSettings(sprinterCreationSettings.characterSettings), physics_system(physics_system) {
		this->character = std::unique_ptr<JPH::Character>(new JPH::Character(&this->characterSettings, sprinterCreationSettings.position, JPH::Quat::sIdentity(), sprinterCreationSettings.inUserData, &this->physics_system));
		this->forward = RVec3ToGLM(getDirectionToCharacter());
		this->currentHealth = sprinterSettings.maxHealth;
	}

	void Sprinter::awake() {
		audio::AudioSystem& audioSystem = audio::AudioSystem::getInstance();
		audio::SoundSettings soundSettings{};
		soundSettings.looping = true;
		soundSettings.volume = 0.5;
		soundSettings.attenuationModel = audio::AttenuationModel::INVERSE_DISTANCE;
		soundSettings.minDistance = 2.0f;
		soundSettings.maxDistance = 100.0f;
		soundSettings.rolloffFactor = 0.5f;
		audioSystem.playSoundAt("growl", getPosition(), soundSettings, std::to_string(getId()));
	}

	Sprinter::~Sprinter() {
		removePhysicsBody();
		audio::AudioSystem& audioSystem = audio::AudioSystem::getInstance();
		audioSystem.stopSound(std::to_string(getId()));
	}

	// doesn't move if the enemy doesn't approximately face the player
	void Sprinter::updatePhysics(float cPhysicsDeltaTime) {
		// TODO deltaTime is handled by the physics system ?

		// player not within detection radius
		if (glm::length(SceneManager::getInstance().getPlayer()->getPosition() - this->getPosition()) > sprinterSettings.detectionRange) {
			return;
		}

		float targetAngle = calculateTargetAngle();
		float currentHorizontalAngle = std::atan2(forward.z, forward.x);

		// std::cout << "Enemy [" << this->id << "] currentAngle=" << currentHorizontalAngle
		//	 	<< " targetAngle=" << targetAngle
		//		<< " rotationSpeed=" << this->sprinterSettings->rotationSpeed
		//	 	<< " movementAngle=" << this->sprinterSettings->movementAngle
		//		<< std::endl;

		float diff = targetAngle - currentHorizontalAngle;
		while (diff > glm::pi<float>()) diff -= 2.0f * glm::pi<float>();
		while (diff < -glm::pi<float>()) diff += 2.0f * glm::pi<float>();

		// std::cout << "  normalized angles: current=" << currentHorizontalAngle
		// 		<< " target=" << targetAngle
		// 		<< " diff=" << angleDiff
		// 		<< std::endl;

		bool isLockedOnPlayer = std::fabs(diff) <= this->sprinterSettings.movementAngle;

		// std::cout << "  isLockedOnPlayer=" << (isLockedOnPlayer ? "true" : "false") << std::endl;

		if (isLockedOnPlayer) {
			JPH::Vec3 currentVelocity = character->GetLinearVelocity();
			JPH::Vec3 directionToCharacter = getDirectionToCharacter();

			// Create a horizontal-only direction vector
			JPH::Vec3 horizontalDirection = directionToCharacter;
			horizontalDirection.SetY(0.0f);	 // Zero out Y component for horizontal movement

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

			JPH::Vec3 newVelocity = currentVelocity;

			if (ground_state != JPH::Character::EGroundState::InAir) {
				// Blend current and desired velocity (with acceleration)
				newVelocity += cPhysicsDeltaTime *
							   this->sprinterSettings.accelerationToMaxSpeed * (desiredVelocity - currentVelocity);
			}

			// Apply a small upward force when on ground to help with slopes
			if (ground_state == JPH::Character::EGroundState::OnGround &&
				glm::abs(newVelocity.GetX()) + glm::abs(newVelocity.GetZ()) > 0.1f) {
				newVelocity.SetY(newVelocity.GetY() + 0.5f);  // Small upward boost
			}

			this->character->SetLinearVelocity(newVelocity);
		}
	}

	void Sprinter::updateVisuals(float deltaTime) {
		float targetAngle = calculateTargetAngle();
		float currentHorizontalAngle = std::atan2(forward.z, forward.x);

		float diff = targetAngle - currentHorizontalAngle;
		while (diff > glm::pi<float>()) diff -= 2.0f * glm::pi<float>();
		while (diff < -glm::pi<float>()) diff += 2.0f * glm::pi<float>();

		float maxStep = sprinterSettings.turnSpeed * deltaTime;
		float step = std::clamp(diff, -maxStep, maxStep);
		float newAng = currentHorizontalAngle + step;

		forward = glm::vec3(std::cos(newAng), 0.0f, std::sin(newAng));
	}

	float Sprinter::calculateTargetAngle() {
		glm::vec3 dir = RVec3ToGLM(getDirectionToCharacter());
		return atan2(dir.z, dir.x);
	}

	JPH::Vec3 Sprinter::getDirectionToCharacter() {
		SceneManager& sceneManager = SceneManager::getInstance();

		glm::vec3 playerPosition = sceneManager.getPlayer()->getPosition();
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

		audio::AudioSystem& audioSystem = audio::AudioSystem::getInstance();
		audioSystem.set3dSourceParameters(std::to_string(getId()), getPosition(), getVelocity());
	}

	glm::mat4 Sprinter::computeModelMatrix() const {
		glm::vec3 pos = RVec3ToGLM(character->GetPosition());

		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::quat orientation = glm::quatLookAt(forward, up);
		glm::quat mappedOrientation = glm::rotate(orientation, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
		glm::mat4 R = glm::toMat4(mappedOrientation);

		// Apply correction translation to push the model up a bit
		glm::mat4 T_correction = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.f, 0.0f));

		// Apply correction rotation to make the enemy model stand upright
		glm::mat4 R_correction_1 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		// Apply additional Y-axis rotation to make the enemy face the correct direction
		glm::mat4 R_correction_2 = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		return T * R * T_correction * R_correction_1 * R_correction_2;
	}

	glm::mat4 Sprinter::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	bool Sprinter::takeDamage(float healthToSubtract, glm::vec3 direction, float knockbackSpeed) {
		this->currentHealth -= healthToSubtract;

		if (direction != glm::vec3(0.0f)) {
			JPH::Vec3 dir = GLMToRVec3(glm::normalize(direction));

			// apply short lived velocity
			character->SetLinearVelocity(dir * knockbackSpeed);
		}

		if (this->currentHealth <= 0) {
			this->markForDeletion();
			return true;
		}
		return false;
	}

	glm::vec3 Sprinter::getPosition() const {
		return RVec3ToGLM(this->character->GetPosition());
	}

	glm::vec3 Sprinter::getVelocity() const {
		return RVec3ToGLM(this->character->GetLinearVelocity());
	}

	JPH::BodyID Sprinter::getBodyID() const {
		return this->character->GetBodyID();
	}

	void Sprinter::printInfo(int iterationStep) const {
		// Output current position and velocity of the enemy

		JPH::RVec3 position = character->GetPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "Enemy (Sprinter) [" << this->id << "] : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << "), health = " << currentHealth << "/" << getMaxHealth() << std::endl;
	}

	float Sprinter::getMaxHealth() const {
		return this->sprinterSettings.maxHealth;
	}

	float Sprinter::getCurrentHealth() const {
		return this->currentHealth;
	}

	float Sprinter::getBaseDamage() const {
		return this->sprinterSettings.baseDamage;
	}

	std::shared_ptr<vk::Model> Sprinter::getModel() const {
		return this->sprinterSettings.model;
	}
}