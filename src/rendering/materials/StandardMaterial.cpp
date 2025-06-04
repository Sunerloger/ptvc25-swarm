#include "StandardMaterial.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../vk/vk_utils.hpp"

// Include stb_image without the implementation
#include "stb_image.h"

#include <stdexcept>
#include <iostream>

namespace vk {

    // Initialize static members
    std::unique_ptr<DescriptorPool> StandardMaterial::descriptorPool;
    std::unique_ptr<DescriptorSetLayout> StandardMaterial::descriptorSetLayout;
    int StandardMaterial::instanceCount = 0;

    StandardMaterial::StandardMaterial(Device& device, const std::string& texturePath)
        : Material(device) {

        // Increment instance count
        instanceCount++;
        
        createDescriptorSetLayoutIfNeeded(device);
        createTextureImage(texturePath, textureImage, textureImageMemory);
        textureImageView = createTextureImageView(textureImage);
        createTextureSampler(textureSampler);
        createDescriptorSets();

        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "texture_shader.vert";
        pipelineConfig.fragShaderPath = "texture_shader.frag";

        setMaterialData();
    }

    StandardMaterial::StandardMaterial(Device& device, const std::string& texturePath,
                                     const std::string& vertShaderPath, const std::string& fragShaderPath)
        : Material(device) {

        // Increment instance count
        instanceCount++;
        
        createDescriptorSetLayoutIfNeeded(device);
        createTextureImage(texturePath, textureImage, textureImageMemory);
        textureImageView = createTextureImageView(textureImage);
        createTextureSampler(textureSampler);
        createDescriptorSets();

        // Set custom pipeline configuration
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;

        setMaterialData();
    }

    StandardMaterial::StandardMaterial(Device& device, const std::vector<unsigned char>& imageData,
                                     int width, int height, int channels)
        : Material(device) {

        // Increment instance count
        instanceCount++;
        
        createDescriptorSetLayoutIfNeeded(device);
        
        // Create texture directly from image data
        createTextureFromImageData(imageData, width, height, channels, textureImage, textureImageMemory);
        textureImageView = createTextureImageView(textureImage);
        createTextureSampler(textureSampler);
        createDescriptorSets();

        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "texture_shader.vert";
        pipelineConfig.fragShaderPath = "texture_shader.frag";

        setMaterialData();
    }

    StandardMaterial::StandardMaterial(Device& device, const std::vector<unsigned char>& imageData,
                                     int width, int height, int channels,
                                     const std::string& vertShaderPath, const std::string& fragShaderPath)
        : Material(device) {

        // Increment instance count
        instanceCount++;
        
        createDescriptorSetLayoutIfNeeded(device);
        
        // Create texture directly from image data
        createTextureFromImageData(imageData, width, height, channels, textureImage, textureImageMemory);
        textureImageView = createTextureImageView(textureImage);
        createTextureSampler(textureSampler);
        createDescriptorSets();

        // Set custom pipeline configuration
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;

        setMaterialData();
    }

    StandardMaterial::~StandardMaterial() {
        auto* destructionQueue = Engine::getDestructionQueue();
        if (destructionQueue) {
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
            
            for (auto& buffer : paramsBuffers) {
                if (buffer) {
                    buffer->scheduleDestroy(*destructionQueue);
                }
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
        
        // decrement instance count and clean up static resources if this is the last instance
        instanceCount--;
        if (instanceCount == 0) {
            std::cout << "Cleaning up StandardMaterial static resources" << std::endl;
            cleanupResources();
        }
    }

    void StandardMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
        if (!descriptorSetLayout) {
            auto layoutBuilder = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            // Store the layout in a static member to prevent it from being destroyed
            descriptorSetLayout = layoutBuilder.build();
            
            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(200 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        }
    }

    void StandardMaterial::createTextureImage(const std::string& texturePath, VkImage& image, VkDeviceMemory& imageMemory) {
        // Load texture image
        int width, height, channels;
        std::string resolvedPath = AssetLoader::getInstance().resolvePath(texturePath);
        stbi_uc* imageData = stbi_load(resolvedPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        
        if (!imageData) {
            throw std::runtime_error("Failed to load texture image: " + resolvedPath);
        }

        VkDeviceSize imageSize = width * height * 4; // 4 bytes per pixel (RGBA)

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        device.createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        // Copy image data to staging buffer
        void* data;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        
        // Simple copy without rotation - we'll fix the UV coordinates instead
        memcpy(data, imageData, static_cast<size_t>(imageSize));
        
        vkUnmapMemory(device.device(), stagingBufferMemory);

        // Free the image data as it's now in the staging buffer
        stbi_image_free(imageData);

        // Create the texture image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory
        );

        // Transition image layout and copy data
        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        // Copy buffer to image
        device.copyBufferToImage(
            stagingBuffer,
            image,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1
        );

        // Transition to shader read layout
        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        auto* destructionQueue = Engine::getDestructionQueue();
        if (destructionQueue) {
            destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
        } else {
            vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
            vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        }
    }

    void StandardMaterial::createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                                    int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory) {
        // Calculate image size (always use RGBA format)
        VkDeviceSize imageSize = width * height * 4;

        if (imageData.empty()) {
            throw std::runtime_error("Empty image data provided");
        }

        std::cout << "Creating texture from image data: " << width << "x" << height
                  << " with " << channels << " channels, size: " << imageData.size() << " bytes" << std::endl;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device.createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        // Copy pixel data to staging buffer
        void* data;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        
        // Copy image data to buffer, converting to RGBA if needed
        unsigned char* dst = static_cast<unsigned char*>(data);
        
        if (channels == 4) {
            // Direct copy for RGBA data
            for (size_t i = 0; i < imageSize && i < imageData.size(); i++) {
                dst[i] = imageData[i];
            }
        } else if (channels == 3) {
            // Convert RGB to RGBA
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    size_t srcIdx = (y * width + x) * 3;
                    size_t dstIdx = (y * width + x) * 4;
                    
                    if (srcIdx + 2 < imageData.size()) {
                        dst[dstIdx] = imageData[srcIdx];       // R
                        dst[dstIdx + 1] = imageData[srcIdx + 1]; // G
                        dst[dstIdx + 2] = imageData[srcIdx + 2]; // B
                        dst[dstIdx + 3] = 255;                   // A (opaque)
                    }
                }
            }
        } else {
            // Handle other formats if needed
            throw std::runtime_error("Unsupported image format with " + std::to_string(channels) + " channels");
        }
        
