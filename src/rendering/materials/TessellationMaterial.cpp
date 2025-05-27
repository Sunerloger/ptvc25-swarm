#include "TessellationMaterial.h"
#include "../../vk/vk_device.h"
#include "../../asset_utils/AssetLoader.h"

#include <stdexcept>
#include <iostream>

namespace vk {

    // Static member initialization
    std::unique_ptr<DescriptorPool> TessellationMaterial::descriptorPool = nullptr;
    std::unique_ptr<DescriptorSetLayout> TessellationMaterial::descriptorSetLayout = nullptr;
    int TessellationMaterial::instanceCount = 0;

    // Constructor with color texture and default shader paths
    TessellationMaterial::TessellationMaterial(Device& device, const std::string& texturePath)
        : Material(device) {
        
        // Increment instance count
        instanceCount++;
        
        // Create descriptor set layout if needed
        createDescriptorSetLayoutIfNeeded(device);
        
        // Create texture
        createTextureImage(texturePath);
        createTextureImageView();
        createTextureSampler();
        createDescriptorSet();
        
        // Set default shader paths
        pipelineConfig.vertShaderPath = "terrain_shader.vert";
        pipelineConfig.fragShaderPath = "terrain_shader.frag";
        pipelineConfig.tessControlShaderPath = "terrain_tess_control.tesc";
        pipelineConfig.tessEvalShaderPath = "terrain_tess_eval.tese";
        
        // Configure for tessellation
        Pipeline::tessellationPipelineConfigInfo(pipelineConfig, 3);  // 3 control points for triangle patches
    }
    
    // Constructor with color texture and shader paths
    TessellationMaterial::TessellationMaterial(Device& device, const std::string& texturePath,
                                           const std::string& vertShaderPath,
                                           const std::string& fragShaderPath,
                                           const std::string& tessControlShaderPath,
                                           const std::string& tessEvalShaderPath)
        : Material(device) {
        
        // Set shader paths
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;
        
        // Configure for tessellation if tessellation shaders are provided
        if (!tessControlShaderPath.empty() && !tessEvalShaderPath.empty()) {
            Pipeline::tessellationPipelineConfigInfo(pipelineConfig, 3);  // 3 control points for triangle patches
            pipelineConfig.tessControlShaderPath = tessControlShaderPath;
            pipelineConfig.tessEvalShaderPath = tessEvalShaderPath;
        }
        
        // Create descriptor set layout if needed
        createDescriptorSetLayoutIfNeeded(device);
        
        // Increment instance count
        instanceCount++;
        
        // Create texture
        createTextureImage(texturePath);
        createTextureImageView();
        createTextureSampler();
        createDescriptorSet();
    }

    // Constructor with separate color and heightmap textures and shader paths
    TessellationMaterial::TessellationMaterial(Device& device, const std::string& texturePath, const std::string& heightmapPath,
                                           const std::string& vertShaderPath,
                                           const std::string& fragShaderPath,
                                           const std::string& tessControlShaderPath,
                                           const std::string& tessEvalShaderPath)
        : Material(device) {
        
        // Set shader paths
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;
        
        // Configure for tessellation if tessellation shaders are provided
        if (!tessControlShaderPath.empty() && !tessEvalShaderPath.empty()) {
            Pipeline::tessellationPipelineConfigInfo(pipelineConfig, 3);  // 3 control points for triangle patches
            pipelineConfig.tessControlShaderPath = tessControlShaderPath;
            pipelineConfig.tessEvalShaderPath = tessEvalShaderPath;
        }
        
        // Create descriptor set layout if needed
        createDescriptorSetLayoutIfNeeded(device);
        
        // Increment instance count
        instanceCount++;
        
        // Create textures
        createTextureImage(texturePath);
        createHeightmapImage(heightmapPath);
        createTextureImageView();
        createHeightmapImageView();
        createTextureSampler();
        createDescriptorSet();
    }

    // Constructor with embedded texture data and shader paths
    TessellationMaterial::TessellationMaterial(Device& device, const std::vector<unsigned char>& imageData,
                                           int width, int height, int channels,
                                           const std::string& vertShaderPath,
                                           const std::string& fragShaderPath,
                                           const std::string& tessControlShaderPath,
                                           const std::string& tessEvalShaderPath)
        : Material(device) {
        
        // Set shader paths
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;
        
        // Configure for tessellation if tessellation shaders are provided
        if (!tessControlShaderPath.empty() && !tessEvalShaderPath.empty()) {
            Pipeline::tessellationPipelineConfigInfo(pipelineConfig, 3);  // 3 control points for triangle patches
            pipelineConfig.tessControlShaderPath = tessControlShaderPath;
            pipelineConfig.tessEvalShaderPath = tessEvalShaderPath;
        }
        
        // Create descriptor set layout if needed
        createDescriptorSetLayoutIfNeeded(device);
        
        // Increment instance count
        instanceCount++;
        
        // Create texture from image data
        createTextureFromImageData(imageData, width, height, channels);
        createTextureImageView();
        createTextureSampler();
        createDescriptorSet();
    }

