#pragma once

#include "vk_window.h"

// std lib headers
#include <string>
#include <vector>
#include <cstdint>	// Required for uint32_t

namespace vk {

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() {
			return graphicsFamilyHasValue && presentFamilyHasValue;
		}
	};

	class Device {
	   public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		Device(Window &window);
		~Device();

		// Not copyable or movable
		Device(const Device &) = delete;
		Device &operator=(const Device &) = delete;
		Device(Device &&) = delete;
		Device &operator=(Device &&) = delete;

		VkCommandPool getCommandPool() {
			return commandPool;
		}
		VkDevice device() {
			return m_device;
		}
		VkPhysicalDevice physicalDevice() {
			return physicalDevice_;
		}
		VkSurfaceKHR surface() {
			return m_surface;
		}
		VkQueue graphicsQueue() {
			return m_graphicsQueue;
		}
		VkQueue presentQueue() {
			return m_presentQueue;
		}
		
		Window& getWindow() {
			return window;
		}

		SwapChainSupportDetails getSwapChainSupport() {
			return querySwapChainSupport(physicalDevice_);
		}
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices findPhysicalQueueFamilies() {
			return findQueueFamilies(physicalDevice_);
		}
		VkFormat findSupportedFormat(
			const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		VkCommandBuffer beginImmediateCommands();
		void endImmediateCommands(VkCommandBuffer commandBuffer);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount = 1, uint32_t baseArrayLayer = 0);	 // Overloaded version
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
	
		// Mipmapping
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount = 1);

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);	 // Add copyBuffer declaration

		VkPhysicalDeviceProperties properties;	// Add properties member

		void createCommandPool(VkCommandPool& out_pool);

	   private:
		bool hasStencilComponent(VkFormat format);	// Add declaration for hasStencilComponent
		VkSampler createTextureSampler();			// Add declaration for createTextureSampler

		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createImmediateCommandPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char *> getRequiredExtensions();
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
		Window &window;
		VkCommandPool commandPool;

		VkDevice m_device;
		VkSurfaceKHR m_surface;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;

		const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
		std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	};

}  // namespace vk