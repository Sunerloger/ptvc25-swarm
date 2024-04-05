#pragma once

#include "../../../camera/FPVCamera.h"
#include "../../../GameObject.h"
#include "../IPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "../../PhysicsUtils.h"

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

namespace physics {
	class Player : public GameObject, public IPhysicsEntity {

	public:
		// JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, Player)

		Player(PhysicsSystem& physics_system, float height, float width, float movementSpeed, float jumpHeight, float maxFloorSeparationDistance, float fov, float aspectRatio, float nearPlane, float farPlane);
		virtual ~Player();

		void addPhysicsBody() override;

		void removePhysicsBody() override;

		BodyID getBodyID() override;

		void postSimulation();

	private:

		PhysicsSystem* physics_system;

		BodyCreationSettings* body_settings = nullptr;

		double movementSpeed;
		double jumpHeight;
		float maxFloorSeparationDistance;

		// before the physics update
		std::unique_ptr<FPVCamera> camera;

		std::unique_ptr<Character> character;

	};
}

// TODO all camera transformations and rotations caused by the physics simulation and main pass through this class and update both the camera and the body


// TODO just transform camera according to position and rotation of physics body and update physics body based on input
