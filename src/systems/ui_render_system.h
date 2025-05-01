// ui_render_system.h
#pragma once

#include "../vk/vk_renderer.h"
#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"
#include "../vk/vk_device.h"
#include "../vk/vk_model.h"
#include "../vk/vk_buffer.h"

#include "../keyboard_placement_controller.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include <memory>
#include <vector>

namespace vk {

	struct UIPushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
		int hasTexture = 0;
		int usePerspectiveProjection = 0;
	};

	class UIRenderSystem {
	   public:
		UIRenderSystem(Device& device, VkRenderPass renderPass,
			VkDescriptorSetLayout globalSetLayout,
			VkDescriptorSetLayout textureSetLayout);
		~UIRenderSystem();

		void renderGameObjects(FrameInfo& frameInfo, int placementTransform);

	   private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout,
			VkDescriptorSetLayout textureSetLayout);
		void createPipelines(VkRenderPass renderPass);
		void createDefaultTexture(VkDescriptorPool textureDescriptorPool,
			VkDescriptorSetLayout textureSetLayout);

		Device& device;
		VkPipelineLayout pipelineLayout;

		// Three pipelines:
		//   orthoPipeline         = 2D HUD (no depth)
		//   depthPopulatePipeline = writes depth for 3D UI
		//   perspectivePipeline   = color pass for 3D UI (tests against that depth)
		std::unique_ptr<Pipeline> orthoPipeline;
		std::unique_ptr<Pipeline> depthPopulatePipeline;
		std::unique_ptr<Pipeline> perspectivePipeline;

		VkDescriptorSet defaultTextureDescriptorSet = VK_NULL_HANDLE;
		VkImage defaultTextureImage = VK_NULL_HANDLE;
		VkDeviceMemory defaultTextureImageMemory = VK_NULL_HANDLE;
		VkImageView defaultTextureImageView = VK_NULL_HANDLE;
		VkSampler defaultTextureSampler = VK_NULL_HANDLE;
	};

}  // namespace vk