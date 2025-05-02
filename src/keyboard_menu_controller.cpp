#include "keyboard_menu_controller.h"
#include <iostream>

namespace controls {

	KeyboardMenuController::KeyboardMenuController(GLFWwindow* window) {
		glfwGetCursorPos(window, &lastCx, &lastCy);
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, &KeyboardMenuController::keyCallback);
	}

	void KeyboardMenuController::keyCallback(GLFWwindow* w, int key, int, int action, int) {
		if (action != GLFW_PRESS)
			return;
		auto* self = static_cast<KeyboardMenuController*>(glfwGetWindowUserPointer(w));
		self->handleKey(w, key);
	}

	void KeyboardMenuController::handleKey(GLFWwindow* window, int key) {
		std::cout << "KeyCallback: " << key << "\n";
		if (key == GLFW_KEY_ESCAPE) {
			menuOpen = !menuOpen;
			if (menuOpen) {
				glfwGetCursorPos(window, &lastCx, &lastCy);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetCursorPos(window, lastCx, lastCy);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			return;
		}

		if (!menuOpen)
			return;

		switch (key) {
			case GLFW_KEY_F1:
				// toggle fullscreen
				if (glfwGetWindowMonitor(window)) {
					glfwSetWindowMonitor(window, nullptr,
						prevX, prevY, prevW, prevH, prevRefresh);
				} else {
					glfwGetWindowPos(window, &prevX, &prevY);
					glfwGetWindowSize(window, &prevW, &prevH);
					prevRefresh = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
					auto vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
					glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(),
						0, 0, vm->width, vm->height, vm->refreshRate);
				}
				break;

			case GLFW_KEY_F2:
				// cycle resolution
				currentRes = (currentRes + 1) % resolutions.size();
				{
					auto [w, h] = resolutions[currentRes];
					glfwSetWindowSize(window, w, h);
					// <-- here you'd signal your renderer to rebuild swapchain
				}
				break;

			case GLFW_KEY_F3:
				// toggle aspect ratio
				use4by3 = !use4by3;
				if (use4by3)
					glfwSetWindowAspectRatio(window, 4, 3);
				else
					glfwSetWindowAspectRatio(window, 16, 9);
				break;

			default:
				break;
		}
	}

}  // namespace controls