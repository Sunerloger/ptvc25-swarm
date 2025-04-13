#pragma once

#include "vk/vk_window.h"
#include "vk/vk_frame_info.h"

#include "vk/vk_model.h"

namespace controls {
	class KeyboardPlacementController {
	   public:
		KeyboardPlacementController() {
		}

		struct KeyMappings {
			int moveLeft = GLFW_KEY_LEFT;
			int moveRight = GLFW_KEY_RIGHT;
			int moveUp = GLFW_KEY_UP;
			int moveDown = GLFW_KEY_DOWN;
			int moveForward = GLFW_KEY_Q;
			int moveBackward = GLFW_KEY_E;
			int scaleDown = GLFW_KEY_MINUS;
			int scaleUp = GLFW_KEY_EQUAL;
			int rotateUpX = GLFW_KEY_Z;
			int rotateDownX = GLFW_KEY_X;
			int rotateUpY = GLFW_KEY_C;
			int rotateDownY = GLFW_KEY_V;
			int rotateUpZ = GLFW_KEY_B;
			int rotateDownZ = GLFW_KEY_N;
		};

		int updateModelMatrix(GLFWwindow* window);

	   private:
		KeyMappings keys{};
		float step = 0.01f;
	};
}  // namespace controls
