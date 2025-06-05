#include "TessellationMaterial.h"
#include "../../vk/vk_device.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../Engine.h"

#include <stdexcept>
#include <iostream>
#include <glm/gtc/noise.hpp>

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
        
        // Generate procedural textures (256x256 resolution)
        const int textureSize = 256;
        generateRockTexture(textureSize, textureSize);
        generateGrassTexture(textureSize, textureSize);
        generateSnowTexture(textureSize, textureSize);
        
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
            if (rockTextureSampler != VK_NULL_HANDLE) {
                destructionQueue->pushSampler(rockTextureSampler);
                rockTextureSampler = VK_NULL_HANDLE;
            }
            
            if (rockTextureImageView != VK_NULL_HANDLE) {
                destructionQueue->pushImageView(rockTextureImageView);
                rockTextureImageView = VK_NULL_HANDLE;
            }
            
            if (rockTextureImage != VK_NULL_HANDLE && rockTextureImageMemory != VK_NULL_HANDLE) {
                destructionQueue->pushImage(rockTextureImage, rockTextureImageMemory);
                rockTextureImage = VK_NULL_HANDLE;
                rockTextureImageMemory = VK_NULL_HANDLE;
            }
            
            if (grassTextureSampler != VK_NULL_HANDLE) {
                destructionQueue->pushSampler(grassTextureSampler);
                grassTextureSampler = VK_NULL_HANDLE;
            }
            
            if (grassTextureImageView != VK_NULL_HANDLE) {
                destructionQueue->pushImageView(grassTextureImageView);
                grassTextureImageView = VK_NULL_HANDLE;
            }
            
            if (grassTextureImage != VK_NULL_HANDLE && grassTextureImageMemory != VK_NULL_HANDLE) {
                destructionQueue->pushImage(grassTextureImage, grassTextureImageMemory);
                grassTextureImage = VK_NULL_HANDLE;
                grassTextureImageMemory = VK_NULL_HANDLE;
            }
            
            if (snowTextureSampler != VK_NULL_HANDLE) {
                destructionQueue->pushSampler(snowTextureSampler);
                snowTextureSampler = VK_NULL_HANDLE;
            }
            
            if (snowTextureImageView != VK_NULL_HANDLE) {
                destructionQueue->pushImageView(snowTextureImageView);
                snowTextureImageView = VK_NULL_HANDLE;
            }
            
            if (snowTextureImage != VK_NULL_HANDLE && snowTextureImageMemory != VK_NULL_HANDLE) {
                destructionQueue->pushImage(snowTextureImage, snowTextureImageMemory);
                snowTextureImage = VK_NULL_HANDLE;
                snowTextureImageMemory = VK_NULL_HANDLE;
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
            if (rockTextureSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), rockTextureSampler, nullptr);
            }
            
            if (rockTextureImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), rockTextureImageView, nullptr);
            }
            
            if (rockTextureImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.device(), rockTextureImage, nullptr);
            }
            
            if (rockTextureImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), rockTextureImageMemory, nullptr);
            }
            
            if (grassTextureSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), grassTextureSampler, nullptr);
            }
            
            if (grassTextureImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), grassTextureImageView, nullptr);
            }
            
            if (grassTextureImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.device(), grassTextureImage, nullptr);
            }
            
            if (grassTextureImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), grassTextureImageMemory, nullptr);
            }
            
            if (snowTextureSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), snowTextureSampler, nullptr);
            }
            
            if (snowTextureImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.device(), snowTextureImageView, nullptr);
            }
            
            if (snowTextureImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.device(), snowTextureImage, nullptr);
            }
            
            if (snowTextureImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.device(), snowTextureImageMemory, nullptr);
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
            descriptorSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
                .build();

            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(500 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * 100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
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

        VkDescriptorImageInfo rockImageInfo{};
        rockImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        rockImageInfo.imageView = rockTextureImageView;
        rockImageInfo.sampler = rockTextureSampler;

        VkDescriptorImageInfo grassImageInfo{};
        grassImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        grassImageInfo.imageView = grassTextureImageView;
        grassImageInfo.sampler = grassTextureSampler;

        VkDescriptorImageInfo snowImageInfo{};
        snowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        snowImageInfo.imageView = snowTextureImageView;
        snowImageInfo.sampler = snowTextureSampler;

        VkDescriptorImageInfo heightImageInfo{};
        heightImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        heightImageInfo.imageView = heightmapImageView;
        heightImageInfo.sampler = heightmapSampler;

        for (int i = 0; i < textureDescriptorSets.size(); i++) {
            auto bufferInfo = paramsBuffers[i]->descriptorInfo();
            DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &rockImageInfo)
                .writeImage(2, &grassImageInfo)
                .writeImage(3, &snowImageInfo)
                .writeImage(4, &heightImageInfo)
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
    
    // fractal Brownian motion (layered noise)
    float TessellationMaterial::seamlessFbm(const glm::vec2& uv, float scale, int octaves, float lacunarity, float gain) {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        for (int i = 0; i < octaves; i++) {
            glm::vec2 p = uv * scale * freq;
            float angleX = p.x * 2.0f * glm::pi<float>();
            float angleY = p.y * 2.0f * glm::pi<float>();
            glm::vec4 samplePos = glm::vec4(
                std::cos(angleX), std::sin(angleX),
                std::cos(angleY), std::sin(angleY)
            );
            sum += amp * glm::perlin(samplePos);  // sample 4D Perlin to wrap seamlessly
            freq *= lacunarity;
            amp *= gain;
        }
        return sum * 0.5f + 0.5f;  // [0,1]
    }
    
    float TessellationMaterial::cellular(const glm::vec2& p, float cellSize) {
        glm::vec2 baseCell = glm::floor(p / cellSize);
        
        float minDist = 1.0f;
        
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                glm::vec2 cellPos = baseCell + glm::vec2(x, y);
                
                glm::vec2 cellOffset = glm::vec2(
                    glm::fract(glm::sin(glm::dot(cellPos, glm::vec2(127.1f, 311.7f))) * 43758.5453f),
                    glm::fract(glm::sin(glm::dot(cellPos, glm::vec2(269.5f, 183.3f))) * 43758.5453f)
                );
                
                glm::vec2 featurePoint = cellPos + cellOffset;
                
                float dist = glm::length((p / cellSize) - featurePoint);
                
                minDist = glm::min(minDist, dist);
            }
        }
        
        return minDist;
    }
    
    void TessellationMaterial::generateRockTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = static_cast<float>(x) / width;
                float ny = static_cast<float>(y) / height;
                
                // rock cracks
                float cellNoise1 = cellular(glm::vec2(nx * 1.5f, ny * 1.5f), 0.4f);
                float cellNoise2 = cellular(glm::vec2(nx * 1.5f + 0.5f, ny * 1.5f + 0.5f), 0.5f);
                
                float crackPattern = (1.0f - cellNoise1) * (1.0f - cellNoise2) * 4.0f;
                crackPattern = glm::clamp(crackPattern, 0.0f, 1.0f);
                
                float largeDetail = seamlessFbm(glm::vec2(nx, ny), 2.5f, 4, 2.0f, 0.5f);
                float smallDetail = seamlessFbm(glm::vec2(nx, ny), 12.0f, 4, 2.0f, 0.4f);
                
                float rockPattern = largeDetail * 0.2f + smallDetail * 0.1f + crackPattern * 0.7f;
                
                glm::vec3 baseStone = glm::vec3(0.12f, 0.10f, 0.08f);
                glm::vec3 colorVariation = glm::vec3(rockPattern * 0.6f, rockPattern * 0.55f, rockPattern * 0.5f);
                glm::vec3 stoneRGB = baseStone + colorVariation;
                unsigned char r = static_cast<unsigned char>(glm::clamp(stoneRGB.r, 0.0f, 1.0f) * 255);
                unsigned char g = static_cast<unsigned char>(glm::clamp(stoneRGB.g, 0.0f, 1.0f) * 255);
                unsigned char b = static_cast<unsigned char>(glm::clamp(stoneRGB.b, 0.0f, 1.0f) * 255);
                unsigned char a = 255;
                
                int index = (y * width + x) * 4;
                textureData[index] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }
        
        uint32_t mipLevels = createTextureFromImageData(textureData, width, height, 4, rockTextureImage, rockTextureImageMemory);
        rockTextureImageView = createImageView(rockTextureImage);
        createTextureSampler(static_cast<float>(mipLevels), rockTextureSampler);
    }
    
    void TessellationMaterial::generateGrassTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);
        
        const float noiseScale = 3.0f;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = static_cast<float>(x) / width * noiseScale;
                float ny = static_cast<float>(y) / height * noiseScale;
                
                float largeDetail = seamlessFbm(glm::vec2(nx, ny), 2.0f, 4, 2.0f, 0.5f);
                float mediumDetail = seamlessFbm(glm::vec2(nx, ny), 6.0f, 3, 2.0f, 0.4f);
                
                // directional streaks for grass blades using cellular noise
                float cellPattern1 = cellular(glm::vec2(nx * 12.0f, ny * 12.0f + largeDetail), 0.08f);
                float cellPattern2 = cellular(glm::vec2(nx * 12.0f + 0.3f, ny * 12.0f), 0.08f);
                
                float streaks = (1.0f - cellPattern1) * (1.0f - cellPattern2);
                streaks = glm::pow(streaks, 3.0f); // Adjust contrast
                
                float grassPattern = largeDetail * 0.15f + mediumDetail * 0.05f + streaks * 0.8f;
                
                float baseGreen = 0.4f;
                float variation = grassPattern * 0.4f;
                
                unsigned char r = static_cast<unsigned char>((0.05f + variation * 0.2f) * 255);
                unsigned char g = static_cast<unsigned char>((baseGreen + variation * 0.6f) * 255);
                unsigned char b = static_cast<unsigned char>((0.05f + variation * 0.1f) * 255);
                unsigned char a = 255;
                
                int index = (y * width + x) * 4;
                textureData[index] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }
        
        uint32_t mipLevels = createTextureFromImageData(textureData, width, height, 4, grassTextureImage, grassTextureImageMemory);
        grassTextureImageView = createImageView(grassTextureImage);
        createTextureSampler(static_cast<float>(mipLevels), grassTextureSampler);
    }
    
    void TessellationMaterial::generateSnowTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);
        
        const float noiseScale = 1.5f;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = static_cast<float>(x) / width * noiseScale;
                float ny = static_cast<float>(y) / height * noiseScale;
                
                float largeDetail = seamlessFbm(glm::vec2(nx, ny), 1.5f, 3, 2.0f, 0.5f);
                float smallDetail = seamlessFbm(glm::vec2(nx, ny), 4.0f, 2, 2.0f, 0.3f);
                
                float snowPattern = largeDetail * 0.7f + smallDetail * 0.3f;
                
                // occasional sparkles using cellular noise
                float cellValue = cellular(glm::vec2(nx * 16.0f, ny * 16.0f), 0.12f);
                float sparkle = 0.0f;
                
                // only create sparkles at cell edges (where distance is around 0.5)
                if (cellValue > 0.3f && cellValue < 0.7f) {
                    sparkle = 0.7f * (1.0f - std::abs(cellValue - 0.5f) * 2.0f);
                }
                
                float baseSnowR = 0.85f, baseSnowG = 0.88f, baseSnowB = 0.95f;
                float variation = snowPattern * 0.08f;
                float sparkleTint = sparkle * 0.9f;
                unsigned char r = static_cast<unsigned char>(glm::clamp(baseSnowR, 0.0f, 1.0f) * 255);
                unsigned char g = static_cast<unsigned char>(glm::clamp(baseSnowG, 0.0f, 1.0f) * 255);
                unsigned char b = static_cast<unsigned char>(glm::clamp(baseSnowB + variation + sparkleTint, 0.0f, 1.0f) * 255);
                unsigned char a = 255;
                
                int index = (y * width + x) * 4;
                textureData[index] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }
        
        uint32_t mipLevels = createTextureFromImageData(textureData, width, height, 4, snowTextureImage, snowTextureImageMemory);
        snowTextureImageView = createImageView(snowTextureImage);
        createTextureSampler(static_cast<float>(mipLevels), snowTextureSampler);
    }
}