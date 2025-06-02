#pragma once

#include "VegetationObject.h"
#include "TreeMaterial.h"
#include "../rendering/materials/StandardMaterial.h"
#include "../vk/vk_device.h"

namespace procedural {

	// Helper class to manage shared resources for vegetation objects
	class VegetationSharedResources {
	   public:
		VegetationSharedResources(vk::Device& device) : device_(device) {
			// Create TreeMaterial with separate bark and leaf materials
			treeMaterial_ = std::make_unique<TreeMaterial>(device);
		}

		TreeMaterial& getTreeMaterial() const {
			return *treeMaterial_;
		}

		// Legacy method for compatibility - returns bark material
		std::shared_ptr<vk::Material> getMaterial() const {
			return treeMaterial_->getBarkMaterial();
		}

	   private:
		vk::Device& device_;
		std::unique_ptr<TreeMaterial> treeMaterial_;
	};

}  // namespace procedural
