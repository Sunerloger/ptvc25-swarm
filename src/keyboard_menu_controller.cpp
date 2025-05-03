#include "keyboard_menu_controller.h"
#include <iostream>

namespace controls {

	KeyboardMenuController::KeyboardMenuController(GLFWwindow* window) {
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
		std::cout << "[Menu] key=" << key << "\n";

		// ESC: toggle pause/menu + always center mouse
		if (key == GLFW_KEY_ESCAPE) {
			menuOpen = !menuOpen;
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			double cx = w * 0.5, cy = h * 0.5;
			glfwSetCursorPos(window, cx, cy);
			glfwSetInputMode(window, GLFW_CURSOR,
				menuOpen ? GLFW_CURSOR_NORMAL
						 : GLFW_CURSOR_DISABLED);
			return;
		}
		if (!menuOpen)
			return;

		const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());

		switch (key) {
			case GLFW_KEY_F1:
				// toggle fullscreen
				if (glfwGetWindowMonitor(window)) {
					glfwSetWindowMonitor(window, nullptr,
						prevX, prevY, prevW, prevH, prevRefresh);
				} else {
					glfwGetWindowPos(window, &prevX, &prevY);
					glfwGetWindowSize(window, &prevW, &prevH);
					prevRefresh = vm->refreshRate;
					glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(),
						0, 0, vm->width, vm->height, vm->refreshRate);
				}
				break;

			case GLFW_KEY_F2: {	 // 1:1
				int w0, h0;
				glfwGetWindowSize(window, &w0, &h0);
				int sz = std::min(w0, h0);
				glfwSetWindowSize(window, sz, sz);
				break;
			}
			case GLFW_KEY_F3: {	 // 16:9
				int w0, h0;
				glfwGetWindowSize(window, &w0, &h0);
				glfwSetWindowSize(window, int(h0 * 16.0f / 9.0f), h0);
				break;
			}
			case GLFW_KEY_F4: {	 // 21:9
				int w0, h0;
				glfwGetWindowSize(window, &w0, &h0);
				glfwSetWindowSize(window, int(h0 * 21.0f / 9.0f), h0);
				break;
			}
			case GLFW_KEY_F5:  // HD
				glfwSetWindowSize(window, 1280, 720);
				break;
			case GLFW_KEY_F6:  // FullHD
				glfwSetWindowSize(window, 1920, 1080);
				break;
			case GLFW_KEY_F7:  // native max
				glfwSetWindowSize(window, vm->width, vm->height);
				break;

			default:
				return;
		}

		if (configChangeCb)
			configChangeCb();
	}

}  // namespace controls