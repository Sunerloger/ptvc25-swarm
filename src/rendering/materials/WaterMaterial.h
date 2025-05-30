#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_buffer.h"

#include <glm/glm.hpp>

namespace vk {

	struct WaterData {
		// x = maxTessLevel, max tessellation subdivisions
		// y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
		// z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
		// w = heightScale, multiplication factor for waves
		glm::vec4 tessParams = glm::vec4{ 16.0f, 10.0f, 100.0f, 5.0f };

		// xy = textureRepetition, how often the texture repeats across the whole tessellation object
		// zw = uvOffset, scroll UV coordinates per time unit to animate water surface
		glm::vec4 textureParams = glm::vec4(1.0f, 1.0f, 0.03f, 0.05f);
		
		// x = hasTexture, yzw = unused
		glm::vec4 flags = glm::vec4{1.0f};
	};

	class WaterMaterial : public Material {
	   public:
		WaterMaterial(Device& device, const std::string& texturePath);
		WaterMaterial(Device& device, const std::string& texturePath,
			const std::string& vertShaderPath, const std::string& fragShaderPath);
		// Constructor for embedded textures
		WaterMaterial(Device& device, const std::vector<unsigned char>& imageData,
			int width, int height, int channels);
		// Constructor for embedded textures with custom shaders
		WaterMaterial(Device& device, const std::vector<unsigned char>& imageData,
			int width, int height, int channels,
			const std::string& vertShaderPath, const std::string& fragShaderPath);
		~WaterMaterial() override;

		VkDescriptorSet getDescriptorSet() const override {
			return textureDescriptorSet;
		}
		VkDescriptorSetLayout getDescriptorSetLayout() const override {
			return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
		}

		void setWaterData(glm::vec2 textureRepetition = glm::vec2(1.0f), float maxLevel = 16.0f, float minDistance = 10.0f, float maxDistance = 100.0f, float heightScale = 5.0f, glm::vec2 uvOffset = glm::vec2(0.03f, 0.05f));

		static std::unique_ptr<DescriptorPool> descriptorPool;
		static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		static int instanceCount;

		static void cleanupResources();

	   private:
		void createTextureImage(const std::string& texturePath);
		void createTextureFromImageData(const std::vector<unsigned char>& imageData,
			int width, int height, int channels);
		void createTextureImageView();
		void createTextureSampler();
		void createDescriptorSet();

		static void createDescriptorSetLayoutIfNeeded(Device& device);

		VkImage textureImage = VK_NULL_HANDLE;
		VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
		VkImageView textureImageView = VK_NULL_HANDLE;
		VkSampler textureSampler = VK_NULL_HANDLE;
		VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;

		Buffer paramsBuffer; // UBO

		WaterData waterData;
	};
}