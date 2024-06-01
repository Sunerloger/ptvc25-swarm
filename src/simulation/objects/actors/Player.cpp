#include "Player.h"

#include <iostream>

namespace physics {

	Player::Player(std::unique_ptr<PlayerCreationSettings> playerCreationSettings, std::shared_ptr<JPH::PhysicsSystem> physics_system) {
		this->settings = std::move(playerCreationSettings->playerSettings);
		this->characterSettings = std::move(playerCreationSettings->characterSettings);
		this->physics_system = physics_system;
		this->character = std::unique_ptr<JPH::Character>(new JPH::Character(this->characterSettings.get(), playerCreationSettings->position, playerCreationSettings->rotation, playerCreationSettings->inUserData, this->physics_system.get()));
		this->camera = std::unique_ptr<CharacterCamera>(new CharacterCamera(std::move(playerCreationSettings->cameraSettings)));
	}

	Player::~Player() {
		removePhysicsBody();
	}

	void Player::addPhysicsBody() {
		character->AddToPhysicsSystem();
	}

	void Player::removePhysicsBody() {
		character->RemoveFromPhysicsSystem();
	}

	void Player::handleMovement(JPH::Vec3 movementDirectionCharacter, bool isJump) {

		// deltaTime is handled by the physics system

		float yaw = glm::radians(camera->getYaw());
		JPH::RMat44 rotation_matrix = JPH::RMat44::sIdentity().sRotationY(yaw);

		JPH::Vec3 movementDirectionWorld = JPH::Vec3(rotation_matrix * JPH::Vec4(movementDirectionCharacter, 1));

		// Cancel movement in opposite direction of normal when touching something we can't walk up
		JPH::Character::EGroundState ground_state = this->character->GetGroundState();
		if (ground_state == JPH::Character::EGroundState::OnSteepGround || ground_state == JPH::Character::EGroundState::NotSupported) {
			JPH::Vec3 normal = character->GetGroundNormal();
			normal.SetY(0.0f);
			float dot = normal.Dot(movementDirectionWorld);
			if (dot < 0.0f) {
				movementDirectionWorld -= (dot * normal) / normal.LengthSq();
			}
		}

		if (settings->controlMovementDuringJump || character->IsSupported()) {
			
			// Update velocity
			JPH::Vec3 current_velocity = character->GetLinearVelocity();
			JPH::Vec3 desired_velocity = settings->movementSpeed * movementDirectionWorld;
			desired_velocity.SetY(current_velocity.GetY());
			JPH::Vec3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

			// Jump - OnGround also means you have friction
			if (isJump && ground_state == JPH::Character::EGroundState::OnGround) {
				new_velocity += JPH::Vec3(0, std::sqrt(2 * settings->jumpHeight * characterSettings->mGravityFactor * 9.81f), 0);
			}

			// Update the velocity
			character->SetLinearVelocity(new_velocity);
		}
		// TODO test different speeds for different directions, double jump, different movement speed in air
	}

	void Player::handleRotation(float deltaYaw, float deltaPitch, float deltaTime) {
		camera->addRotation(deltaYaw, deltaPitch, deltaTime);
	}

	void Player::postSimulation() {
		character->PostSimulation(settings->maxFloorSeparationDistance);

		camera->setPhysicsPosition(character->GetCenterOfMassPosition());
	}

	const glm::vec3 Player::getCameraPosition() const {
		return camera->getPosition();
	}

	void Player::printPosition(int iterationStep) const {

		// Output current position and velocity of the player
		
		JPH::RVec3 position = character->GetCenterOfMassPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "Player : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << std::endl;
	}

	JPH::BodyID Player::getBodyID() {
		return this->character->GetBodyID();
	}

	glm::mat4 Player::computeModelMatrix() const {
		return glm::mat4(1.0);
	}

	glm::mat4 Player::computeNormalMatrix() const {
		return glm::mat4(1.0);
	}

	glm::vec3 Player::getPosition() const {
		return RVec3ToGLM(this->character->GetPosition());
	}
}