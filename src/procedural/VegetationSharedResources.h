#pragma once

#include "VegetationObject.h"
#include "../rendering/materials/StandardMaterial.h"
#include "../vk/vk_device.h"

namespace procedural {

// Helper class to manage shared resources for vegetation objects
class VegetationSharedResources {
public:
    VegetationSharedResources(vk::Device& device) {
        // Initialize shared material for ferns only
        std::vector<unsigned char> fernPixel = {46, 125, 50, 255}; // Fern green
        
        // Create a single shared material for all ferns
        fernMaterial = std::make_shared<vk::StandardMaterial>(device, fernPixel, 1, 1, 4);
    }
    
    std::shared_ptr<vk::Material> getMaterial(VegetationType type) const {
        return fernMaterial;
    }
    
private:
    std::shared_ptr<vk::Material> fernMaterial;
};

} // namespace procedural
