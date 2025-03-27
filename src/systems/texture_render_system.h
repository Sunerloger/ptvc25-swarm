#pragma once

#include "../vk/vk_renderer.h"
#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"
#include <memory>
#include <vector>
#include "glm/glm.hpp"

namespace vk {

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

		Device& device;
		VkPipelineLayout pipelineLayout;
		std::unique_ptr<Pipeline> pipeline;
	};

}  // namespace vk