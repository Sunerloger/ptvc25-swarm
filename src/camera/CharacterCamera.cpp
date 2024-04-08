#include "CharacterCamera.h"

CharacterCamera::CharacterCamera(CharacterCameraSettings* cameraSettings) {

	this->settings = cameraSettings;

	// vklCreatePerspectiveProjectionMatrix already flips y axis
	projMatrix = vklCreatePerspectiveProjectionMatrix(glm::radians(cameraSettings->fov), cameraSettings->aspectRatio, cameraSettings->nearPlane, cameraSettings->farPlane);
}

CharacterCamera::~CharacterCamera() {}

glm::mat4 CharacterCamera::getViewProjMatrix() {
	return projMatrix * glm::lookAt(position, position + this->getFront(), glm::vec3{0.0f, 1.0f, 0.0f});
}

void CharacterCamera::setPhysicsPosition(Vec3 physicsPosition) {
	for (size_t i = 0; i < 3; i++)
	{
		this->position[i] = physicsPosition[i];
	}

	// offset doesn't need to be rotated
	this->position += settings->cameraOffsetFromCharacter;
}

void CharacterCamera::addRotation(float deltaYaw, float deltaPitch, float deltaTime) {
	setYaw(this->yaw + deltaYaw * settings->cameraSpeed * deltaTime);
	setPitch(this->pitch + deltaPitch * settings->cameraSpeed * deltaTime);
}

glm::vec3 CharacterCamera::getFront() {
	glm::vec3 front = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

	front = glm::rotate(front, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	front = glm::rotate(front, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

	return front;
}

glm::vec3 CharacterCamera::getPosition() {
	return position;
}

void CharacterCamera::setYaw(float newYaw) {
	yaw = fmod(newYaw, 360.0f);
	if (yaw < 0.0f) {
		yaw += 360.0f; // ensure yaw is always positive
	}
}

void CharacterCamera::setPitch(float newPitch) {
	pitch = glm::clamp(newPitch, -89.0f, 89.0f); // Limit pitch to avoid gimbal lock
}

float CharacterCamera::getYaw() {
	return yaw;
}