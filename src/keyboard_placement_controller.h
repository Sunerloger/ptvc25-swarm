#pragma once

#include "vk/vk_window.h"
#include "vk/vk_frame_info.h"
#include "vk/vk_model.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

namespace controls {
	struct PlacementKeyMappings {
		static constexpr int moveLeft = GLFW_KEY_LEFT;
		static constexpr int moveRight = GLFW_KEY_RIGHT;
		static constexpr int moveUp = GLFW_KEY_UP;
		static constexpr int moveDown = GLFW_KEY_DOWN;
		static constexpr int moveForward = GLFW_KEY_COMMA;
		static constexpr int moveBackward = GLFW_KEY_PERIOD;
		static constexpr int scaleDown = GLFW_KEY_MINUS;
		static constexpr int scaleUp = GLFW_KEY_EQUAL;
		static constexpr int rotateUpX = GLFW_KEY_Z;
		static constexpr int rotateDownX = GLFW_KEY_X;
		static constexpr int rotateUpY = GLFW_KEY_C;
		static constexpr int rotateDownY = GLFW_KEY_V;
		static constexpr int rotateUpZ = GLFW_KEY_B;
		static constexpr int rotateDownZ = GLFW_KEY_N;
	};

	class KeyboardPlacementController {
	   public:
		KeyboardPlacementController() {
		}

		int updateModelMatrix(GLFWwindow* window);

	   private:
		PlacementKeyMappings keys{};
		float step = 0.01f;
	};
}  // namespace controls
