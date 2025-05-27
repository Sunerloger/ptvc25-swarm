#include "CharacterCamera.h"

CharacterCamera::CharacterCamera(CharacterCameraSettings cameraSettings) {
	this->settings = cameraSettings;
}

CharacterCamera::~CharacterCamera() {}

const glm::mat4 CharacterCamera::calculateViewMat() const {
	return glm::lookAt(settings.position, settings.position + this->getFront(), glm::vec3{0.0f, 1.0f, 0.0f});
}

const glm::mat4 CharacterCamera::getProjMat() const {
	return settings.projMatrix;
}

void CharacterCamera::setPhysicsPosition(JPH::Vec3 physicsPosition) {
	for (size_t i = 0; i < 3; i++) {
		settings.position[i] = physicsPosition[i];
	}

	// offset doesn't need to be rotated
	settings.position += settings.cameraOffsetFromCharacter;
}

void CharacterCamera::setPosition(glm::vec3 newPosition) {
	settings.position = newPosition;
}

void CharacterCamera::addRotation(float deltaYaw, float deltaPitch) {
	setYaw(this->settings.yaw + deltaYaw * settings.cameraSpeed);
	setPitch(this->settings.pitch + deltaPitch * settings.cameraSpeed);
}

const glm::vec3 CharacterCamera::getFront() const {
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

	front = glm::rotate(front, glm::radians(settings.pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	front = glm::rotate(front, glm::radians(settings.yaw), glm::vec3(0.0f, 1.0f, 0.0f));

	return glm::normalize(front);
}

const glm::vec3 CharacterCamera::getPosition() const {
	return settings.position;
}

void CharacterCamera::setYaw(float newYaw) {
	settings.yaw = fmod(newYaw, 360.0f);
	if (settings.yaw < 0.0f) {
		settings.yaw += 360.0f;	// ensure yaw is always positive
	}
}

void CharacterCamera::setPitch(float newPitch) {
	settings.pitch = glm::clamp(newPitch, -89.0f, 89.0f);  // Limit pitch to avoid gimbal lock
}

float CharacterCamera::getYaw() const {
	return settings.yaw;
}

void CharacterCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
	glm::mat4 proj = glm::mat4{1.0f};
	proj[0][0] = 2.f / (right - left);
	proj[1][1] = 2.f / (bottom - top);
	proj[2][2] = 1.f / (far - near);
	proj[3][0] = -(right + left) / (right - left);
	proj[3][1] = -(bottom + top) / (bottom - top);
	proj[3][2] = -near / (far - near);

	settings.projMatrix = proj * getVulkanAxisInversionMatrix();
}

void CharacterCamera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFov = tan(fov / 2.f);
	glm::mat4 proj = glm::mat4{0.0f};
	proj[0][0] = 1.f / (aspect * tanHalfFov);
	proj[1][1] = 1.f / (tanHalfFov);
	proj[2][2] = far / (far - near);
	proj[2][3] = 1.f;
	proj[3][2] = -(far * near) / (far - near);

	settings.projMatrix = proj * getVulkanAxisInversionMatrix();
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
	setViewDirection(target - settings.position);
}

CharacterCameraSettings CharacterCamera::getSettings() const {
	return settings;
}