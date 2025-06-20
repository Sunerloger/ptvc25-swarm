#include "WaterMaterial.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../vk/vk_utils.hpp"
#include "../../Engine.h"

#include <stdexcept>
#include <iostream>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <algorithm>

namespace vk {

	std::unique_ptr<DescriptorPool> WaterMaterial::descriptorPool = nullptr;
	std::unique_ptr<DescriptorSetLayout> WaterMaterial::descriptorSetLayout = nullptr;
	int WaterMaterial::instanceCount = 0;

    WaterMaterial::WaterMaterial(Device& device, const std::string& texturePath)
        : Material(device) {
        // Increment instance count
        instanceCount++;

        createDescriptorSetLayoutIfNeeded(device);
        createTextureImage(texturePath, textureImage, textureImageMemory);
        textureImageView = createImageView(textureImage);
        createTextureSampler(textureSampler);

        createDescriptorSets();

        Pipeline::defaultTessellationPipelineConfigInfo(pipelineConfig, 4);
        
        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "water_shader.vert";
        pipelineConfig.fragShaderPath = "water_shader.frag";
        pipelineConfig.tessControlShaderPath = "water_shader.tesc";
        pipelineConfig.tessEvalShaderPath = "water_shader.tese";
        pipelineConfig.depthStencilInfo = {};
        pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
        // Enable alpha blending for transparency
        pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
        pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.attributeDescriptions.clear();

        setWaterData();
    }

    WaterMaterial::WaterMaterial(Device& device, const std::string& texturePath,
                               const std::string& vertShaderPath, const std::string& fragShaderPath)
        : Material(device) {
        // Increment instance count
        instanceCount++;

        createDescriptorSetLayoutIfNeeded(device);
        createTextureImage(texturePath, textureImage, textureImageMemory);
        textureImageView = createImageView(textureImage);
        createTextureSampler(textureSampler);

        createDescriptorSets();

        Pipeline::defaultTessellationPipelineConfigInfo(pipelineConfig, 4);

        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "water_shader.vert";
        pipelineConfig.fragShaderPath = "water_shader.frag";
        pipelineConfig.tessControlShaderPath = "water_shader.tesc";
        pipelineConfig.tessEvalShaderPath = "water_shader.tese";
        pipelineConfig.depthStencilInfo = {};
        pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
        // Enable alpha blending for transparency
        pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
        pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.attributeDescriptions.clear();

        setWaterData();
    }

    WaterMaterial::WaterMaterial(Device& device, const std::vector<unsigned char>& imageData,
                               int width, int height, int channels)
        : Material(device) {
        // Increment instance count
        instanceCount++;

        createDescriptorSetLayoutIfNeeded(device);

        // Create texture directly from image data
        createTextureFromImageData(imageData, width, height, channels, textureImage, textureImageMemory);
        textureImageView = createImageView(textureImage);
        createTextureSampler(textureSampler);

        createDescriptorSets();

        Pipeline::defaultTessellationPipelineConfigInfo(pipelineConfig, 4);

        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "water_shader.vert";
        pipelineConfig.fragShaderPath = "water_shader.frag";
        pipelineConfig.tessControlShaderPath = "water_shader.tesc";
        pipelineConfig.tessEvalShaderPath = "water_shader.tese";
        pipelineConfig.depthStencilInfo = {};
        pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
        // Enable alpha blending for transparency
        pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
        pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.attributeDescriptions.clear();

        setWaterData();
    }

    WaterMaterial::WaterMaterial(Device& device, const std::vector<unsigned char>& imageData,
                               int width, int height, int channels,
                               const std::string& vertShaderPath, const std::string& fragShaderPath)
        : Material(device) {
        // Increment instance count
        instanceCount++;

        createDescriptorSetLayoutIfNeeded(device);

        // Create texture directly from image data
        createTextureFromImageData(imageData, width, height, channels, textureImage, textureImageMemory);
        textureImageView = createImageView(textureImage);
        createTextureSampler(textureSampler);

        createDescriptorSets();

        Pipeline::defaultTessellationPipelineConfigInfo(pipelineConfig, 4);

        // Set default pipeline configuration
        pipelineConfig.vertShaderPath = "water_shader.vert";
        pipelineConfig.fragShaderPath = "water_shader.frag";
        pipelineConfig.tessControlShaderPath = "water_shader.tesc";
        pipelineConfig.tessEvalShaderPath = "water_shader.tese";
        pipelineConfig.depthStencilInfo = {};
        pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

        // pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.attributeDescriptions.clear();

        setWaterData();
    }

    WaterMaterial::~WaterMaterial() {
        auto destructionQueue = vk::Engine::getDestructionQueue();
        
        if (destructionQueue) {
            // schedule resources for safe destruction
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
            
            for (int i = 0; i < textureDescriptorSets.size(); i++) {
                if (textureDescriptorSets[i] != VK_NULL_HANDLE && descriptorPool) {
                    destructionQueue->pushDescriptorSet(textureDescriptorSets[i], descriptorPool->getPool());
                    textureDescriptorSets[i] = VK_NULL_HANDLE;
                }
            }
        }
    
        // Decrement instance count and clean up static resources if this is the last instance
        instanceCount--;
        if (instanceCount == 0) {
            std::cout << "Cleaning up WaterMaterial static resources" << std::endl;
            cleanupResources();
        }
    }

