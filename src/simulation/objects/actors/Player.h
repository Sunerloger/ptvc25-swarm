#pragma once

#include "../../../GameObject.h"
#include "../../../camera/CharacterCamera.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>

class Player : public vk::GameObject {

public:

	virtual JPH::BodyID getBodyID() const = 0;
	virtual void addPhysicsBody() = 0;

	virtual void printInfo(int iterationStep) const = 0;

	virtual void takeDamage(float healthToSubtract, glm::vec3 direction = glm::vec3{ 0 }, float knockbackSpeed = 0.0f) = 0;
	
	virtual float getCurrentHealth() const = 0;

	virtual void handleRotation(float deltaYaw, float deltaPitch) = 0;

	virtual float getMovementSpeed() const = 0;

	virtual const glm::mat4 calculateViewMat() const = 0;
	virtual const glm::mat4 getProjMat() const = 0;

	virtual void setPerspectiveProjection(float fov, float aspect, float near, float far) = 0;

	virtual CharacterCameraSettings getCameraSettings() const = 0;
	
	virtual bool isPhysicsPlayer() const { return false; }

	virtual glm::vec3 getCameraPosition() const = 0;

	virtual glm::vec3 getFront() const = 0;
	virtual glm::vec3 getUp() const = 0;
};