    TessellationMaterial::~TessellationMaterial() {
        // Clean up resources
        vkDestroySampler(device.device(), textureSampler, nullptr);
        vkDestroyImageView(device.device(), textureImageView, nullptr);
        vkDestroyImage(device.device(), textureImage, nullptr);
        vkFreeMemory(device.device(), textureImageMemory, nullptr);
        
        if (m_hasHeightmapTexture) {
            vkDestroyImageView(device.device(), heightmapImageView, nullptr);
            vkDestroyImage(device.device(), heightmapImage, nullptr);
            vkFreeMemory(device.device(), heightmapImageMemory, nullptr);
        }
        
        // Decrement instance count
        instanceCount--;
        
        // Clean up static resources if this is the last instance
        if (instanceCount == 0) {
            cleanupResources();
        }
    }

    void TessellationMaterial::cleanupResources() {
        descriptorPool.reset();
        descriptorSetLayout.reset();
    }

    void TessellationMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
        if (descriptorSetLayout == nullptr) {
            // Create descriptor set layout
            descriptorSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
                .build();
                
            // Create descriptor pool
            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(100)  // Adjust as needed
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200)  // Adjust as needed
                .build();
        }
    }

    void TessellationMaterial::createTextureImage(const std::string& texturePath) {
        // Use AssetLoader to load texture
        auto textureData = AssetLoader::getInstance().loadTexture(texturePath);
        
        // Create texture from image data
        createTextureFromImageData(textureData.pixels, textureData.width, textureData.height, textureData.channels);
    }

    void TessellationMaterial::createHeightmapImage(const std::string& heightmapPath) {
        // Use AssetLoader to load heightmap
        auto heightmapData = AssetLoader::getInstance().loadTexture(heightmapPath);
        
        // Create heightmap image
        VkDeviceSize imageSize = heightmapData.width * heightmapData.height * heightmapData.channels;
        
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
        
        // Copy data to staging buffer
        void* data;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, heightmapData.pixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device.device(), stagingBufferMemory);
        
        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = heightmapData.width;
        imageInfo.extent.height = heightmapData.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            heightmapImage,
            heightmapImageMemory
        );
        
        // Transition image layout and copy data
        device.transitionImageLayout(heightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer, heightmapImage, heightmapData.width, heightmapData.height, 1);
        device.transitionImageLayout(heightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // Clean up staging buffer
        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        
        // Set flag
        m_hasHeightmapTexture = true;
    }

    void TessellationMaterial::createTextureFromImageData(const std::vector<unsigned char>& imageData, int width, int height, int channels) {
        VkDeviceSize imageSize = width * height * channels;
        
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
        
        // Copy data to staging buffer
        void* data;
        vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, imageData.data(), static_cast<size_t>(imageSize));
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
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory
        );
        
        // Transition image layout and copy data
        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer, textureImage, width, height, 1);
        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // Clean up staging buffer
        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
    }

    void TessellationMaterial::createTextureImageView() {
        textureImageView = device.createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void TessellationMaterial::createHeightmapImageView() {
        if (m_hasHeightmapTexture) {
            heightmapImageView = device.createImageView(heightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void TessellationMaterial::createTextureSampler() {
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
        
        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void TessellationMaterial::createDescriptorSet() {
        descriptorSet = VK_NULL_HANDLE;
        
        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool->getPool();
        allocInfo.descriptorSetCount = 1;
        // Store the layout in a member variable to ensure it doesn't go out of scope
        VkDescriptorSetLayout descriptorLayout = descriptorSetLayout->getDescriptorSetLayout();
        allocInfo.pSetLayouts = &descriptorLayout;
        
        if (vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set!");
        }
        
        // Update descriptor set for color texture
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        
        std::vector<VkWriteDescriptorSet> descriptorWrites = {descriptorWrite};
        
        // Always add heightmap descriptor (use the color texture as a fallback if no heightmap)
        VkDescriptorImageInfo heightmapInfo{};
        heightmapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        // Use heightmap if available, otherwise use the color texture as a fallback
        if (m_hasHeightmapTexture) {
            heightmapInfo.imageView = heightmapImageView;
        } else {
            heightmapInfo.imageView = textureImageView; // Fallback to color texture
        }
        
        heightmapInfo.sampler = textureSampler;
        
        VkWriteDescriptorSet heightmapWrite{};
        heightmapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        heightmapWrite.dstSet = descriptorSet;
        heightmapWrite.dstBinding = 1;
        heightmapWrite.dstArrayElement = 0;
        heightmapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        heightmapWrite.descriptorCount = 1;
        heightmapWrite.pImageInfo = &heightmapInfo;
        
        descriptorWrites.push_back(heightmapWrite);
        
        // Update descriptor set
        vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void TessellationMaterial::setTessellationParams(float maxLevel, float distance, float minDistance, float heightScale) {
        // Store the values in member variables for later access
        this->maxTessLevel = maxLevel;
        this->tessDistance = distance;
        this->minTessDistance = minDistance;
        this->heightScale = heightScale;
    }

    void TessellationMaterial::setTileScale(float x, float y) {
        // Store the values in member variables for later access
        this->tileScale = glm::vec2(x, y);
    }
}