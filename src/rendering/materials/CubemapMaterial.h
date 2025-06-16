#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_swap_chain.h"
#include <array>
#include <cstdint>

namespace vk {

	class CubemapMaterial : public Material {
	   public:
		CubemapMaterial(Device& device, const std::array<std::string, 6>& facePaths);
		CubemapMaterial(Device& device, const std::string& singleImagePath, bool isHorizontalStrip = true);

		~CubemapMaterial() override;

		DescriptorSet getDescriptorSet(int frameIndex) const override;

		static std::unique_ptr<DescriptorPool> descriptorPool;
		static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		static int instanceCount;

		static void cleanupResources();

	   private:
		void createCubemapFromFaces(const std::array<std::string, 6>& facePaths);
		void createCubemapFromSingleImage(const std::string& imagePath, bool isHorizontalStrip);
		void createCubemapImageView();
		void createCubemapSampler();
		void createDescriptorSets();

		static void createDescriptorSetLayoutIfNeeded(Device& device);

		VkImage cubemapImage = VK_NULL_HANDLE;
		VkDeviceMemory cubemapImageMemory = VK_NULL_HANDLE;
		VkImageView cubemapImageView = VK_NULL_HANDLE;
		VkSampler cubemapSampler = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> cubemapDescriptorSets{SwapChain::MAX_FRAMES_IN_FLIGHT};
		uint32_t mipLevels = 1;
	};
}