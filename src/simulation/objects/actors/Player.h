#pragma once

#include "../../../camera/FPVCamera.h"
#include "../PhysicsEntity.h"

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
	class Player : public PhysicsEntity {

	public:
		// JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, Player)

		Player(BodyInterface& body_interface);
		virtual ~Player();

		virtual void addPhysicsBody();

	private:

		BodyCreationSettings* body_settings = nullptr;

		// before the physics update
		// FPVCamera& camera;

		// RefConst<Shape> standingShape;
		// Ref<Character> body;

	};
}

// TODO all camera transformations and rotations caused by the physics simulation and main pass through this class and update both the camera and the body
