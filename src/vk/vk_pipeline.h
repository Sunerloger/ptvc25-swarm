#pragma once

#include "string"
#include "vector"
#include "vk_device.h"

namespace vk {

    struct PipelineConfigInfo {
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;
        
        // Convenience accessors for backward compatibility
        bool& depthWriteEnable = *reinterpret_cast<bool*>(&depthStencilInfo.depthWriteEnable);
        VkCompareOp& depthCompareOp = depthStencilInfo.depthCompareOp;
        VkCullModeFlags& cullMode = rasterizationInfo.cullMode;

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
        uint32_t patchControlPoints = 3;  // Default for triangles
    };

    class Pipeline {
    public:
        Pipeline(Device &device,
                 const std::string& vertFilepath,
                 const std::string& fragFilepath,
                 const PipelineConfigInfo& configInfo);
                 
        // Constructor with tessellation shaders
        Pipeline(Device &device,
                 const std::string& vertFilepath,
                 const std::string& tessControlFilepath,
                 const std::string& tessEvalFilepath,
                 const std::string& fragFilepath,
                 const PipelineConfigInfo& configInfo);
                 
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline &operator=(const Pipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);

        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void tessellationPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t patchControlPoints = 3);


    private:
        static std::vector<char> readFile(const std::string& filepath);

        void createGraphicsPipeline(const std::string& vertFilepath,
                                    const std::string& fragFilepath,
                                    const PipelineConfigInfo& configInfo);
                                    
        void createTessellationPipeline(const std::string& vertFilepath,
                                       const std::string& tessControlFilepath,
                                       const std::string& tessEvalFilepath,
                                       const std::string& fragFilepath,
                                       const PipelineConfigInfo& configInfo);

        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        Device& device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModule tessControlShaderModule = VK_NULL_HANDLE;
        VkShaderModule tessEvalShaderModule = VK_NULL_HANDLE;
    };
}
