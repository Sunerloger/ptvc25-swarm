#pragma once

#include "Player.h"
#include "../../../camera/CharacterCamera.h"

#include <glm/glm.hpp>
// #include <glm/gtc/quaternion.hpp>
// #include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <limits>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>

#include <iostream>

class DebugPlayer : public Player {

public:

	DebugPlayer(CharacterCameraSettings cameraSettings = {}, float movementSpeed = 7.0f);
	virtual ~DebugPlayer() = default;

	void printInfo(int iterationStep) const override;

	inline void takeDamage(float damage) override {}

	inline float getCurrentHealth() const override { return std::numeric_limits<float>::max();}

	void handleRotation(float deltaYaw, float deltaPitch) override;

	inline const glm::mat4 calculateViewMat() const override { return camera.calculateViewMat(); }

	inline const glm::mat4 getProjMat() const override { return camera.getProjMat(); }

	inline void setPerspectiveProjection(float fov, float aspect, float near, float far) override {
		camera.setPerspectiveProjection(fov, aspect, near, far);
	}

	inline CharacterCameraSettings getCameraSettings() const override { return camera.getSettings(); }

	inline JPH::BodyID getBodyID() const override { return JPH::BodyID(JPH::BodyID::cInvalidBodyID); }
	inline void addPhysicsBody() override {}

	void handleSpeedChange(float scrollOffset, float changeSpeed = 1.0f);
	void updatePosition(float dt, glm::vec3 dir);

	inline float getMovementSpeed() const override { return movementSpeed; }

	inline glm::mat4 computeModelMatrix() const override { return glm::mat4{1.0f}; }
	inline glm::mat4 computeNormalMatrix() const override { return glm::mat4{1.0f}; }
	inline glm::vec3 getPosition() const override { return camera.getPosition(); }
	inline std::shared_ptr<vk::Model> getModel() const override { return nullptr; }

	glm::vec3 getCameraPosition() const override { return camera.getPosition(); };

private:

	float movementSpeed = 7.0f;

	CharacterCamera camera;
};