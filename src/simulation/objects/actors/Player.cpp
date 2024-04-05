#include "Player.h"

namespace physics {

	static const float cCollisionTolerance = 0.05f;

	Player::Player(PhysicsSystem& physics_system, float height, float width, float movementSpeed, float jumpHeight, float maxFloorSeparationDistance, float fov, float aspectRatio, float nearPlane, float farPlane) {

		this->physics_system = &physics_system;

		body_settings = new BodyCreationSettings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);

		QuatArg rotation = Quatarg();

		this->character = unique_ptr<Character>(new Character());

		this->camera = unique_ptr<FPVCamera>(new FPVCamera(fov, aspectRatio, nearPlane, farPlane));

		this->movementSpeed = movementSpeed;
		this->jumpHeight = jumpHeight;
		this->maxFloorSeparationDistance = maxFloorSeparationDistance;
	}

	Player::~Player() {
		delete body_settings;
		body_settings = nullptr;
	}

	void Player::addPhysicsBody() {
		character->AddToPhysicsSystem();
	}

	void Player::removePhysicsBody() {
		character->RemoveFromPhysicsSystem();
	}

	BodyID Player::getBodyID() {
		return character->GetBodyID();
	}

	void Player::postSimulation() {
		return character->PostSimulation(this->maxFloorSeparationDistance);
	}
}