#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "IPhysicsEntity.h"
#include "../../GameObject.h"

using namespace vk;
using namespace JPH;

namespace physics {
	class ManagedPhysicsEntity : public GameObject, public IPhysicsEntity {
	
	public:
		ManagedPhysicsEntity(PhysicsSystem& physics_system);

		// TODO do not delete a physics entity before deleting it from its scene!
		virtual ~ManagedPhysicsEntity();

		void removePhysicsBody() override;
	
	protected:

		PhysicsSystem& physics_system;
		BodyID bodyID;
	};
}