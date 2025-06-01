#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_buffer.h"
#include "../../vk/vk_swap_chain.h"

#include <glm/glm.hpp>

namespace vk {

	struct WaterData {
		// x = maxTessLevel, max tessellation subdivisions
		// y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
		// z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
		// w = unused
		glm::vec4 tessParams = glm::vec4{ 16.0f, 50.0f, 500.0f, 0.0f };

		// xy = textureRepetition, how often the texture repeats across the whole tessellation object
		// zw = unused
		glm::vec4 textureParams = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);

		// x = ka, y = kd, z = ks, w = alpha
		glm::vec4 materialProperties = glm::vec4{ 0.2f, 0.1f, 0.4f, 10.0f };

		// xyz = default color, w = transparency
		glm::vec4 color = glm::vec4{ 0.302f, 0.404f, 0.859f, 0.8f };
		
		// x = hasTexture, y = waveCount, zw = unused
		glm::vec4 flags = glm::vec4{1.0f};

		// xy = direction, z = steepness in [0,1], w = wavelength
		glm::vec4 waves[WaterMaterial::MAX_NUM_WAVES];
	};

	struct CreateWaterData {
		// max tessellation subdivisions
		float maxTessLevel = 16.0f;

		// within minTessDistance the tessellation has maxTessLevels
		float minTessDistance = 50.0f;

		// tessellation decreases linearly until maxTessDistance(minimum tessellation level, here: no subdivisions)
		float maxTessDistance = 500.0f;

		// how often the texture repeats across the whole tessellation object
		glm::vec2 textureRepetition = glm::vec2{ 1.0f, 1.0f };

		// ambient lighting factor
		float ka = 0.2f;

		// diffuse lighting factor
		float kd = 0.1f;

		// specular lighting factor
		float ks = 0.4f;

		// shininess of specular lighting component
		float alpha = 10.0f;

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

		VkDescriptorSet getDescriptorSet(int frameIndex) const override {
			return textureDescriptorSets[frameIndex];
		}
		VkDescriptorSetLayout getDescriptorSetLayout() const override {
			return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
		}

		void setWaterData(CreateWaterData createWaterData = CreateWaterData{});
		void setWaves(std::vector<glm::vec4> params);
		void updateDescriptorSet(int frameIndex) override;

		static std::unique_ptr<DescriptorPool> descriptorPool;
		static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		static int instanceCount;

		static void cleanupResources();

		static constexpr size_t MAX_NUM_WAVES = 32;

	private:
		void createTextureImage(const std::string& texturePath);
		void createTextureFromImageData(const std::vector<unsigned char>& imageData,
			int width, int height, int channels);
		void createTextureImageView();
		void createTextureSampler();
		void createDescriptorSets();

		static void createDescriptorSetLayoutIfNeeded(Device& device);

		// image not rewritten during runtime, so only one version
		VkImage textureImage = VK_NULL_HANDLE;
		VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
		VkImageView textureImageView = VK_NULL_HANDLE;
		VkSampler textureSampler = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> textureDescriptorSets{SwapChain::MAX_FRAMES_IN_FLIGHT};

		std::vector<std::unique_ptr<Buffer>> paramsBuffers{SwapChain::MAX_FRAMES_IN_FLIGHT};

		WaterData waterData;
	};
}