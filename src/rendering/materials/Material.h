#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include "../../vk/vk_device.h"
#include "../../vk/vk_pipeline.h"

namespace vk {

    // Material interface
    class Material {
    public:
        Material(Device& device);
        virtual ~Material();

        Material(const Material&) = delete;
        void operator=(const Material&) = delete;

        // Pipeline configuration
        // Non-const version for modifying the pipeline config
        virtual PipelineConfigInfo& getPipelineConfig() { return pipelineConfig; }
        
        // Const version for reading the pipeline config
        virtual const PipelineConfigInfo& getPipelineConfig() const { return pipelineConfig; }

        // Descriptor set access
        virtual VkDescriptorSet getDescriptorSet() const = 0;
        virtual VkDescriptorSetLayout getDescriptorSetLayout() const = 0;

        virtual int getID() const = 0;

        static int s_nextID;

    protected:
        Device& device;
        PipelineConfigInfo pipelineConfig{};  // Initialize in-place
    };
}