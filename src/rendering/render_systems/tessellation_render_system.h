#pragma once

#include "../../vk/vk_pipeline.h"
#include "../../vk/vk_frame_info.h"
#include "../../vk/vk_device.h"
#include "../../rendering/materials/Material.h"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace vk {

    struct TessellationPushConstantData {
        glm::mat4 modelMatrix{1.0f};
        glm::mat4 normalMatrix{1.0f};
        glm::vec4 params1{0.0f, 0.1f, 0.1f, 16.0f};  // x: hasTexture, yz: textureRepetition, w: maxTessLevel
        glm::vec4 params2{20.0f, 100.0f, 1.0f, 0.0f}; // x: tessDistance, y: minTessDistance, z: heightScale, w: useHeightmapTexture
    };

    class TessellationRenderSystem {
    
    public:
        TessellationRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~TessellationRenderSystem();

        void renderGameObjects(FrameInfo& frameInfo);

    private:
        struct PipelineInfo {
            std::unique_ptr<Pipeline> pipeline;
            VkPipelineLayout pipelineLayout; // store handle to pipeline layout but manage in own map to be able to share layout among pipelines with the same descriptor sets
        };

        PipelineInfo& getPipeline(const Material& material);
        void getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout);

        Device& device;
        VkRenderPass renderPass;
        VkDescriptorSetLayout globalSetLayout;

        std::unordered_map<PipelineConfigInfo, PipelineInfo> pipelineCache;
        std::unordered_map<VkDescriptorSetLayout, VkPipelineLayout> pipelineLayoutCache;
    };

}