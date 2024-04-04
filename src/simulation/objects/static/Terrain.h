// TODO provides functions to manipulate terrain generated in geometry.cpp + store geometry

#pragma once

#include "../PhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include "../../PhysicsUtils.h"

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

namespace physics {
	class Terrain : public PhysicsEntity {

	public:
		// JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, Player)

		Terrain(BodyInterface& body_interface);
		virtual ~Terrain();

		virtual void addPhysicsBody();

	private:

		BodyCreationSettings* body_settings = nullptr;
	};
}