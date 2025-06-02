#include "TreeMaterial.h"
#include <iostream>

namespace procedural {

	TreeMaterial::TreeMaterial(vk::Device& device) : device(device) {
		// Try to load textures, fallback to solid colors if textures don't exist
		try {
			createBarkMaterial("textures:ground/dirt.png");	 // Use dirt texture for bark
			std::cout << "Loaded bark texture successfully" << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Failed to load bark texture, using solid color: " << e.what() << std::endl;
			createBarkMaterialSolid();
		}

		try {
			createLeafMaterial("textures:ground/dirt.png");	 // Use dirt texture temporarily for leaves until we get proper leaf texture
			std::cout << "Loaded leaf texture successfully" << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Failed to load leaf texture, using solid color: " << e.what() << std::endl;
			createLeafMaterialSolid();
		}
	}

	void TreeMaterial::createBarkMaterial(const std::string& barkTexturePath) {
		try {
			barkMaterial = std::make_shared<vk::StandardMaterial>(device, barkTexturePath);

			// Configure material for bark - enable backface culling for performance
			barkMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

			std::cout << "Created bark texture material from: " << barkTexturePath << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Failed to load bark texture: " << e.what() << std::endl;
			// Fallback to solid color
			createBarkMaterialSolid();
		}
	}

	void TreeMaterial::createLeafMaterial(const std::string& leafTexturePath) {
		try {
			leafMaterial = std::make_shared<vk::StandardMaterial>(device, leafTexturePath);

			// Configure material for leaves - disable backface culling to see both sides
			leafMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

			std::cout << "Created leaf texture material from: " << leafTexturePath << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Failed to load leaf texture: " << e.what() << std::endl;
			// Fallback to solid color
			createLeafMaterialSolid();
		}
	}

	void TreeMaterial::createBarkMaterialSolid(const glm::vec3& barkColor) {
		// Create a 1x1 bark color texture
		std::vector<unsigned char> barkPixel = {
			static_cast<unsigned char>(barkColor.r * 255),
			static_cast<unsigned char>(barkColor.g * 255),
			static_cast<unsigned char>(barkColor.b * 255),
			255	 // Alpha
		};

		barkMaterial = std::make_shared<vk::StandardMaterial>(device, barkPixel, 1, 1, 4);
		barkMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	}

	void TreeMaterial::createLeafMaterialSolid(const glm::vec3& leafColor) {
		// Create a 1x1 leaf color texture
		std::vector<unsigned char> leafPixel = {
			static_cast<unsigned char>(leafColor.r * 255),
			static_cast<unsigned char>(leafColor.g * 255),
			static_cast<unsigned char>(leafColor.b * 255),
			255	 // Alpha
		};

		leafMaterial = std::make_shared<vk::StandardMaterial>(device, leafPixel, 1, 1, 4);
		leafMaterial->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	}

}  // namespace procedural
