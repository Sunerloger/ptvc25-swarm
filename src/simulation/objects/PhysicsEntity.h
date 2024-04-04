#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "../../GameObject.h"

using namespace vk;
using namespace JPH;

namespace physics {
	class PhysicsEntity : public GameObject {
	
	public:
		PhysicsEntity(BodyInterface& body_interface);

		// TODO do not delete a physics entity before deleting it from its scene!
		virtual ~PhysicsEntity();

		// also creates physics body if it does not exist yet
		virtual void addPhysicsBody() = 0;

		void removePhysicsBody();

		BodyID getBodyID();
	
	protected:

		BodyInterface& body_interface;
		BodyID bodyID;
	};
}