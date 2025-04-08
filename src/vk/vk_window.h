#ifndef GCGPROJECT_VK_WINDOW_H
#define GCGPROJECT_VK_WINDOW_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

#pragma once

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
			return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		}
		bool wasWindowResized() {
			return framebufferResized;
		}
		void resetWindowResizedFlag() {
			framebufferResized = false;
		}
		GLFWwindow *getGLFWWindow() const {
			return window;
		}
		int getWidth() const {
			return width;
		}
		int getHeight() const {
			return height;
		}

		void getFramebufferSize(int &width, int &height) const {
			glfwGetFramebufferSize(this->window, &width, &height);
		}
		void
		createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

	   private:
		static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool framebufferResized = false;

		std::string windowName;
		GLFWwindow *window;
	};
}

#endif	// GCGPROJECT_VK_WINDOW_H
