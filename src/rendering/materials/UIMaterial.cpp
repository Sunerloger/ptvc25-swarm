#include "UIMaterial.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../vk/vk_utils.hpp"
#include "../../Engine.h"
#include "stb_image.h"
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace vk {

	std::unique_ptr<DescriptorPool> UIMaterial::descriptorPool = nullptr;
	std::unique_ptr<DescriptorSetLayout> UIMaterial::descriptorSetLayout = nullptr;
	int UIMaterial::instanceCount = 0;

	UIMaterial::UIMaterial(Device& device, const std::string& texturePath)
		: Material(device) {
		// Increment instance count
		instanceCount++;

		createDescriptorSetLayoutIfNeeded(device);
		createTextureImage(texturePath);
		createTextureImageView();
		createTextureSampler();
		createDescriptorSets();

		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.vertShaderPath = "ui_shader.vert";
		pipelineConfig.fragShaderPath = "ui_shader.frag";
	}

	UIMaterial::UIMaterial(Device& device, const std::string& texturePath,
		const std::string& vertShaderPath, const std::string& fragShaderPath) : Material(device) {
		// Increment instance count
		instanceCount++;

		createDescriptorSetLayoutIfNeeded(device);
		createTextureImage(texturePath);
		createTextureImageView();
		createTextureSampler();
		createDescriptorSets();

		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.vertShaderPath = vertShaderPath;
		pipelineConfig.fragShaderPath = fragShaderPath;
	}

	UIMaterial::UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
	   int width, int height, int channels) : Material(device) {
    // Increment instance count
    instanceCount++;

    createDescriptorSetLayoutIfNeeded(device);
    createTextureFromImageData(imageData, width, height, channels);
    createTextureImageView();
    createTextureSampler();
    createDescriptorSets();

    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
    pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
    pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineConfig.vertShaderPath = "ui_shader.vert";
    pipelineConfig.fragShaderPath = "ui_shader.frag";
}

void UIMaterial::createTextureFromImageData(const std::vector<unsigned char>& imageData,
    int width, int height, int channels) {
    
    VkDeviceSize imageSize = width * height * channels;

    // Calculate mip levels
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    device.createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    // Copy pixel data to staging buffer
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    // Create the texture image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    device.createImageWithInfo(
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory);

    // Transition image layout for copy operation
    device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

    // Copy buffer to image
    device.copyBufferToImage(stagingBuffer, textureImage, width, height, 1);

    // Clean up staging buffer
    auto destructionQueue = Engine::getDestructionQueue();
    if (destructionQueue) {
        destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
    } else {
        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
    }

    // Generate mipmaps
    device.generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);
}

