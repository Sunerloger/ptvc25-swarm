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
        glm::vec4 params1{0.0f, 0.1f, 0.1f, 16.0f};  // x: hasTexture, yz: tileScale, w: maxTessLevel
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
            VkPipelineLayout pipelineLayout;
        };

        PipelineInfo& getPipeline(const Material& material);
        void createPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout);

        struct PipelineKey {
            std::string vertShaderPath;
            std::string tessControlShaderPath;
            std::string tessEvalShaderPath;
            std::string fragShaderPath;
            uint32_t patchControlPoints;
            bool depthWriteEnable;
            VkCompareOp depthCompareOp;
            VkCullModeFlags cullMode;

            bool operator==(const PipelineKey& other) const {
                return vertShaderPath == other.vertShaderPath &&
                    tessControlShaderPath == other.tessControlShaderPath &&
                    tessEvalShaderPath == other.tessEvalShaderPath &&
                    fragShaderPath == other.fragShaderPath &&
                    patchControlPoints == other.patchControlPoints &&
                    depthWriteEnable == other.depthWriteEnable &&
                    depthCompareOp == other.depthCompareOp &&
                    cullMode == other.cullMode;
            }
        };

        struct PipelineKeyHash {
            std::size_t operator()(const PipelineKey& key) const {
                // Simple hash function
                std::size_t h1 = std::hash<std::string>{}(key.vertShaderPath);
                std::size_t h2 = std::hash<std::string>{}(key.tessControlShaderPath);
                std::size_t h3 = std::hash<std::string>{}(key.tessEvalShaderPath);
                std::size_t h4 = std::hash<std::string>{}(key.fragShaderPath);
                std::size_t h5 = std::hash<uint32_t>{}(key.patchControlPoints);
                std::size_t h6 = std::hash<bool>{}(key.depthWriteEnable);
                std::size_t h7 = std::hash<int>{}(static_cast<int>(key.depthCompareOp));
                std::size_t h8 = std::hash<int>{}(static_cast<int>(key.cullMode));
                return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ 
                       (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
            }
        };

        Device& device;
        VkRenderPass renderPass;
        VkDescriptorSetLayout globalSetLayout;

        std::unordered_map<PipelineKey, PipelineInfo, PipelineKeyHash> pipelineCache;
        std::unordered_map<VkDescriptorSetLayout, VkPipelineLayout> pipelineLayoutCache;
    };

}  // namespace vk