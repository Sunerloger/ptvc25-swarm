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
		float movementSpeed = 10;
		float jumpSpeed = 10;
		float maxFloorSeparationDistance = 0.05f;
		bool controlMovementDuringJump = true;
	};

	struct PlayerCreationSettings {
		RVec3Arg position = RVec3::sZero();
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

		virtual void postSimulation();

		virtual void handleMovement(Vec3 movementDirectionWorld, bool isJump);
		virtual void handleRotation(float deltaYaw, float deltaPitch, float deltaTime);

		virtual glm::vec3 getCameraPosition();
		virtual glm::mat4 getViewProjMatrix();
		virtual glm::vec3 getFront();

		void printPosition(int iterationStep);

	private:

		PlayerSettings* settings;

		std::unique_ptr<CharacterCamera> camera;
		std::unique_ptr<Character> character;
	};
}