UIMaterial::UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
    int width, int height, int channels,
    const std::string& vertShaderPath, const std::string& fragShaderPath) : Material(device) {
    // Increment instance count
    instanceCount++;

    createDescriptorSetLayoutIfNeeded(device);
    createTextureFromImageData(imageData, width, height, channels);
    createTextureImageView();
    createTextureSampler();
    createDescriptorSets();

    pipelineConfig.depthStencilInfo.depthWriteEnable = false;
    pipelineConfig.depthStencilInfo.depthTestEnable = false;
    pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
    pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineConfig.vertShaderPath = vertShaderPath;
    pipelineConfig.fragShaderPath = fragShaderPath;
}

	UIMaterial::~UIMaterial() {
		auto destructionQueue = Engine::getDestructionQueue();
		
		if (destructionQueue) {
			// schedule resources for safe destruction
			for (int i = 0; i < textureDescriptorSets.size(); i++) {
				if (textureDescriptorSets[i] != VK_NULL_HANDLE && descriptorPool) {
					destructionQueue->pushDescriptorSet(textureDescriptorSets[i], descriptorPool->getPool());
					textureDescriptorSets[i] = VK_NULL_HANDLE;
				}
			}
			
			if (textureSampler != VK_NULL_HANDLE) {
				destructionQueue->pushSampler(textureSampler);
				textureSampler = VK_NULL_HANDLE;
			}
			
			if (textureImageView != VK_NULL_HANDLE) {
				destructionQueue->pushImageView(textureImageView);
				textureImageView = VK_NULL_HANDLE;
			}
			
			if (textureImage != VK_NULL_HANDLE && textureImageMemory != VK_NULL_HANDLE) {
				destructionQueue->pushImage(textureImage, textureImageMemory);
				textureImage = VK_NULL_HANDLE;
				textureImageMemory = VK_NULL_HANDLE;
			}
		} else {
			// fallback to immediate destruction if queue is not available
			if (textureSampler != VK_NULL_HANDLE) {
				vkDestroySampler(device.device(), textureSampler, nullptr);
			}
			
			if (textureImageView != VK_NULL_HANDLE) {
				vkDestroyImageView(device.device(), textureImageView, nullptr);
			}
			
			if (textureImage != VK_NULL_HANDLE) {
				vkDestroyImage(device.device(), textureImage, nullptr);
			}
			
			if (textureImageMemory != VK_NULL_HANDLE) {
				vkFreeMemory(device.device(), textureImageMemory, nullptr);
			}
		}
		
		// Decrement instance count and clean up static resources if this is the last instance
		instanceCount--;
		if (instanceCount == 0) {
			std::cout << "Cleaning up UIMaterial static resources" << std::endl;
			cleanupResources();
		}
	}

	void UIMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
		if (!descriptorSetLayout) {
			auto layoutBuilder = DescriptorSetLayout::Builder(device)
									 .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

			// Store the layout in a static member to prevent it from being destroyed
			descriptorSetLayout = layoutBuilder.build();

			descriptorPool = DescriptorPool::Builder(device)
								 .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
								 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
								 .build();
		}
	}

	void UIMaterial::createTextureImage(const std::string& texturePath) {
		// Load texture image
		int texWidth, texHeight, texChannels;
		std::string resolvedPath = AssetLoader::getInstance().resolvePath(texturePath);
		stbi_uc* pixels = stbi_load(resolvedPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("Failed to load texture image: " + resolvedPath);
		}

		// Calculate mip levels
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		// Create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		device.createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Copy pixel data to staging buffer
		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device.device(), stagingBufferMemory);

		// Free the pixel data
		stbi_image_free(pixels);

		// Create the texture image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage,
			textureImageMemory);

		// Transition image layout for copy operation
		device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		// Copy buffer to image
		device.copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight, 1);

		// Clean up staging buffer
		auto destructionQueue = Engine::getDestructionQueue();
		if (destructionQueue) {
			destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
		} else {
			vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
			vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
		}

		// Generate mipmaps
		device.generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
	}

	void UIMaterial::createTextureImageView() {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = textureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture image view!");
		}
	}

	void UIMaterial::createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture sampler!");
		}
	}

	void UIMaterial::createDescriptorSets() {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		for (int i = 0; i < textureDescriptorSets.size(); i++) {
			DescriptorWriter(*descriptorSetLayout, *descriptorPool)
				.writeImage(0, &imageInfo)
				.build(textureDescriptorSets[i]);
		}
	}

	void UIMaterial::cleanupResources() {
		auto destructionQueue = Engine::getDestructionQueue();
		
		if (descriptorPool) {
			if (destructionQueue) {
				destructionQueue->pushDescriptorPool(descriptorPool->getPool());
				descriptorPool.reset();
			} else {
				descriptorPool->resetPool();
				descriptorPool.reset();
			}
		}
		
		if (descriptorSetLayout && descriptorSetLayout->getDescriptorSetLayout() != VK_NULL_HANDLE) {
			if (destructionQueue) {
				VkDescriptorSetLayout layoutHandle = descriptorSetLayout->getDescriptorSetLayout();
				destructionQueue->pushDescriptorSetLayout(layoutHandle);
				descriptorSetLayout.reset();
			} else {
				descriptorSetLayout.reset();
			}
		}
	}
}