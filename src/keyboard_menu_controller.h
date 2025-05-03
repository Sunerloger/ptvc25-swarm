#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <utility>
#include <functional>

namespace controls {

	class KeyboardMenuController {
	   public:
		KeyboardMenuController(GLFWwindow* window);

		bool isMenuOpen() const {
			return menuOpen;
		}

		void setConfigChangeCallback(std::function<void()> cb) {
			configChangeCb = std::move(cb);
		}

	   private:
		static void keyCallback(GLFWwindow* w, int key, int sc, int action, int mods);
		void handleKey(GLFWwindow* window, int key);

		bool menuOpen = false;
		int prevX = 0, prevY = 0, prevW = 0, prevH = 0, prevRefresh = 0;

		std::function<void()> configChangeCb;
	};

}  // namespace controls