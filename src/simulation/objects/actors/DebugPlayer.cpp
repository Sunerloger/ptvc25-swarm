#include "DebugPlayer.h"

DebugPlayer::DebugPlayer(CharacterCameraSettings cameraSettings, float movementSpeed)
	: movementSpeed(movementSpeed), camera(cameraSettings) {}

void DebugPlayer::printInfo(int iterationStep) const {
	glm::vec3 pos = camera.getPosition();
	std::cout << "DebugPlayer [" << id << "] : Step " << iterationStep << " : Position = [" << pos.x << "," << pos.y << "," << pos.z << "]" << std::endl;
}

void DebugPlayer::handleRotation(float deltaYaw, float deltaPitch) {
	camera.addRotation(deltaYaw, deltaPitch);
}

void DebugPlayer::handleSpeedChange(float scrollOffset, float changeSpeed) {
	movementSpeed *= 1 + changeSpeed * scrollOffset;
	movementSpeed = std::clamp(movementSpeed, 0.001f, 100000.0f);
}

void DebugPlayer::updatePosition(float dt, glm::vec3 dir) {
	if (glm::length(dir) > 0.0f) {
		// Get the camera's orientation vectors
		glm::vec3 front = camera.getFront();
		front.y = 0.0f;
		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		
		// Transform the input direction to be relative to the camera
		glm::vec3 moveDir = glm::vec3(0.0f);
		moveDir += front * -dir.z;  // Forward/backward
		moveDir += right * dir.x;   // Left/right
		moveDir += up * dir.y;      // Up/down
		
		// Normalize and apply movement
		if (glm::length(moveDir) > 0.0f) {
			moveDir = glm::normalize(moveDir);
			glm::vec3 oldPos = camera.getPosition();
			glm::vec3 newPos = oldPos + dt * movementSpeed * moveDir;
			camera.setPosition(newPos);
		}
	}
}