#pragma once

#include "../../../camera/CharacterCamera.h"
#include "../../../GameObject.h"
#include "../IPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../../PhysicsConversions.h"

namespace physics {

	struct PlayerSettings {

		// update per second
		float movementSpeed = 4.5f;
		float jumpHeight = 1.0f;
		float maxFloorSeparationDistance = 0.05f;
		bool controlMovementDuringJump = true;
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

	class Player : public vk::GameObject, public IPhysicsEntity {

	public:

		float maxHealth = 100.0f;
		float currentHealth = 100.0f;

		PlayerSettings settings;

		Player(PlayerCreationSettings playerSettings, JPH::PhysicsSystem& physics_system);
		virtual ~Player();

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		void postSimulation();

		void setInputDirection(const glm::vec3 dir);

		void handleMovement(float deltaTime);
		void handleRotation(float deltaYaw, float deltaPitch);
		void handleJump();
		void handleShoot();

		const glm::vec3 getCameraPosition() const;
		inline const glm::mat4 calculateViewMat() const { return camera.calculateViewMat(); }
		inline const glm::mat4 getProjMat() const { return camera.getProjMat(); }
		inline const glm::vec3 getFront() const { return camera.getFront(); }

		void printInfo(int iterationStep) const;

		JPH::BodyID getBodyID() override;

		inline void setViewDirection(glm::vec3 direction) { camera.setViewDirection(direction); }
		inline void setViewTarget(glm::vec3 target) { camera.setViewTarget(target); }

		inline void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) { 
			camera.setOrthographicProjection(left, right, top, bottom, near, far); }
		inline void setPerspectiveProjection(float fov, float aspect, float near, float far) {
			camera.setPerspectiveProjection(fov, aspect, near, far); }

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		inline std::shared_ptr<vk::Model> getModel() const override { return nullptr; }

	private:

		JPH::CharacterSettings characterSettings;

		CharacterCamera camera;
		std::unique_ptr<JPH::Character> character;

		JPH::PhysicsSystem& physics_system;

		glm::vec3 currentMovementDirection = glm::vec3{ 0 };
	};
}