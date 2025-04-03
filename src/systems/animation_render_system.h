#pragma once

#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"
#include "../vk/vk_device.h"
#include <memory>
#include <vector>

namespace vk {

	class AnimationRenderSystem {
	   public:
		AnimationRenderSystem(Device& device, VkRenderPass renderPass,
			VkDescriptorSetLayout globalSetLayout,
			VkDescriptorSetLayout textureSetLayout,
			VkDescriptorSetLayout animationSetLayout);
		~AnimationRenderSystem();

		void renderGameObjects(FrameInfo& frameInfo);

	   private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout, VkDescriptorSetLayout animationSetLayout);
		void createPipeline(VkRenderPass renderPass);

		Device& device;
		VkPipelineLayout pipelineLayout;
		std::unique_ptr<Pipeline> pipeline;
	};
}  // namespace vk