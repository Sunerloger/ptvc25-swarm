#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>
#include <unordered_map>

namespace vk {
	class Window {

	public:
		Window(int w, int h, std::string name);
		~Window();

		Window(const Window &) = delete;
		Window &operator=(const Window &) = delete;

		bool shouldClose() {
			return glfwWindowShouldClose(window);
		}

		VkExtent2D getExtent() {
			return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		}

		GLFWwindow* getGLFWWindow() const {
			return window;
		}

		int getWidth() const {
			return width;
		}
		int getHeight() const {
			return height;
		}

		VkSurfaceKHR createWindowSurface(VkInstance instance);

		bool framebufferResized = false;

	private:
		void initWindow();
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
		
		int width;
		int height;

		std::string windowName;
		GLFWwindow* window;

		static std::unordered_map<GLFWwindow*, Window*> windows;
	};
}