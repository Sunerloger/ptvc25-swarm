#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <utility>

namespace controls {

	class KeyboardMenuController {
	   public:
		/// Call once, right after window creation.
		KeyboardMenuController(GLFWwindow* window);

		/// True when “paused”/menu is open.
		bool isMenuOpen() const {
			return menuOpen;
		}

	   private:
		/// GLFW key callback thunk
		static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);

		/// Our real handler
		void handleKey(GLFWwindow* window, int key);

		bool menuOpen = false;
		double lastCx = 0, lastCy = 0;
		int prevX = 0, prevY = 0, prevW = 0, prevH = 0, prevRefresh = 0;

		std::vector<std::pair<int, int>> resolutions = {
			{800, 600}, {1280, 720}, {1920, 1080}};
		size_t currentRes = 0;
		bool use4by3 = false;
	};

}  // namespace controls