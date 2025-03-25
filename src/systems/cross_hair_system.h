#pragma once

#include "../vk/vk_device.h"
#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"

#include <memory>
#include <vector>

namespace vk {
	class CrossHairSystem {
	   public:
		CrossHairSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~CrossHairSystem();

		CrossHairSystem(const CrossHairSystem&) = delete;
		CrossHairSystem& operator=(const CrossHairSystem&) = delete;

		void renderGameObjects(FrameInfo& frameInfo);

	   private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		Device& device;
		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout;
	};
}