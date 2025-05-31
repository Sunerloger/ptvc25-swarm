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

		// x = waveFrequency, y = timeScale1, z = timeScale2, w = timeScale3
		glm::vec4 waveParams1 = glm::vec4{ 100.0f, 2.0f, 1.5f, 1.0f };

		// x = waveAmplitude1, y = waveAmplitude2, z = waveAmplitude3, w = diagonal wave frequency
		glm::vec4 waveParams2 = glm::vec4{ 0.3f, 0.2f, 0.1f, 15.0f };

		// xy = textureRepetition, how often the texture repeats across the whole tessellation object
		// zw = uvOffset, scroll UV coordinates per time unit to animate water surface
		glm::vec4 textureParams = glm::vec4(1.0f, 1.0f, 0.03f, 0.05f);

		// x = ka, y = kd, z = ks, w = shininess
		glm::vec4 materialProperties = glm::vec4{ 0.3f, 0.5f, 0.3f, 64.0f };

		// xyz = default color, w = transparency
		glm::vec4 color = glm::vec4{ 0.302f, 0.404f, 0.859f, 0.8f };
		
		// x = hasTexture, yzw = unused
		glm::vec4 flags = glm::vec4{1.0f};
	};

	struct CreateWaterData {
		// max tessellation subdivisions
		float maxTessLevel = 16.0f;

		// within minTessDistance the tessellation has maxTessLevels
		float minTessDistance = 10.0f;

		// tessellation decreases linearly until maxTessDistance(minimum tessellation level, here: no subdivisions)
		float maxTessDistance = 100.0f;

		// multiplication factor for waves
		float heightScale = 5.0f;

		float waveFrequency = 100.0f;

		float timeScaleWaveComponent1 = 2.0f;
		float timeScaleWaveComponent2 = 1.5f;
		float timeScaleWaveComponent3 = 1.0f;

		float amplitudeWaveComponent1 = 0.3f;
		float amplitudeWaveComponent2 = 0.2f;
		float amplitudeWaveComponent3 = 0.1f;

		float diagonalWaveFrequency = 15.0f;

		// how often the texture repeats across the whole tessellation object
		glm::vec2 textureRepetition = glm::vec2{ 1.0f, 1.0f };

		// scroll UV coordinates per time unit to animate water surface
		glm::vec2 uvOffset = glm::vec2{ 0.03f, 0.05f };

		// ambient lighting factor
		float ka = 0.3f;

		// diffuse lighting factor
		float kd = 0.5f;

		// specular lighting factor
		float ks = 0.3f;

		// shininess of specular lighting component
		float shininess = 64.0f;

		// color that is used when no texture is provided
		glm::vec3 defaultColor = glm::vec3{ 0.302f, 0.404f, 0.859f };

		// transparency of material (also used with texture)
		float transparency = 0.8f;
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

		void setWaterData(CreateWaterData createWaterData = CreateWaterData{});

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