#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "../GameObject.h"

namespace physics {

	class MyContactListener : public JPH::ContactListener {

	public:

		MyContactListener();
		virtual ~MyContactListener();

		JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
		void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

		void handlePlayerEnemyCollision(vk::GameObject* player, vk::GameObject* enemy, float impactSpeed, const JPH::Vec3& normal);
	};



	class MyBodyActivationListener : public JPH::BodyActivationListener {

	public:

		MyBodyActivationListener();
		virtual ~MyBodyActivationListener();

		void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;

		void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
	};

}