        vkUnmapMemory(device.device(), stagingBufferMemory);

        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory
        );

        // Transition image layout and copy buffer to image
        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        device.copyBufferToImage(
            stagingBuffer,
            image,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1
        );

        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        auto* destructionQueue = Engine::getDestructionQueue();
        if (destructionQueue) {
            destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
        } else {
            vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
            vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        }
    }

    VkImageView StandardMaterial::createTextureImageView(VkImage& image) {
        return device.createImageView(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }

    void StandardMaterial::createTextureSampler(VkSampler& sampler) {
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
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void StandardMaterial::createDescriptorSets() {

        for (int i = 0; i < paramsBuffers.size(); i++) {
            paramsBuffers[i] = std::make_unique<Buffer>(device,
                sizeof(MaterialData),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            paramsBuffers[i]->map();
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        for (int i = 0; i < textureDescriptorSets.size(); i++) {
            auto bufferInfo = paramsBuffers[i]->descriptorInfo();
            DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeImage(0, &imageInfo)
                .writeBuffer(1, &bufferInfo)
                .build(textureDescriptorSets[i]);
        }
    }

    void StandardMaterial::updateDescriptorSet(int frameIndex) {
        paramsBuffers[frameIndex]->writeToBuffer(&materialData);
        paramsBuffers[frameIndex]->flush();
    }

    void StandardMaterial::setMaterialData(MaterialCreationData creationData) {
        materialData.lightingProperties = glm::vec4{creationData.ka, creationData.kd, creationData.ks, creationData.shininess};
        materialData.flags.x = textureImage != VK_NULL_HANDLE ? 1 : 0;
    }

    void StandardMaterial::cleanupResources() {
    	std::cout << "StandardMaterial: Starting static resource cleanup" << std::endl;
    	
    	auto* destructionQueue = Engine::getDestructionQueue();
    	
    	if (descriptorPool) {
    		if (destructionQueue) {
    			// Get the pool handle before resetting the unique_ptr
    			VkDescriptorPool poolHandle = descriptorPool->getPool();
    			
    			// Don't reset the unique_ptr until after all descriptor sets have been freed
    			// This prevents a double-free situation where the destructor tries to destroy
    			// the pool that's already scheduled for destruction
    			destructionQueue->pushDescriptorPool(poolHandle);
    			
    			// Now it's safe to reset the unique_ptr
    			descriptorPool.reset();
    			std::cout << "StandardMaterial: Descriptor pool scheduled for destruction" << std::endl;
    		} else {
    			// fallback to immediate destruction if queue is not available
    			descriptorPool->resetPool();
    			descriptorPool.reset();
    			std::cout << "StandardMaterial: Descriptor pool destroyed immediately" << std::endl;
    		}
    	}
    	
    	if (descriptorSetLayout && descriptorSetLayout->getDescriptorSetLayout() != VK_NULL_HANDLE) {
    		if (destructionQueue) {
    			VkDescriptorSetLayout layoutHandle = descriptorSetLayout->getDescriptorSetLayout();
    			destructionQueue->pushDescriptorSetLayout(layoutHandle);
    			
    			descriptorSetLayout.reset();
    			std::cout << "StandardMaterial: Descriptor set layout scheduled for destruction" << std::endl;
    		} else {
    			// fallback to immediate destruction if queue is not available
    			descriptorSetLayout.reset();
    			std::cout << "StandardMaterial: Descriptor set layout destroyed immediately" << std::endl;
    		}
    	}
    	
    	std::cout << "StandardMaterial: Static resource cleanup complete" << std::endl;
    }
}