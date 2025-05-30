#pragma once

#include "VegetationObject.h"
#include "../rendering/materials/StandardMaterial.h"
#include "../vk/vk_device.h"

namespace procedural {

// Helper class to manage shared resources for vegetation objects
class VegetationSharedResources {
public:
    VegetationSharedResources(vk::Device& device) {
        // Initialize shared material for ferns only - using darker, richer green
        std::vector<unsigned char> fernPixel = {34, 139, 34, 255}; // Forest green

        // Create a single shared material for all ferns
        fernMaterial = std::make_shared<vk::StandardMaterial>(device, fernPixel, 1, 1, 4);

        // Disable backface culling for the fern material to see both sides
        fernMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    }

    std::shared_ptr<vk::Material> getMaterial(VegetationType type) const {
        return fernMaterial;
    }

private:
    std::shared_ptr<vk::Material> fernMaterial;
};

} // namespace procedural
