#include "vk_window.h"
#include "stdexcept"

namespace vk {

	std::unordered_map<GLFWwindow*, Window*> Window::windows;

	Window::Window(int w, int h, std::string name) : width(w), height(h), windowName(name) {
		initWindow();
	}

	Window::~Window() {
		windows.erase(window);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		windows[window] = this;
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		{
			int fbWidth = 0, fbHeight = 0;
			glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
			width = fbWidth;
			height = fbHeight;
		}
	}

	void Window::framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
		auto it = windows.find(glfwWindow);

		if (it == windows.end())
			return;

		it->second->framebufferResized = true;
		it->second->width = width;
		it->second->height = height;
	}

	VkSurfaceKHR Window::createWindowSurface(VkInstance instance) {
		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
		return surface;
	}
}