#pragma once

#include "../../../camera/CharacterCamera.h"
#include "Player.h"
#include "../IPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../../PhysicsConversions.h"

#include <functional>
#include <chrono>

namespace physics {

	class PhysicsPlayer : public Player, public IPhysicsEntity {
	   public:
		struct PlayerSettings {
			// update per second
			float movementSpeed = 7.0f;
			float jumpHeight = 1.0f;
			bool controlMovementDuringJump = true;

			float shootRange = 1000.0f;
			float grenadeRange = 100.0f;
			float shootDamage = 40.0f;
			float knockbackSpeed = 10.0f;
			float maxHealth = 100.0f;

			float maxFloorSeparationDistance = 0.05f;

			// Grenade cooldown in seconds (60 seconds = 1 minute)
			float grenadeCooldownTime = 60.0f;

			std::function<void()> deathCallback;
		};

		struct PlayerCreationSettings {
			JPH::RVec3 position = JPH::RVec3::sZero();

			// you probably don't want to set that but the camera rotation
			// this only rotates the physics-body
			JPH::QuatArg rotation = JPH::Quat::sIdentity();

			PlayerSettings playerSettings;
			CharacterCameraSettings cameraSettings;

			JPH::CharacterSettings characterSettings;

			JPH::uint64 inUserData = 0;
		};

		PlayerSettings settings;

		PhysicsPlayer(PlayerCreationSettings playerSettings, JPH::PhysicsSystem& physics_system);
		virtual ~PhysicsPlayer();

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		void postSimulation();

		void setInputDirection(const glm::vec3 dir);

		void handleMovement(float deltaTime);
		void handleRotation(float deltaYaw, float deltaPitch) override;
		void handleJump();
		void handleShoot();
		void handleThrowGrenade(vk::Device& device, std::shared_ptr<vk::Model> grenadeModel);

		// Grenade cooldown methods
		bool canThrowGrenade() const;
		float getGrenadeCooldownRemaining() const;
		void updateGrenadeCooldown(float deltaTime);

		float getMaxHealth() const;
		float getCurrentHealth() const override;
		void takeDamage(float healthToSubtract, glm::vec3 direction = glm::vec3{0}, float knockbackSpeed = 0.0f) override;
		bool isDead() const;

		glm::vec3 getCameraPosition() const override;
		inline const glm::mat4 calculateViewMat() const override { return camera.calculateViewMat(); }
		inline const glm::mat4 getProjMat() const override { return camera.getProjMat(); }
		glm::vec3 getFront() const override { return camera.getFront(); }
		glm::vec3 getUp() const override { return glm::vec3{0.0f, 1.0f, 0.0f}; }

		void printInfo(int iterationStep) const override;

		JPH::BodyID getBodyID() const override;

		inline void setViewDirection(glm::vec3 direction) {
			camera.setViewDirection(direction);
		}
		inline void setViewTarget(glm::vec3 target) {
			camera.setViewTarget(target);
		}

		inline void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
			camera.setOrthographicProjection(left, right, top, bottom, near, far);
		}
		inline void setPerspectiveProjection(float fov, float aspect, float near, float far) override {
			camera.setPerspectiveProjection(fov, aspect, near, far);
		}

		inline CharacterCameraSettings getCameraSettings() const override {
			return camera.getSettings();
		}

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		inline std::shared_ptr<vk::Model> getModel() const override {
			return nullptr;
		}
		inline float getMovementSpeed() const override {
			return settings.movementSpeed;
		}

		bool isPhysicsPlayer() const override {
			return true;
		}

		PlayerCreationSettings getCreationSettings() const;

	   private:
		JPH::CharacterSettings characterSettings;

		CharacterCamera camera;
		std::unique_ptr<JPH::Character> character;

		JPH::PhysicsSystem& physics_system;

		glm::vec3 currentMovementDirection = glm::vec3{0};

		float currentHealth = 100.0f;

		// Grenade cooldown system
		std::chrono::steady_clock::time_point lastGrenadeThrowTime;
		bool hasGrenadeAvailable = true;  // Start with one grenade available
	};
}