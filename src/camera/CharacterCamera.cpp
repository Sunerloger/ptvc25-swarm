#include "CharacterCamera.h"

CharacterCamera::CharacterCamera(std::unique_ptr<CharacterCameraSettings> cameraSettings) {

	this->settings = std::move(cameraSettings);

	this->yaw = this->settings->initialYaw;
	this->pitch = this->settings->initialPitch;
}

CharacterCamera::~CharacterCamera() {}

const glm::mat4 CharacterCamera::calculateViewMat() const {
	return glm::lookAt(position, position + this->getFront(), glm::vec3{0.0f, 1.0f, 0.0f});
}

const glm::mat4 CharacterCamera::getProjMat() const {
	return projMatrix;
}

void CharacterCamera::setPhysicsPosition(JPH::Vec3 physicsPosition) {
	for (size_t i = 0; i < 3; i++)
	{
		this->position[i] = physicsPosition[i];
	}

	// offset doesn't need to be rotated
	this->position += settings->cameraOffsetFromCharacter;
}

void CharacterCamera::addRotation(float deltaYaw, float deltaPitch, float deltaTime) {
	setYaw(this->yaw + deltaYaw * settings->cameraSpeed);
	setPitch(this->pitch + deltaPitch * settings->cameraSpeed);
}

const glm::vec3 CharacterCamera::getFront() const {
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

	front = glm::rotate(front, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	front = glm::rotate(front, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

	return glm::normalize(front);
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

float CharacterCamera::getYaw() const {
	return yaw;
}

void CharacterCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
	glm::mat4 proj = glm::mat4{ 1.0f };
	proj[0][0] = 2.f / (right - left);
	proj[1][1] = 2.f / (bottom - top);
	proj[2][2] = 1.f / (far - near);
	proj[3][0] = -(right + left) / (right - left);
	proj[3][1] = -(bottom + top) / (bottom - top);
	proj[3][2] = -near / (far - near);

	projMatrix = proj * getVulkanAxisInversionMatrix();
}

glm::mat4 CharacterCamera::getVulkanAxisInversionMatrix() {
	glm::mat4 inversion = glm::mat4(1.0f);
	inversion[1][1] = -1.0f;
	inversion[2][2] = -1.0f;
	return inversion;
}

void CharacterCamera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFov = tan(fov / 2.f);
	glm::mat4 proj = glm::mat4{ 0.0f };
	proj[0][0] = 1.f / (aspect * tanHalfFov);
	proj[1][1] = 1.f / (tanHalfFov);
	proj[2][2] = far / (far - near);
	proj[2][3] = 1.f;
	proj[3][2] = -(far * near) / (far - near);

	projMatrix = proj * getVulkanAxisInversionMatrix();
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