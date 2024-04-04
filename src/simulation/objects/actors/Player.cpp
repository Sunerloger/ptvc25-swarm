#include "Player.h"

namespace physics {

	static const float cCollisionTolerance = 0.05f;

	Player::Player(BodyInterface& body_interface) : PhysicsEntity(body_interface) {
		body_settings = new BodyCreationSettings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
	}

	Player::~Player() {
		delete body_settings;
		body_settings = nullptr;
	}

	void Player::addPhysicsBody() {
		
		bodyID = body_interface.CreateAndAddBody(*body_settings, EActivation::Activate);

		// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
		// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
		body_interface.SetLinearVelocity(bodyID, Vec3(0.0f, -5.0f, 0.0f));
	}
}