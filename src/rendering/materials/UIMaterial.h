#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_swap_chain.h"
#include <cstdint>	// Required for uint32_t

namespace vk {

	class UIMaterial : public Material {
	   public:
		UIMaterial(Device& device, const std::string& texturePath);
		UIMaterial(Device& device, const std::string& texturePath,
			const std::string& vertShaderPath, const std::string& fragShaderPath);
		// Constructor for embedded textures
		UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
			int width, int height, int channels);
		// Constructor for embedded textures with custom shaders
		UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
			int width, int height, int channels,
			const std::string& vertShaderPath, const std::string& fragShaderPath);
		~UIMaterial() override;

		DescriptorSet getDescriptorSet(int frameIndex) const override;

		static std::unique_ptr<DescriptorPool> descriptorPool;
		static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		static int instanceCount;

		static void cleanupResources();

	   private:
		void createTextureImage(const std::string& texturePath);
		void createTextureFromImageData(const std::vector<unsigned char>& imageData,
			int width, int height, int channels);
		void createTextureImageView();
		void createTextureSampler();
		void createDescriptorSets();

		static void createDescriptorSetLayoutIfNeeded(Device& device);

		VkImage textureImage = VK_NULL_HANDLE;
		VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
		VkImageView textureImageView = VK_NULL_HANDLE;
		VkSampler textureSampler = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> textureDescriptorSets = std::vector<VkDescriptorSet>(SwapChain::MAX_FRAMES_IN_FLIGHT);
		uint32_t mipLevels = 1;
	};
}