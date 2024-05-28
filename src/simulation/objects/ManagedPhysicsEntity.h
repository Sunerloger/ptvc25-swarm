#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "IPhysicsEntity.h"
#include "../../GameObject.h"

using namespace vk;
using namespace JPH;

namespace physics {
	class ManagedPhysicsEntity : public vk::GameObject, public IPhysicsEntity {
	
	public:
		
		virtual ~ManagedPhysicsEntity();

		void removePhysicsBody() override;

		BodyID getBodyID() override { return this->bodyID; }
	
	protected:

		ManagedPhysicsEntity(PhysicsSystem& physics_system);

		PhysicsSystem& physics_system;
		BodyID bodyID;
	};
}