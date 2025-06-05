#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "IPhysicsEntity.h"
#include "../../GameObject.h"

namespace physics {
	class ManagedPhysicsEntity : public vk::GameObject, public IPhysicsEntity {
	   public:
		virtual ~ManagedPhysicsEntity();

		void removePhysicsBody() override;

		JPH::BodyID getBodyID() const override {
			return this->bodyID;
		}

		// Virtual method for physics updates - can be overridden by derived classes that need updates
		virtual void updatePhysics(float deltaTime) {}

	   protected:
		ManagedPhysicsEntity(JPH::PhysicsSystem& physics_system);

		JPH::PhysicsSystem& physics_system;
		JPH::BodyID bodyID;
	};
}

// (Override the MyShapeSettings::Create function to construct an instance of MyShape for custom physics bodies)