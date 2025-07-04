#pragma once

#include "../ManagedPhysicsEntity.h"
#include "../../../vk/vk_model.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Core/Reference.h>

#include <chrono>
#include <functional>

namespace physics {

	class Grenade : public ManagedPhysicsEntity {
	   public:
		struct GrenadeSettings {
			float explosionRadius = 10.0f;
			float explosionDamage = 80.0f;
			float fuseTime = 2.0f;	// seconds before explosion
			float mass = 1.0f;		// kg
			float radius = 0.2f;	// meters
			bool enableDebugOutput = false;
		};

		struct GrenadeCreationSettings {
			JPH::RVec3 position;
			JPH::Vec3 initialVelocity;
			GrenadeSettings grenadeSettings;
			std::shared_ptr<vk::Model> model = nullptr;
		};

		Grenade(const GrenadeCreationSettings& settings, JPH::PhysicsSystem& physics_system);
		virtual ~Grenade() = default;

		void addPhysicsBody() override;
		void updatePhysics(float deltaTime) override;

		// GameObject interface
		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<vk::Model> getModel() const override;

		bool hasExploded() const {
			return exploded;
		}
		bool shouldExplode() const;
		void explode();

		float getExplosionRadius() const {
			return settings.explosionRadius;
		}
		float getExplosionDamage() const {
			return settings.explosionDamage;
		}

	   private:
		GrenadeSettings settings;
		std::shared_ptr<vk::Model> model;

		std::chrono::steady_clock::time_point creationTime;
		std::chrono::steady_clock::time_point explosionTime;
		bool exploded = false;
		bool markedForDeletion = false;
		static constexpr float DELETION_DELAY = 0.1f;

		void createPhysicsBody(const JPH::RVec3& position, const JPH::Vec3& initialVelocity);
	};
}
