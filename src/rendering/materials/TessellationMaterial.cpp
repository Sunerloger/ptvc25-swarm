#include "TessellationMaterial.h"
#include "../../vk/vk_device.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../Engine.h"

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
        
        uint32_t textureMipLevels = createTextureImage(texturePath, textureImage, textureImageMemory);
        textureImageView = createImageView(textureImage);
        createTextureSampler(static_cast<float>(textureMipLevels), textureSampler);

        materialData.textureParams.w = true;
        uint32_t heightmapMipLevels = createTextureImage(heightmapPath, heightmapImage, heightmapImageMemory);
        heightmapImageView = createImageView(heightmapImage);
        createTextureSampler(static_cast<float>(heightmapMipLevels), heightmapSampler);

        createDescriptorSets();
    }

    TessellationMaterial::~TessellationMaterial() {
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
            
            if (materialData.textureParams.w) {
                if (heightmapImageView != VK_NULL_HANDLE) {
                    destructionQueue->pushImageView(heightmapImageView);
                    heightmapImageView = VK_NULL_HANDLE;
                }
                
                if (heightmapImage != VK_NULL_HANDLE && heightmapImageMemory != VK_NULL_HANDLE) {
                    destructionQueue->pushImage(heightmapImage, heightmapImageMemory);
                    heightmapImage = VK_NULL_HANDLE;
                    heightmapImageMemory = VK_NULL_HANDLE;
                }
                
                if (heightmapSampler != VK_NULL_HANDLE) {
                    destructionQueue->pushSampler(heightmapSampler);
                    heightmapSampler = VK_NULL_HANDLE;
                }
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
            
            if (materialData.textureParams.w) {
                if (heightmapImageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(device.device(), heightmapImageView, nullptr);
                }
                
                if (heightmapImage != VK_NULL_HANDLE) {
                    vkDestroyImage(device.device(), heightmapImage, nullptr);
                }
                
                if (heightmapSampler != VK_NULL_HANDLE) {
                    vkDestroySampler(device.device(), heightmapSampler, nullptr);
                }
                
                if (heightmapImageMemory != VK_NULL_HANDLE) {
                    vkFreeMemory(device.device(), heightmapImageMemory, nullptr);
                }
            }
        }
        
        instanceCount--;
        
        // Clean up static resources if this is the last instance
        if (instanceCount == 0) {
            cleanupResources();
        }
    }

    void TessellationMaterial::cleanupResources() {
        auto destructionQueue = vk::Engine::getDestructionQueue();
        
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

    void TessellationMaterial::createDescriptorSetLayoutIfNeeded(Device& device) {
        if (descriptorSetLayout == nullptr) {
            // color texture sampler and heightmap sampler
            descriptorSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
                .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(200 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        }
    }

    // @return mipLevels
    uint32_t TessellationMaterial::createTextureImage(const std::string& texturePath, VkImage& image, VkDeviceMemory& imageMemory) {
        auto textureData = AssetLoader::getInstance().loadTexture(texturePath);
        return createTextureFromImageData(textureData.pixels, textureData.width, textureData.height, textureData.channels, image, imageMemory);
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
        
        auto destructionQueue = vk::Engine::getDestructionQueue();
        if (destructionQueue) {
            destructionQueue->pushBuffer(stagingBuffer, stagingBufferMemory);
        } else {
            vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
            vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
        }

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

    void TessellationMaterial::createDescriptorSets() {
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

        VkDescriptorImageInfo heightImageInfo{};
        heightImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        heightImageInfo.imageView = heightmapImageView;
        heightImageInfo.sampler = heightmapSampler;

        for (int i = 0; i < textureDescriptorSets.size(); i++) {
            auto bufferInfo = paramsBuffers[i]->descriptorInfo();
            DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeImage(0, &imageInfo)
                .writeImage(1, &heightImageInfo)
                .writeBuffer(2, &bufferInfo)
                .build(textureDescriptorSets[i]);
        }
    }

    void TessellationMaterial::updateDescriptorSet(int frameIndex) {
        paramsBuffers[frameIndex]->writeToBuffer(&materialData);
        paramsBuffers[frameIndex]->flush();
    }

    void TessellationMaterial::setParams(MaterialCreationData creationData) {
        materialData.tessParams = glm::vec4{ creationData.maxTessLevel, creationData.minTessDistance, creationData.maxTessDistance, creationData.heightScale };
        materialData.textureParams = glm::vec4{ creationData.textureRepetition, 1.0f, 1.0f };
        materialData.lightingProperties = glm::vec4{ creationData.ka, creationData.kd, creationData.ks, creationData.alpha };
    }
}