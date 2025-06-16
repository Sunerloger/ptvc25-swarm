#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include "../../vk/vk_device.h"
#include "../../vk/vk_pipeline.h"
#include "../../vk/vk_descriptors.h"

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
        virtual PipelineConfigInfo& getPipelineConfigRef() { return pipelineConfig; }
        
        virtual PipelineConfigInfo getPipelineConfig() const { return pipelineConfig; }

        // Descriptor set access
        virtual DescriptorSet getDescriptorSet(int frameIndex) const = 0;
        
        virtual void updateDescriptorSet(int frameIndex) {};

    protected:
        Device& device;
        PipelineConfigInfo pipelineConfig{};  // Initialize in-place
    };
}