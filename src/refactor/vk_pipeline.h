//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_PIPELINE_H
#define GCGPROJECT_VK_PIPELINE_H
#pragma once
#include "string"
#include "vector"
#include "vk_device.h"

namespace vk {

    struct PipelineConfigInfo {};

    class Pipeline {
    public:
        Pipeline(Device &device,
                 const std::string& vertFilepath,
                 const std::string& fragFilepath,
                 const PipelineConfigInfo& configInfo);
        ~Pipeline() {}

        Pipeline(const Pipeline&) = delete;
        void operator=(const Pipeline&) = delete;

        static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);


    private:
        static std::vector<char> readFile(const std::string& ilepath);

        void createGraphicsPipeline(const std::string& vertFilepath,
                                    const std::string& fragFilepath,
                                    const PipelineConfigInfo& configInfo);

        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        Device& device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}

#endif //GCGPROJECT_VK_PIPELINE_H
