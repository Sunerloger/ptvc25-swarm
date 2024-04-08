#include "ManagedPhysicsEntity.h"

namespace physics {

	ManagedPhysicsEntity::ManagedPhysicsEntity(PhysicsSystem& physics_system) : physics_system(physics_system) {}

	ManagedPhysicsEntity::~ManagedPhysicsEntity() {
		
		// if not created -> don't delete
		if (bodyID.IsInvalid()) { return; }

		removePhysicsBody();

		BodyInterface& body_interface = physics_system.GetBodyInterface();
		body_interface.DestroyBody(bodyID);
	}

	void ManagedPhysicsEntity::removePhysicsBody() {

		// if not created -> don't remove
		if (bodyID.IsInvalid()) { return; }

		// checks automatically if body is added or active
		physics_system.GetBodyInterface().RemoveBody(bodyID);
	}
}