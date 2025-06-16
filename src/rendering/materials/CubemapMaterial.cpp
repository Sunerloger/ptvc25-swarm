#include "CubemapMaterial.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../vk/vk_utils.hpp"
#include "../../Engine.h"
#include "stb_image.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace vk {

	std::unique_ptr<DescriptorPool> CubemapMaterial::descriptorPool = nullptr;
	std::unique_ptr<DescriptorSetLayout> CubemapMaterial::descriptorSetLayout = nullptr;
	int CubemapMaterial::instanceCount = 0;

	CubemapMaterial::CubemapMaterial(Device& device, const std::array<std::string, 6>& facePaths)
		: Material(device) {
		// Increment instance count
		instanceCount++;

		createDescriptorSetLayoutIfNeeded(device);
		createCubemapFromFaces(facePaths);
		createCubemapImageView();
		createCubemapSampler();
		createDescriptorSets();

		pipelineConfig.depthStencilInfo.depthWriteEnable = false;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipelineConfig.vertShaderPath = "skybox_shader.vert";
		pipelineConfig.fragShaderPath = "skybox_shader.frag";
	}

	CubemapMaterial::CubemapMaterial(Device& device, const std::string& singleImagePath, bool isHorizontalStrip)
		: Material(device) {
		// Increment instance count
		instanceCount++;

		createDescriptorSetLayoutIfNeeded(device);
		createCubemapFromSingleImage(singleImagePath, isHorizontalStrip);
		createCubemapImageView();
		createCubemapSampler();
		createDescriptorSets();

		pipelineConfig.depthStencilInfo.depthWriteEnable = false;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipelineConfig.vertShaderPath = "skybox_shader.vert";
		pipelineConfig.fragShaderPath = "skybox_shader.frag";
	}

	CubemapMaterial::~CubemapMaterial() {
		auto destructionQueue = Engine::getDestructionQueue();
		
		if (destructionQueue) {
			// schedule resources for safe destruction
			for (int i = 0; i < cubemapDescriptorSets.size(); i++) {
				if (cubemapDescriptorSets[i] != VK_NULL_HANDLE && descriptorPool) {
					destructionQueue->pushDescriptorSet(cubemapDescriptorSets[i], descriptorPool->getPool());
					cubemapDescriptorSets[i] = VK_NULL_HANDLE;
				}
			}
			
			if (cubemapSampler != VK_NULL_HANDLE) {
				destructionQueue->pushSampler(cubemapSampler);
				cubemapSampler = VK_NULL_HANDLE;
			}
			
			if (cubemapImageView != VK_NULL_HANDLE) {
				destructionQueue->pushImageView(cubemapImageView);
				cubemapImageView = VK_NULL_HANDLE;
			}
			
			if (cubemapImage != VK_NULL_HANDLE && cubemapImageMemory != VK_NULL_HANDLE) {
				destructionQueue->pushImage(cubemapImage, cubemapImageMemory);
				cubemapImage = VK_NULL_HANDLE;
				cubemapImageMemory = VK_NULL_HANDLE;
			}
		} else {
			// fallback to immediate destruction if queue is not available
			if (cubemapSampler != VK_NULL_HANDLE) {
				vkDestroySampler(device.device(), cubemapSampler, nullptr);
			}
			
			if (cubemapImageView != VK_NULL_HANDLE) {
				vkDestroyImageView(device.device(), cubemapImageView, nullptr);
			}
			
			if (cubemapImage != VK_NULL_HANDLE) {
				vkDestroyImage(device.device(), cubemapImage, nullptr);
			}
			
			if (cubemapImageMemory != VK_NULL_HANDLE) {
				vkFreeMemory(device.device(), cubemapImageMemory, nullptr);
			}
		}
		
		// Decrement instance count and clean up static resources if this is the last instance
		instanceCount--;
		if (instanceCount == 0) {
			std::cout << "Cleaning up CubemapMaterial static resources" << std::endl;
			cleanupResources();
		}
	}

	void CubemapMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
		if (!descriptorSetLayout) {
			auto layoutBuilder = DescriptorSetLayout::Builder(device)
									 .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

			// Store the layout in a static member to prevent it from being destroyed
			descriptorSetLayout = layoutBuilder.build();

			descriptorPool = DescriptorPool::Builder(device)
								 .setMaxSets(10 * SwapChain::MAX_FRAMES_IN_FLIGHT)
								 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 * SwapChain::MAX_FRAMES_IN_FLIGHT)
								 .build();
		}
	}

	void CubemapMaterial::createCubemapFromFaces(const std::array<std::string, 6>& facePaths) {
		// Load all 6 faces
		int width = 0, height = 0, channels = 0;
		stbi_uc* faceData[6];

		for (int i = 0; i < 6; i++) {
			std::string resolvedPath = AssetLoader::getInstance().resolvePath(facePaths[i]);
			faceData[i] = stbi_load(resolvedPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!faceData[i]) {
				throw std::runtime_error("Failed to load cubemap face: " + resolvedPath);
			}
		}

		VkDeviceSize imageSize = width * height * 4 * 6;  // 4 bytes per pixel (RGBA) * 6 faces
		this->mipLevels = 1; // Cubemap materials don't need mipmaps

		// Create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		device.createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Copy face data to staging buffer
		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);

		VkDeviceSize faceSize = width * height * 4;
		for (int i = 0; i < 6; i++) {
			memcpy(static_cast<char*>(data) + (faceSize * i), faceData[i], faceSize);
			stbi_image_free(faceData[i]);
		}

		vkUnmapMemory(device.device(), stagingBufferMemory);

		// Create the cubemap image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1; // Cubemap materials don't need mipmaps
		imageInfo.arrayLayers = 6;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			cubemapImage,
			cubemapImageMemory);

		// Transition image layout for all 6 faces from UNDEFINED to TRANSFER_DST_OPTIMAL
		VkCommandBuffer initialLayoutCmd = device.beginImmediateCommands();

		VkImageMemoryBarrier initialBarrier{};
		initialBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		initialBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		initialBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		initialBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		initialBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		initialBarrier.image = cubemapImage;
		initialBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		initialBarrier.subresourceRange.baseMipLevel = 0;
		initialBarrier.subresourceRange.levelCount = 1;
		initialBarrier.subresourceRange.baseArrayLayer = 0;
		initialBarrier.subresourceRange.layerCount = 6;	 // All 6 faces
		initialBarrier.srcAccessMask = 0;
		initialBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(
			initialLayoutCmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &initialBarrier);

		device.endImmediateCommands(initialLayoutCmd);

		// Copy buffer to image
		VkCommandBuffer commandBuffer = device.beginImmediateCommands();

		VkBufferImageCopy regions[6];
		for (int i = 0; i < 6; i++) {
			regions[i].bufferOffset = faceSize * i;
			regions[i].bufferRowLength = 0;
			regions[i].bufferImageHeight = 0;
			regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[i].imageSubresource.mipLevel = 0;
			regions[i].imageSubresource.baseArrayLayer = i;
			regions[i].imageSubresource.layerCount = 1;
			regions[i].imageOffset = {0, 0, 0};
			regions[i].imageExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
				1};
		}

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			cubemapImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			6,
			regions);

		device.endImmediateCommands(commandBuffer);

		// Clean up staging buffer
		auto destructionQueue = Engine::getDestructionQueue();
		if (destructionQueue) {
			destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
		} else {
			vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
			vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
		}

		device.transitionImageLayout(cubemapImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);
	}

	void CubemapMaterial::createCubemapFromSingleImage(const std::string& imagePath, bool isHorizontalStrip) {
		// Load the single image containing all 6 faces
		int width = 0, height = 0, channels = 0;
		std::string resolvedPath = AssetLoader::getInstance().resolvePath(imagePath);
		stbi_uc* imageData = stbi_load(resolvedPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!imageData) {
			throw std::runtime_error("Failed to load cubemap image: " + resolvedPath);
		}

		// Determine face dimensions based on the strip orientation
		int faceWidth, faceHeight, faceSize;
		if (isHorizontalStrip) {
			// For horizontal strip: width = 6 * faceWidth, height = faceHeight
			faceWidth = width / 6;
			faceHeight = height;

			// For cubemaps, faces must be square, so take the minimum dimension
			faceSize = std::min(faceWidth, faceHeight);
			faceWidth = faceSize;
			faceHeight = faceSize;

			std::cout << "Creating cubemap from horizontal strip: face size = " << faceSize << std::endl;
		} else {
			// For vertical strip: width = faceWidth, height = 6 * faceHeight
			faceWidth = width;
			faceHeight = height / 6;

			// For cubemaps, faces must be square, so take the minimum dimension
			faceSize = std::min(faceWidth, faceHeight);
			faceWidth = faceSize;
			faceHeight = faceSize;

			std::cout << "Creating cubemap from vertical strip: face size = " << faceSize << std::endl;
		}

		// Create staging buffer for all 6 faces
		VkDeviceSize imageSize = faceWidth * faceHeight * 4 * 6;  // 4 bytes per pixel (RGBA) * 6 faces
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		device.createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Copy face data to staging buffer
		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);

		// Extract each face from the strip and copy to the staging buffer
		VkDeviceSize faceSizeBytes = faceWidth * faceHeight * 4;
		// Define face mapping based on strip orientation
		// Standard cubemap face order: Right(+X), Left(-X), Top(+Y), Bottom(-Y), Front(+Z), Back(-Z)
		int faceMapping[6];
		if (isHorizontalStrip) {
			// Assuming horizontal strip order: Right, Left, Top, Bottom, Back, Front
			int mapping[] = {0, 1, 2, 3, 5, 4};
			memcpy(faceMapping, mapping, sizeof(mapping));
		} else {
			// Adjust for vertical strip order if different, e.g., Right, Left, Front, Back, Top, Bottom
			int mapping[] = {0, 1, 4, 5, 2, 3};
			memcpy(faceMapping, mapping, sizeof(mapping));
		}

		for (int i = 0; i < 6; i++) {
			// Get the position in the strip
			int stripPos = faceMapping[i];

			// Calculate the offset in the source image for this face
			int srcX, srcY;
			if (isHorizontalStrip) {
				srcX = stripPos * faceWidth;
				srcY = 0;
			} else {
				srcX = 0;
				srcY = stripPos * faceHeight;
			}

			// Calculate offsets based on strip orientation
			int xOffset = 0;
			int yOffset = 0;

			if (isHorizontalStrip) {
				// For horizontal strip, the original face width is width/6 and height is height
				int originalFaceWidth = width / 6;
				int originalFaceHeight = height;

				// Center the square face within the original face
				xOffset = (originalFaceWidth - faceSize) / 2;
				yOffset = (originalFaceHeight - faceSize) / 2;
			} else {
				// For vertical strip, the original face width is width and height is height/6
				int originalFaceWidth = width;
				int originalFaceHeight = height / 6;

				// For vertical strips, align to top-left instead of center
				// Adjust these offsets based on your specific image layout
				xOffset = (originalFaceWidth - faceSize) / 2;
				yOffset = 0;  // Start from top (adjust if needed)
			}

			// Copy the face data row by row
			bool flipVertically = false;  // Set to true if faces appear upside down

			for (int y = 0; y < faceHeight; y++) {
				// Calculate source Y position, with optional vertical flip
				int srcYPos;
				if (flipVertically) {
					srcYPos = srcY + (faceHeight - 1 - y) + yOffset;
				} else {
					srcYPos = srcY + y + yOffset;
				}

				for (int x = 0; x < faceWidth; x++) {
					int srcIndex = (srcYPos * width + (srcX + x + xOffset)) * 4;
					int dstIndex = (i * faceSizeBytes) + (y * faceWidth + x) * 4;

					// Copy RGBA values
					memcpy(static_cast<char*>(data) + dstIndex, imageData + srcIndex, 4);
				}
			}
		}

		vkUnmapMemory(device.device(), stagingBufferMemory);
		stbi_image_free(imageData);

		// Create the cubemap image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = faceWidth;
		imageInfo.extent.height = faceHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1; // Cubemap materials don't need mipmaps
		imageInfo.arrayLayers = 6;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			cubemapImage,
			cubemapImageMemory);

		// Transition image layout for all 6 faces from UNDEFINED to TRANSFER_DST_OPTIMAL
		VkCommandBuffer singleImageInitialCmd = device.beginImmediateCommands();

		VkImageMemoryBarrier singleImageBarrier{};
		singleImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		singleImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		singleImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		singleImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		singleImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		singleImageBarrier.image = cubemapImage;
		singleImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		singleImageBarrier.subresourceRange.baseMipLevel = 0;
		singleImageBarrier.subresourceRange.levelCount = 1;
		singleImageBarrier.subresourceRange.baseArrayLayer = 0;
		singleImageBarrier.subresourceRange.layerCount = 6;	 // All 6 faces
		singleImageBarrier.srcAccessMask = 0;
		singleImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(
			singleImageInitialCmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &singleImageBarrier);

		device.endImmediateCommands(singleImageInitialCmd);

		// Copy buffer to image
		VkCommandBuffer commandBuffer = device.beginImmediateCommands();

		VkBufferImageCopy regions[6];
		for (int i = 0; i < 6; i++) {
			regions[i].bufferOffset = faceSizeBytes * i;
			regions[i].bufferRowLength = 0;
			regions[i].bufferImageHeight = 0;
			regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[i].imageSubresource.mipLevel = 0;
			regions[i].imageSubresource.baseArrayLayer = i;
			regions[i].imageSubresource.layerCount = 1;
			regions[i].imageOffset = {0, 0, 0};
			regions[i].imageExtent = {
				static_cast<uint32_t>(faceWidth),
				static_cast<uint32_t>(faceHeight),
				1};
		}

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			cubemapImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			6,
			regions);

		device.endImmediateCommands(commandBuffer);

		// Clean up staging buffer
		auto destructionQueue = Engine::getDestructionQueue();
		if (destructionQueue) {
			destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
		} else {
			vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
			vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
		}

		device.transitionImageLayout(cubemapImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);
	}

	void CubemapMaterial::createCubemapImageView() {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = cubemapImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1; // Cubemap materials don't need mipmaps
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &cubemapImageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create cubemap image view!");
		}
	}

	void CubemapMaterial::createCubemapSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &cubemapSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create cubemap texture sampler!");
		}
	}

	void CubemapMaterial::createDescriptorSets() {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = cubemapImageView;
		imageInfo.sampler = cubemapSampler;

		for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			DescriptorWriter(*descriptorSetLayout, *descriptorPool)
				.writeImage(0, &imageInfo)
				.build(cubemapDescriptorSets[i]);
		}
	}

	void CubemapMaterial::cleanupResources() {
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

	DescriptorSet CubemapMaterial::getDescriptorSet(int frameIndex) const {
		DescriptorSet descriptorSet{};
		descriptorSet.binding = 1;
		descriptorSet.handle = cubemapDescriptorSets[frameIndex];
		descriptorSet.layout = descriptorSetLayout->getDescriptorSetLayout();

		return descriptorSet;
	}
}