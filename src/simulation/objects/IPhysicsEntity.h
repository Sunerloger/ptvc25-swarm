#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

using namespace JPH;

namespace physics {
	class IPhysicsEntity {

	public:

		// also creates physics body if it does not exist yet
		virtual void addPhysicsBody() = 0;

		// does not delete it (state is saved) - this should be done by the destructor
		virtual void removePhysicsBody() = 0;

		virtual BodyID getBodyID() = 0;
	};
}