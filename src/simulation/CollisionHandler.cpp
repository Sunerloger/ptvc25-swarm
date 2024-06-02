#include "CollisionHandler.h"

// STL includes
#include <iostream>

namespace physics {

	JPH::ValidateResult MyContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) {
		std::cout << "Contact validate callback" << std::endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
		std::cout << "A contact was added" << std::endl;
	}

	void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
		std::cout << "A contact was persisted" << std::endl;
	}

	void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
		std::cout << "A contact was removed" << std::endl;
	}



	void MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
		std::cout << "A body got activated" << std::endl;
	}

	void MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
		std::cout << "A body went to sleep" << std::endl;
	}

}