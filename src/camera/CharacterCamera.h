#pragma once

#include "CameraUtils.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cmath>

#include <Jolt/Jolt.h>

struct CharacterCameraSettings {
	float yaw = 0.0f;
	float pitch = 0.0f;

	// offset from the point touching the ground
	glm::vec3 cameraOffsetFromCharacter = glm::vec3(0.0f, 1.0f, 0.0f);

	// rad/s but it gets scaled by mouse delta, so keep small
	float cameraSpeed = 0.07f;

	glm::vec3 position = glm::vec3(0);

	glm::mat4 projMatrix = glm::mat4(1);
};

class CharacterCamera {
   public:
	CharacterCamera(CharacterCameraSettings cameraSettings);
	virtual ~CharacterCamera();

	const glm::mat4 calculateViewMat() const;
	const glm::mat4 getProjMat() const;
	const glm::vec3 getPosition() const;

	void setPhysicsPosition(JPH::Vec3 physicsPosition);
	void setPosition(glm::vec3 newPosition);

	void addRotation(float deltaYaw, float deltaPitch);

	const glm::vec3 getFront() const;

	float getYaw() const;

	void setViewDirection(glm::vec3 direction);
	void setViewTarget(glm::vec3 target);

	CharacterCameraSettings getSettings() const;

	// flips y axis
	void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

	// flips y axis
	void setPerspectiveProjection(float fov, float aspect, float near, float far);

   private:
	CharacterCameraSettings settings;

	void setYaw(float yaw);
	void setPitch(float pitch);
};