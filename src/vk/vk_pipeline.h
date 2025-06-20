#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "vk_device.h"

namespace vk {

    struct PipelineConfigInfo {

        PipelineConfigInfo() = default;
        PipelineConfigInfo(PipelineConfigInfo const&) = default;
        PipelineConfigInfo& operator=(PipelineConfigInfo const&) = default;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;

        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;

        // Shader paths
        std::string vertShaderPath = "texture_shader.vert";
        std::string fragShaderPath = "texture_shader.frag";

        // Tessellation support
        bool useTessellation = false;
        std::string tessControlShaderPath = "";
        std::string tessEvalShaderPath = "";
        VkPipelineTessellationStateCreateInfo tessellationInfo{};
        uint32_t patchControlPoints = 4;

        bool operator==(PipelineConfigInfo const& o) const {
            return vertShaderPath == o.vertShaderPath
                && tessControlShaderPath == o.tessControlShaderPath
                && tessEvalShaderPath == o.tessEvalShaderPath
                && fragShaderPath == o.fragShaderPath
                && patchControlPoints == o.patchControlPoints
                && rasterizationInfo.cullMode == o.rasterizationInfo.cullMode
                && rasterizationInfo.polygonMode == o.rasterizationInfo.polygonMode
                && depthStencilInfo.depthWriteEnable == o.depthStencilInfo.depthWriteEnable
                && depthStencilInfo.depthCompareOp == o.depthStencilInfo.depthCompareOp
                && renderPass == o.renderPass
                && subpass == o.subpass;
        }
    };

    class Pipeline {
    public:
        Pipeline(Device &device,
                 const PipelineConfigInfo& configInfo);
                 
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline &operator=(const Pipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);
        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void defaultTessellationPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t patchControlPoints = 4);

        // modify an existing config
        static void shadowPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void terrainShadowPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:

        void createPipeline(const PipelineConfigInfo& configInfo);

        void createShaderModule(const std::string& filepath, VkShaderModule* shaderModule);

        VkPipelineShaderStageCreateInfo makeStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

        Device& device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModule tessControlShaderModule = VK_NULL_HANDLE;
        VkShaderModule tessEvalShaderModule = VK_NULL_HANDLE;
    };

    struct PipelineInfo {
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout; // store handle to pipeline layout but manage in own map to be able to share layout among pipelines with the same descriptor sets without recreating it
    };
}

namespace std {
    template<>
    struct hash<vk::PipelineConfigInfo> {
        size_t operator()(vk::PipelineConfigInfo const& c) const noexcept {
            size_t h = 0;
            auto hc = [](auto& seed, auto const& v) {
                // hash_combine
                using T = std::decay_t<decltype(v)>;
                seed ^= std::hash<T>()(v)
                    + 0x9e3779b97f4a7c15ULL
                    + (seed << 6) + (seed >> 2);
                };
            hc(h, c.vertShaderPath);
            hc(h, c.tessControlShaderPath);
            hc(h, c.tessEvalShaderPath);
            hc(h, c.fragShaderPath);
            hc(h, c.patchControlPoints);
            hc(h, c.rasterizationInfo.cullMode);
            hc(h, c.rasterizationInfo.polygonMode);
            hc(h, c.depthStencilInfo.depthWriteEnable);
            hc(h, c.depthStencilInfo.depthCompareOp);
            hc(h, (uint64_t)c.renderPass);
            hc(h, c.subpass);
            return h;
        }
    };
}