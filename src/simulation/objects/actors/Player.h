#pragma once

#include "../../../camera/CharacterCamera.h"
#include "../../../GameObject.h"
#include "../IPhysicsEntity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../../PhysicsUtils.h"

using namespace JPH;

namespace physics {

	struct PlayerSettings {

		// update per second
		float movementSpeed = 7.0f;
		float jumpHeight = 3.0f;
		float maxFloorSeparationDistance = 0.05f;
		bool controlMovementDuringJump = true;
	};

	struct PlayerCreationSettings {
		RVec3Arg position = RVec3::sZero();

		// you probably don't want to set that but the camera rotation
		// this only rotates the physics-body
		QuatArg rotation = Quat::sIdentity();

		PlayerSettings* playerSettings;
		CharacterCameraSettings* cameraSettings;

		CharacterSettings* characterSettings;

		uint64 inUserData = 0;
	};

	class Player : public vk::GameObject, public IPhysicsEntity {

	public:

		Player(PlayerCreationSettings* playerSettings, PhysicsSystem* physics_system);
		virtual ~Player();

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		void postSimulation();

		void handleMovement(Vec3 movementDirectionWorld, bool isJump);
		void handleRotation(float deltaYaw, float deltaPitch, float deltaTime);

		const glm::vec3 getCameraPosition() const;
		const glm::mat4 calculateViewMat() const { return camera->calculateViewMat(); }
		const glm::mat4 getProjMat() const { return camera->getProjMat(); }
		const glm::vec3 getFront() const { return camera->getFront(); }

		void printPosition(int iterationStep) const;

		BodyID getBodyID() override;

		void setViewDirection(glm::vec3 direction) { camera->setViewDirection(direction); }
		void setViewTarget(glm::vec3 target) { camera->setViewTarget(target); }

		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) { 
			camera->setOrthographicProjection(left, right, top, bottom, near, far); }
		void setPerspectiveProjection(float fov, float aspect, float near, float far) { 
			camera->setPerspectiveProjection(fov, aspect, near, far); }

	private:

		float maxHealth = 100.0f;
		float currentHealth = 100.0f;

		PlayerSettings* settings;
		CharacterSettings* characterSettings;

		std::unique_ptr<CharacterCamera> camera;
		std::unique_ptr<Character> character;
	};
}