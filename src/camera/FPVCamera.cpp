#include "FPVCamera.h"
#include "VulkanLaunchpad.h"
#include <cmath>

FPVCamera::FPVCamera(float fov, float aspectRatio, float nearPlane, float farPlane) {
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	yaw = 0.0f; // Initial yaw, looking along -Z axis
	pitch = 0.0f; // Initial pitch

	// vklCreatePerspectiveProjectionMatrix already flips y axis
	projMatrix = vklCreatePerspectiveProjectionMatrix(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

FPVCamera::~FPVCamera() {}

void FPVCamera::setYaw(float newYaw) {
	yaw = fmod(newYaw, 360.0f);
	if (yaw < 0.0f) {
		yaw += 360.0f; // ensure yaw is always positive
	}
}

void FPVCamera::setPitch(float newPitch) {
	pitch = glm::clamp(newPitch, -89.0f, 89.0f); // Limit pitch to avoid gimbal lock
}

void FPVCamera::addYaw(float deltaYaw) {
	setYaw(this->yaw + deltaYaw);
}

void FPVCamera::addPitch(float deltaPitch) {
	setPitch(this->pitch + deltaPitch);
}

glm::mat4 FPVCamera::getViewProjMatrix() {
	return projMatrix * this->getViewMatrix();
}

glm::vec3 FPVCamera::getPosition() {
	return position;
}

void FPVCamera::moveForward(float delta) {

	// don't move up or down
	glm::vec3 front = getFront();
	front[1] = 0.0f;

	front = glm::normalize(front);

	position += front * delta;
}

void FPVCamera::moveBackward(float delta) {

	// don't move up or down
	glm::vec3 front = getFront();
	front[1] = 0.0f;

	front = glm::normalize(front);

	position -= front * delta;
}

void FPVCamera::moveLeft(float delta) {
	glm::vec3 right = glm::normalize(glm::cross(getFront(), up));

	position -= right * delta;
}

void FPVCamera::moveRight(float delta) {
	glm::vec3 right = glm::normalize(glm::cross(getFront(), up));

	position += right * delta;
}

glm::vec3 FPVCamera::getFront() {
	glm::vec3 front = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

	front = glm::rotate(front, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	front = glm::rotate(front, glm::radians(yaw), up);

	return front;
}

glm::mat4 FPVCamera::getViewMatrix() {
	return glm::lookAt(position, position + this->getFront(), up);
}

// TODO framerate
