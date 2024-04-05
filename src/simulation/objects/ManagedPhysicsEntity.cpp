#include "ManagedPhysicsEntity.h"

namespace physics {

	ManagedPhysicsEntity::ManagedPhysicsEntity(PhysicsSystem& physics_system) : physics_system(physics_system) {}

	ManagedPhysicsEntity::~ManagedPhysicsEntity() {
		BodyInterface& body_interface = physics_system.GetBodyInterface();
		body_interface.DestroyBody(bodyID);
	}

	void ManagedPhysicsEntity::removePhysicsBody() {
		physics_system.GetBodyInterface().RemoveBody(bodyID);
	}

	BodyID ManagedPhysicsEntity::getBodyID() {
		return bodyID;
	}
}