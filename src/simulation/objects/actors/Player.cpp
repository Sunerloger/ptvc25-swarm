#include "Player.h"

namespace physics {

	Player::Player(PlayerCreationSettings* playerCreationSettings, PhysicsSystem* physics_system) {

		this->character = unique_ptr<Character>(new Character(playerCreationSettings->characterSettings, playerCreationSettings->position, playerCreationSettings->rotation, playerCreationSettings->inUserData, physics_system));
		this->camera = unique_ptr<CharacterCamera>(new CharacterCamera(playerCreationSettings->cameraSettings));

		this->settings = playerCreationSettings->playerSettings;
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

	void Player::handleMovement(Vec3 movementDirectionCharacter, bool isJump) {

		// deltaTime is handled by the physics system

		float yaw = glm::radians(camera->getYaw());
		RMat44 rotation_matrix = RMat44::sIdentity().sRotationY(yaw);

		Vec3 movementDirectionWorld = Vec3(rotation_matrix * Vec4(movementDirectionCharacter, 1));

		// Cancel movement in opposite direction of normal when touching something we can't walk up
		Character::EGroundState ground_state = this->character->GetGroundState();
		if (ground_state == Character::EGroundState::OnSteepGround || ground_state == Character::EGroundState::NotSupported) {
			Vec3 normal = character->GetGroundNormal();
			normal.SetY(0.0f);
			float dot = normal.Dot(movementDirectionWorld);
			if (dot < 0.0f) {
				movementDirectionWorld -= (dot * normal) / normal.LengthSq();
			}
		}

		if (settings->controlMovementDuringJump || character->IsSupported()) {
			
			// Update velocity
			Vec3 current_velocity = character->GetLinearVelocity();
			Vec3 desired_velocity = settings->movementSpeed * movementDirectionWorld;
			desired_velocity.SetY(current_velocity.GetY());
			Vec3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

			// Jump - OnGround also means you have friction
			if (isJump && ground_state == Character::EGroundState::OnGround) {
				new_velocity += Vec3(0, settings->jumpSpeed, 0);
			}

			// Update the velocity
			character->SetLinearVelocity(new_velocity);
		}
		// TODO test different speeds for different directions, double jump, different movement speed in air
	}

	void Player::handleRotation(float deltaYaw, float deltaPitch, float deltaTime) {
		camera->addRotation(deltaYaw, deltaPitch, deltaTime);
	}

	glm::vec3 Player::getFront() {
		return this->camera->getFront();
	}

	void Player::postSimulation() {
		character->PostSimulation(settings->maxFloorSeparationDistance);

		camera->setPhysicsPosition(character->GetCenterOfMassPosition());
	}

	glm::vec3 Player::getCameraPosition() {
		return camera->getPosition();
	}

	glm::mat4 Player::getViewProjMatrix() {
		return camera->getViewProjMatrix();
	}

	void Player::printPosition(int iterationStep) {

		// Output current position and velocity of the player, player needs to be set
		
		RVec3 position = character->GetCenterOfMassPosition();
		Vec3 velocity = character->GetLinearVelocity();
		cout << "Step " << iterationStep << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;
	}
}