    void WaterMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
        if (!descriptorSetLayout) {
            auto layoutBuilder = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

            // Store the layout in a static member to prevent it from being destroyed
            descriptorSetLayout = layoutBuilder.build();

            // depends on frames in flight so that in use buffers are not written to (concurrent cpu write and gpu processing)
            descriptorPool = DescriptorPool::Builder(device)
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                .setMaxSets(200 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        }
    }

    void WaterMaterial::createTextureImage(const std::string& texturePath, VkImage& image, VkDeviceMemory& imageMemory) {
        // Load texture image
        int width, height, channels;
        std::string resolvedPath = AssetLoader::getInstance().resolvePath(texturePath);
        stbi_uc* imageData = stbi_load(resolvedPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!imageData) {
            throw std::runtime_error("Failed to load texture image: " + resolvedPath);
        }

        VkDeviceSize imageSize = width * height * 4;  // 4 bytes per pixel (RGBA)
        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        device.createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

        // Copy image data to staging buffer
        void* data;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);

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
        imageInfo.mipLevels = this->mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory);

        // Transition image layout and copy data
        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy buffer to image
        device.copyBufferToImage(
            stagingBuffer,
            image,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1);

        device.generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, width, height, this->mipLevels);

        auto destructionQueue = Engine::getDestructionQueue();
        if (destructionQueue) {
            destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
        } else {
            vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
            vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        }
    }

    void WaterMaterial::createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                                int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory) {
        // Calculate image size (always use RGBA format)
        VkDeviceSize imageSize = width * height * 4;
        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

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
            stagingBufferMemory);

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
                        dst[dstIdx] = imageData[srcIdx];          // R
                        dst[dstIdx + 1] = imageData[srcIdx + 1];  // G
                        dst[dstIdx + 2] = imageData[srcIdx + 2];  // B
                        dst[dstIdx + 3] = 255;                    // A (opaque)
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
        imageInfo.mipLevels = this->mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory);

        // Transition image layout and copy buffer to image
        device.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        device.copyBufferToImage(
            stagingBuffer,
            image,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1);

        device.generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, width, height, this->mipLevels);

        auto destructionQueue = Engine::getDestructionQueue();
        if (destructionQueue) {
            destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
        } else {
            vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
            vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        }
    }

    VkImageView WaterMaterial::createImageView(VkImage& image) {
        return device.createImageView(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            this->mipLevels);
    }

    void WaterMaterial::createTextureSampler(VkSampler& sampler) {
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
        samplerInfo.maxLod = static_cast<float>(this->mipLevels);

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void WaterMaterial::createDescriptorSets() {

        for (int i = 0; i < paramsBuffers.size(); i++) {
            paramsBuffers[i] = std::make_unique<Buffer>(device,
                sizeof(WaterData),
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

    void WaterMaterial::updateDescriptorSet(int frameIndex) {
        paramsBuffers[frameIndex]->writeToBuffer(&waterData);
        paramsBuffers[frameIndex]->flush();
    }

    void WaterMaterial::setWaterData(CreateWaterData createWaterData) {
        waterData.tessParams = glm::vec4(createWaterData.maxTessLevel, createWaterData.minTessDistance, createWaterData.maxTessDistance, 0.0f);
        waterData.textureParams = glm::vec4(createWaterData.textureRepetition, 0.0f, 0.0f);
        waterData.materialProperties = glm::vec4(createWaterData.ka, createWaterData.kd, createWaterData.ks, createWaterData.alpha);
        waterData.color = glm::vec4(createWaterData.defaultColor, createWaterData.transparency);
        waterData.flags.x = textureImage != VK_NULL_HANDLE ? 1 : 0;
    }

    void WaterMaterial::setWaves(std::vector<glm::vec4> params) {
        int count = params.size();
        if (count > MAX_NUM_WATER_WAVES) {
            printf("INFO: WaterMaterial only supports %d waves. Other waves are ignored.\n", MAX_NUM_WATER_WAVES);
            count = MAX_NUM_WATER_WAVES;
        }

        waterData.flags.y = count;

        for (int i = 0; i < count; i++) {
            waterData.waves[i] = params[i];
        }
    }

    void WaterMaterial::cleanupResources() {
        auto destructionQueue = Engine::getDestructionQueue();
        
        if (descriptorPool) {
            if (destructionQueue) {
                destructionQueue->pushDescriptorPool(descriptorPool->getPool());
            } else {
                descriptorPool->resetPool();
            }
            descriptorPool.reset();
        }
    
        if (descriptorSetLayout && descriptorSetLayout->getDescriptorSetLayout() != VK_NULL_HANDLE) {
            if (destructionQueue) {
                destructionQueue->pushDescriptorSetLayout(descriptorSetLayout->getDescriptorSetLayout());
            }
            descriptorSetLayout.reset();
        }
    }

    DescriptorSet WaterMaterial::getDescriptorSet(int frameIndex) const {
        DescriptorSet descriptorSet{};
        descriptorSet.binding = 1;
        descriptorSet.handle = textureDescriptorSets[frameIndex];
        descriptorSet.layout = descriptorSetLayout->getDescriptorSetLayout();

        return descriptorSet;
    }
}