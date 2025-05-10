#include "Material.h"
#include "../../vk/vk_device.h"

namespace vk {

    int Material::s_nextID = 0;

    // Minimal implementation for the base Material class
    Material::Material(Device& device) : device(device) {
        // Initialize the PipelineConfigInfo with default values
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    }
    
    Material::~Material() {}
}