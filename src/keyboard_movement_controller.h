#pragma once

#include "vk/vk_window.h"
#include "vk/vk_frame_info.h"

#include "vk/vk_model.h"

#include <iostream>
#include "simulation/objects/actors/Player.h"
#include "movement_controller_utils.h"

namespace controls {
	class KeyboardMovementController {
	   public:
		KeyboardMovementController(int WIDTH,
			int HEIGHT) {
			lastCursorPosX = WIDTH / 2;
			lastCursorPosY = HEIGHT / 2;
		}

		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int jump = GLFW_KEY_SPACE;
		};

		MovementIntent getMovementIntent(GLFWwindow* window);

		// Mouse input since last frame inherently contains deltaTime
		void handleRotation(GLFWwindow* window,
			physics::Player& player);

		void handleEscMenu(GLFWwindow* window);

		bool escapeMenuOpen = false;

	   private:
		KeyMappings keys{};

		bool firstMouse = true;

		bool escKeyPressedLastFrame = false;
		bool f2KeyPressedLastFrame = false;
		int xPos, yPos, width, height, refreshRate;
		double lastCursorPosX, lastCursorPosY;
	};

}