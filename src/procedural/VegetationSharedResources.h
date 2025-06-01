#pragma once

#include "VegetationObject.h"
#include "../rendering/materials/StandardMaterial.h"
#include "../vk/vk_device.h"

namespace procedural {

	// Helper class to manage shared resources for vegetation objects
	class VegetationSharedResources {
	   public:
		VegetationSharedResources(vk::Device& device) {
			// Initialize shared material for trees only - using darker, richer green
			std::vector<unsigned char> treePixel = {34, 139, 34, 255};	// Forest green

			// Create a single shared material for all trees
			treeMaterial = std::make_shared<vk::StandardMaterial>(device, treePixel, 1, 1, 4);

			// Disable backface culling for the tree material to see both sides
			treeMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		}

		std::shared_ptr<vk::Material> getMaterial(VegetationType type) const {
			return treeMaterial;
		}

	   private:
		std::shared_ptr<vk::Material> treeMaterial;
	};

}  // namespace procedural
