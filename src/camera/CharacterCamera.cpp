#include "CharacterCamera.h"

CharacterCamera::CharacterCamera(CharacterCameraSettings* cameraSettings) {

	this->settings = cameraSettings;

	this->yaw = cameraSettings->initialYaw;
	this->pitch = cameraSettings->initialPitch;
}

CharacterCamera::~CharacterCamera() {}

const glm::mat4 CharacterCamera::calculateViewMat() const {
	return glm::lookAt(position, position + this->getFront(), glm::vec3{0.0f, 1.0f, 0.0f});
}

const glm::mat4 CharacterCamera::getProjMat() const {
	return projMatrix;
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

const glm::vec3 CharacterCamera::getFront() const {
	glm::vec3 front = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

	front = glm::rotate(front, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	front = glm::rotate(front, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

	return front;
}

const glm::vec3 CharacterCamera::getPosition() const {
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

void CharacterCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
	projMatrix = glm::mat4{ 1.0f };
	projMatrix[0][0] = 2.f / (right - left);
	projMatrix[1][1] = 2.f / (bottom - top);
	projMatrix[2][2] = 1.f / (far - near);
	projMatrix[3][0] = -(right + left) / (right - left);
	projMatrix[3][1] = -(bottom + top) / (bottom - top);
	projMatrix[3][2] = -near / (far - near);
}

void CharacterCamera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFov = tan(fov / 2.f);
	projMatrix = glm::mat4{ 0.0f };
	projMatrix[0][0] = 1.f / (aspect * tanHalfFov);
	projMatrix[1][1] = -1.f / (tanHalfFov);
	projMatrix[2][2] = far / (far - near);
	projMatrix[2][3] = 1.f;
	projMatrix[3][2] = -(far * near) / (far - near);
}

void CharacterCamera::setViewDirection(glm::vec3 direction) {
	
	direction = glm::normalize(direction);

	// normalization not necessary because tan is a ratio
	float localYaw = atan2(direction.x, -direction.z);
	float localPitch = glm::asin(direction.y);

	setYaw(glm::degrees(localYaw));
	setPitch(glm::degrees(localPitch));
}

void CharacterCamera::setViewTarget(glm::vec3 target) {
	setViewDirection(target - position);
}