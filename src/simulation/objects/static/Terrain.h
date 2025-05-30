// TODO provides functions to manipulate terrain generated in geometry.cpp + store geometry

#pragma once

#include "../ManagedPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

#include "../../PhysicsConversions.h"
#include "../../CollisionSettings.h"
#include <random>

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

namespace physics {
	class Terrain : public ManagedPhysicsEntity {
	   public:
		// Constructor for simple box terrain
		Terrain(PhysicsSystem& physics_system, glm::vec3 color, std::shared_ptr<vk::Model> model, glm::vec3 position, glm::vec3 scale = {1.0f, 1.0f, 1.0f});
		
		// Constructor with externally provided heightmap data
		Terrain(PhysicsSystem& physics_system, glm::vec3 color, std::shared_ptr<vk::Model> model,
		        glm::vec3 position, glm::vec3 scale, std::vector<float>&& heightfieldData);
		
		virtual ~Terrain();

		void addPhysicsBody() override;

		// model information
		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<vk::Model> getModel() const override;

		void toggleWireframeModeIfSupported() override;
		
		std::shared_ptr<vk::Model> model;
		glm::vec3 scale;

		bool useHeightfield = false;
		std::vector<float> heightfieldSamples;
		
		// For Perlin noise generation
		std::vector<int> p; // Permutation table for Perlin noise
		
		BodyCreationSettings body_settings;
	};
}

// TODO create model from dynamically created mesh and enhance model in tesselation shaders

// TODO procedural texturing