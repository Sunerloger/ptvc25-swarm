#pragma once

#include "../../vk/vk_pipeline.h"
#include "../../vk/vk_frame_info.h"
#include "../../vk/vk_device.h"
#include "../../vk/vk_renderer.h"
#include "../../rendering/materials/Material.h"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace vk {

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
		int hasTexture = 0;
	};

	class TextureRenderSystem {
    
    public:
		TextureRenderSystem(Device& device, Renderer& renderer, VkDescriptorSetLayout globalSetLayout);
		~TextureRenderSystem();

		void renderGameObjects(FrameInfo& frameInfo);

    private:
        struct PipelineInfo {
            std::unique_ptr<Pipeline> pipeline;
            VkPipelineLayout pipelineLayout; // store handle to pipeline layout but manage in own map to be able to share layout among pipelines with the same descriptor sets
        };

        PipelineInfo& getPipeline(const Material& material);
        void getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout);

        Device& device;
        Renderer& renderer;
        VkDescriptorSetLayout globalSetLayout;

        std::unordered_map<PipelineConfigInfo, PipelineInfo> pipelineCache;
        std::unordered_map<VkDescriptorSetLayout, VkPipelineLayout> pipelineLayoutCache;
	};

}
