#pragma once

#include "../rendering/materials/StandardMaterial.h"
#include "../vk/vk_device.h"
#include <memory>
#include <glm/glm.hpp>

namespace procedural {

	// Enhanced material system for realistic trees
	class TreeMaterial {
	   public:
		TreeMaterial(vk::Device& device);
		~TreeMaterial() = default;

		// Create materials with different textures
		void createBarkMaterial(const std::string& barkTexturePath);
		void createLeafMaterial(const std::string& leafTexturePath);

		// Create materials with solid colors (fallback)
		void createBarkMaterialSolid(const glm::vec3& barkColor = glm::vec3(0.4f, 0.2f, 0.1f));	 // Brown
		void createLeafMaterialSolid(const glm::vec3& leafColor = glm::vec3(0.2f, 0.6f, 0.2f));	 // Green

		// Get materials for different tree parts
		std::shared_ptr<vk::Material> getBarkMaterial() const {
			return barkMaterial;
		}
		std::shared_ptr<vk::Material> getLeafMaterial() const {
			return leafMaterial;
		}

		// Material type enumeration for geometry generation
		enum MaterialType {
			BARK = 0,
			LEAF = 1
		};

	   private:
		vk::Device& device;
		std::shared_ptr<vk::Material> barkMaterial;
		std::shared_ptr<vk::Material> leafMaterial;
	};

	// Enhanced geometry structure with material information
	struct TreeGeometry {
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		// Separate geometry by material type
		struct MaterialGeometry {
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
		};

		MaterialGeometry bark;	  // Trunk and branch geometry
		MaterialGeometry leaves;  // Leaf geometry
	};

}  // namespace procedural
