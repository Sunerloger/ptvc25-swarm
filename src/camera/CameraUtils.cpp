#include "CameraUtils.h"


glm::mat4 getOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
	glm::mat4 proj = glm::mat4{ 1.0f };
	proj[0][0] = 2.f / (right - left);
	proj[1][1] = 2.f / (bottom - top);
	proj[2][2] = 1.f / (far - near);
	proj[3][0] = -(right + left) / (right - left);
	proj[3][1] = -(bottom + top) / (bottom - top);
	proj[3][2] = -near / (far - near);

	return proj * getVulkanAxisInversionMatrix();
	// TODO: something is off here - correct matrix: glm::ortho(left, right, bottom, top, near, far) * getVulkanAxisInversionMatrix() - but then the ui doesn't work
}

glm::mat4 getPerspectiveProjection(float fov, float aspect, float near, float far) {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFov = tan(fov / 2.f);
	glm::mat4 proj = glm::mat4{ 0.0f };
	proj[0][0] = 1.f / (aspect * tanHalfFov);
	proj[1][1] = 1.f / (tanHalfFov);
	proj[2][2] = far / (far - near);
	proj[2][3] = 1.f;
	proj[3][2] = -(far * near) / (far - near);

	return proj * getVulkanAxisInversionMatrix();
}

glm::mat4 getVulkanAxisInversionMatrix() {
	glm::mat4 inversion = glm::mat4(1.0f);
	inversion[1][1] = -1.0f;
	inversion[2][2] = -1.0f;
	return inversion;
}