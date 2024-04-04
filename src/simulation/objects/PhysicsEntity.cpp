#include "PhysicsEntity.h"

namespace physics {

	PhysicsEntity::PhysicsEntity(BodyInterface& body_interface) : body_interface(body_interface) {}

	PhysicsEntity::~PhysicsEntity() {

		if (body_interface.IsAdded(bodyID)) { // body is created in physics system

			body_interface.RemoveBody(bodyID); // automatically checks if body is active
			body_interface.DestroyBody(bodyID);
		}
	}

	void PhysicsEntity::removePhysicsBody() {
		body_interface.RemoveBody(bodyID);
	}

	BodyID PhysicsEntity::getBodyID() {
		return bodyID;
	}
}