#include "CharacterCamera.h"

CharacterCamera::CharacterCamera(float fov, float aspectRatio, float nearPlane, float farPlane) {

	// vklCreatePerspectiveProjectionMatrix already flips y axis
	projMatrix = vklCreatePerspectiveProjectionMatrix(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

CharacterCamera::~CharacterCamera() {}

glm::mat4 CharacterCamera::getViewProjMatrix() {
	return projMatrix * viewMatrix;
}

void CharacterCamera::setViewMatrix(RMat44 physicsViewMatrix) {
	this->viewMatrix = physicsViewMatrix;
}
