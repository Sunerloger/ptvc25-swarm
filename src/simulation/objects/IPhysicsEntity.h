#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

namespace physics {
	class IPhysicsEntity {

	public:

		~IPhysicsEntity() = default;

		// also creates physics body if it does not exist yet
		virtual void addPhysicsBody() = 0;

		// does not delete it (state is saved) - this should be done by the destructor
		virtual void removePhysicsBody() = 0;

		virtual JPH::BodyID getBodyID() const = 0;
	};
}