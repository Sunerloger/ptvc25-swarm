#pragma once

#include "../vk/vk_renderer.h"
#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"
#include "../vk/vk_device.h"
#include "../vk/vk_model.h"
#include "../vk/vk_buffer.h"

#include <memory>
#include <vector>
#include "glm/glm.hpp"

namespace vk {

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	class TextureRenderSystem {
	   public:
		TextureRenderSystem(Device& device, VkRenderPass renderPass,
			VkDescriptorSetLayout globalSetLayout,
			VkDescriptorSetLayout textureSetLayout);
		~TextureRenderSystem();

		void renderGameObjects(FrameInfo& frameInfo);

	   private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout);
		void createPipeline(VkRenderPass renderPass);
		void createDefaultTexture(VkDescriptorPool textureDescriptorPool, VkDescriptorSetLayout textureSetLayout);

		Device& device;
		VkPipelineLayout pipelineLayout;
		std::unique_ptr<Pipeline> pipeline;

		// Default texture descriptor set and its associated resources.
		VkDescriptorSet defaultTextureDescriptorSet = VK_NULL_HANDLE;
		VkImage defaultTextureImage = VK_NULL_HANDLE;
		VkDeviceMemory defaultTextureImageMemory = VK_NULL_HANDLE;
		VkImageView defaultTextureImageView = VK_NULL_HANDLE;
		VkSampler defaultTextureSampler = VK_NULL_HANDLE;
	};

}  // namespace vk
