// TODO provides functions to manipulate terrain generated in geometry.cpp + store geometry

#pragma once

#include "../ManagedPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include "../../PhysicsUtils.h"

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

namespace physics {
	class Terrain : public ManagedPhysicsEntity {

	public:

		Terrain(PhysicsSystem& physics_system, glm::vec3 color, Model* model, glm::vec3 position);
		virtual ~Terrain();

		void addPhysicsBody() override;

		// model information
		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		Model* getModel() const override;

	private:

		Model* model;

		unique_ptr<BodyCreationSettings> body_settings;
	};
}

// TODO create model from dynamically created mesh and enhance model in tesselation shaders

// TODO procedural texturing