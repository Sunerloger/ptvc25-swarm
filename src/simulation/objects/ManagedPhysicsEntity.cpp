#include "ManagedPhysicsEntity.h"

namespace physics {

	ManagedPhysicsEntity::ManagedPhysicsEntity(PhysicsSystem& physics_system) : physics_system(physics_system) {}

	ManagedPhysicsEntity::~ManagedPhysicsEntity() {
		BodyInterface& body_interface = physics_system.GetBodyInterface();

		if (body_interface.IsActive(bodyID)) {
			body_interface.RemoveBody(bodyID);
		}

		body_interface.DestroyBody(bodyID);
	}

	void ManagedPhysicsEntity::removePhysicsBody() {
		physics_system.GetBodyInterface().RemoveBody(bodyID);
	}

	BodyID ManagedPhysicsEntity::getBodyID() {
		return bodyID;
	}
}