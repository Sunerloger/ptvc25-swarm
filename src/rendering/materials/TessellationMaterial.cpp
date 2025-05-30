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

    // Constructor with separate color and heightmap textures and shader paths
    TessellationMaterial::TessellationMaterial(Device& device, const std::string& texturePath, const std::string& heightmapPath,
                                           const std::string& vertShaderPath,
                                           const std::string& fragShaderPath,
                                           const std::string& tessControlShaderPath,
                                           const std::string& tessEvalShaderPath,
                                           uint32_t patchControlPoints)
        : Material(device) {
        
        // Configure for tessellation if tessellation shaders are provided
        if (!tessControlShaderPath.empty() && !tessEvalShaderPath.empty()) {
            Pipeline::defaultTessellationPipelineConfigInfo(pipelineConfig, patchControlPoints);
            pipelineConfig.tessControlShaderPath = tessControlShaderPath;
            pipelineConfig.tessEvalShaderPath = tessEvalShaderPath;
        }
        else {
            Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        }
        
        pipelineConfig.vertShaderPath = vertShaderPath;
        pipelineConfig.fragShaderPath = fragShaderPath;

        createDescriptorSetLayoutIfNeeded(device);
        
        instanceCount++;
        
        uint32_t textureMipLevels = createTextureImage(texturePath);
        textureImageView = createImageView(textureImage);
        createTextureSampler(static_cast<float>(textureMipLevels), textureSampler);

        uint32_t heightmapMipLevels = createHeightmapImage(heightmapPath);
        if (m_hasHeightmapTexture) {
            heightmapImageView = createImageView(heightmapImage);
            createTextureSampler(static_cast<float>(heightmapMipLevels), heightmapSampler);
        }

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
            vkDestroySampler(device.device(), heightmapSampler, nullptr);
            vkFreeMemory(device.device(), heightmapImageMemory, nullptr);
        }
        
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
            // color texture sampler and heightmap sampler
            descriptorSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
                .build();
                
            // support for only 1 material for now (only one terrain)
            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(1)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
                .build();
        }
    }

    // @return mipLevels
    uint32_t TessellationMaterial::createTextureImage(const std::string& texturePath) {
        auto textureData = AssetLoader::getInstance().loadTexture(texturePath);
        return createTextureFromImageData(textureData.pixels, textureData.width, textureData.height, textureData.channels, textureImage, textureImageMemory);
    }

    // @return mipLevels
    uint32_t TessellationMaterial::createHeightmapImage(const std::string& heightmapPath) {
        auto heightmapData = AssetLoader::getInstance().loadTexture(heightmapPath);

        m_hasHeightmapTexture = true;

        return createTextureFromImageData(heightmapData.pixels, heightmapData.width, heightmapData.height, heightmapData.channels, heightmapImage, heightmapImageMemory);       
    }

    // @return mipLevels
    uint32_t TessellationMaterial::createTextureFromImageData(const std::vector<unsigned char>& imageData, int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory) {
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

        uint32_t mipLevels = std::floor(std::log2(std::max(width, height))) + 1;
        
        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory
        );
        
        // Transition image layout and copy data
        device.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer, image, width, height, 1);
        device.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // Clean up staging buffer
        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

        return mipLevels;
    }

    VkImageView TessellationMaterial::createImageView(VkImage image) {
        return device.createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void TessellationMaterial::createTextureSampler(float maxLod, VkSampler& sampler) {
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
        samplerInfo.maxLod = maxLod;
        
        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void TessellationMaterial::createDescriptorSet() {
        descriptorSet = VK_NULL_HANDLE;

        VkDescriptorSetLayout layout = descriptorSetLayout->getDescriptorSetLayout();
        
        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool->getPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;
        
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
        
        if (m_hasHeightmapTexture) {
            assert(heightmapImageView != VK_NULL_HANDLE);

            VkDescriptorImageInfo heightmapInfo{};
            heightmapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            heightmapInfo.imageView = heightmapImageView;
            heightmapInfo.sampler = heightmapSampler;

            VkWriteDescriptorSet heightmapWrite{};
            heightmapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            heightmapWrite.dstSet = descriptorSet;
            heightmapWrite.dstBinding = 1;
            heightmapWrite.dstArrayElement = 0;
            heightmapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            heightmapWrite.descriptorCount = 1;
            heightmapWrite.pImageInfo = &heightmapInfo;

            descriptorWrites.push_back(heightmapWrite);
        }
        
        vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void TessellationMaterial::setTessellationParams(float maxLevel, float minDistance, float maxDistance, float heightScale) {
        this->maxTessLevel = maxLevel;
        this->minTessDistance = minDistance;
        this->maxTessDistance = maxDistance;
        this->heightScale = heightScale;
    }

    void TessellationMaterial::setTextureRepetition(glm::vec2 textureRepetition) {
        this->textureRepetition = textureRepetition;
    }
}