#include "keyboard_placement_controller.h"

#include <iostream>
#include <fstream>

namespace controls {

	int KeyboardPlacementController::updateModelMatrix(GLFWwindow* window) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
			return keys.moveForward;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -step));
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
			return keys.moveBackward;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, step));
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
			return keys.moveLeft;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-step, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
			return keys.moveRight;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(step, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
			return keys.moveUp;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, step, 0.0f));
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
			return keys.moveDown;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -step, 0.0f));
		}
		if (glfwGetKey(window, keys.scaleUp) == GLFW_PRESS) {
			return keys.scaleUp;
			modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f + step));
		}
		if (glfwGetKey(window, keys.scaleDown) == GLFW_PRESS) {
			return keys.scaleDown;
			modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f - step));
		}
		if (glfwGetKey(window, keys.rotateUpX) == GLFW_PRESS) {
			return keys.rotateUpX;
			modelMatrix = glm::rotate(modelMatrix, step, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.rotateDownX) == GLFW_PRESS) {
			return keys.rotateDownX;
			modelMatrix = glm::rotate(modelMatrix, -step, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.rotateUpY) == GLFW_PRESS) {
			return keys.rotateUpY;
			modelMatrix = glm::rotate(modelMatrix, step, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.rotateDownY) == GLFW_PRESS) {
			return keys.rotateDownY;
			modelMatrix = glm::rotate(modelMatrix, -step, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (glfwGetKey(window, keys.rotateUpZ) == GLFW_PRESS) {
			return keys.rotateUpZ;
			modelMatrix = glm::rotate(modelMatrix, step, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		if (glfwGetKey(window, keys.rotateDownZ) == GLFW_PRESS) {
			return keys.rotateDownZ;
			modelMatrix = glm::rotate(modelMatrix, -step, glm::vec3(0.0f, 0.0f, 1.0f));
		}
	}

}