#include "keyboard_placement_controller.h"

#include <iostream>
#include <fstream>

namespace controls {

	int KeyboardPlacementController::updateModelMatrix(GLFWwindow* window) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
			return keys.moveForward;
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
			return keys.moveBackward;
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
			return keys.moveLeft;
		}
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
			return keys.moveRight;
		}
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
			return keys.moveUp;
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
			return keys.moveDown;
		}
		if (glfwGetKey(window, keys.scaleUp) == GLFW_PRESS) {
			return keys.scaleUp;
		}
		if (glfwGetKey(window, keys.scaleDown) == GLFW_PRESS) {
			return keys.scaleDown;
		}
		if (glfwGetKey(window, keys.rotateUpX) == GLFW_PRESS) {
			return keys.rotateUpX;
		}
		if (glfwGetKey(window, keys.rotateDownX) == GLFW_PRESS) {
			return keys.rotateDownX;
		}
		if (glfwGetKey(window, keys.rotateUpY) == GLFW_PRESS) {
			return keys.rotateUpY;
		}
		if (glfwGetKey(window, keys.rotateDownY) == GLFW_PRESS) {
			return keys.rotateDownY;
		}
		if (glfwGetKey(window, keys.rotateUpZ) == GLFW_PRESS) {
			return keys.rotateUpZ;
		}
		if (glfwGetKey(window, keys.rotateDownZ) == GLFW_PRESS) {
			return keys.rotateDownZ;
		}

		return -1;
	}

}