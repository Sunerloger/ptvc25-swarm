#include "TessellationMaterial.h"
#include "../../vk/vk_device.h"
#include "../../asset_utils/AssetLoader.h"
#include "../../Engine.h"

#include <stdexcept>
#include <iostream>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <algorithm>

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
        heightmapMipLevels = createTextureImage(heightmapPath, heightmapImage, heightmapImageMemory);
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
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
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
        
        // Generate mipmaps if this is not a heightmap texture
        if (&image != &heightmapImage) {
            device.generateMipmaps(image, VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels);
        } else {
            device.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        
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
    float TessellationMaterial::seamlessFbm(glm::vec2 uv, float scale, int octaves, float lacunarity, float gain) {
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
            sum += amp * glm::perlin(samplePos);  // sample 4D Perlin to wrap seamlessly (expensive, but looks way better for tiling than 2d)
            freq *= lacunarity;
            amp *= gain;
        }
        return sum * 0.5f + 0.5f;  // [0,1]
    }
    
    float TessellationMaterial::tileableCellular(glm::vec2 uv, float cellCount) {
        glm::vec2 p = uv * cellCount;
        glm::vec2 baseCell = glm::floor(p);
        glm::vec2 frac = p - baseCell;

        float minDist = 10.0f;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                glm::vec2 neighborCell = baseCell + glm::vec2((float)dx, (float)dy);

                glm::vec2 wrapped = glm::mod(neighborCell, cellCount);

                float hx = glm::fract(glm::sin(glm::dot(wrapped, glm::vec2(127.1f, 311.7f))) * 43758.5453f);
                float hy = glm::fract(glm::sin(glm::dot(wrapped, glm::vec2(269.5f, 183.3f))) * 43758.5453f);
                glm::vec2 featurePoint = glm::vec2(hx, hy);

                glm::vec2 diff = glm::vec2((float)dx, (float)dy) + featurePoint - frac;
                float d = glm::length(diff);
                minDist = glm::min(minDist, d);
            }
        }

        return glm::clamp(minDist / 1.41421356f, 0.0f, 1.0f);
    }

    float TessellationMaterial::tileableVoronoi(glm::vec2 uv, float cellCount) {
        glm::vec2 p = uv * cellCount;
        glm::vec2 iCell = glm::floor(p);
        glm::vec2 fCell = glm::fract(p);

        float d0 = 1.0f;
        float d1 = 1.0f;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                glm::vec2 neighbor = glm::vec2((float)dx, (float)dy);
                glm::vec2 cellId = iCell + neighbor;
                glm::vec2 wrapped = glm::mod(cellId, cellCount);

                float hx = glm::fract(glm::sin(glm::dot(wrapped, glm::vec2(127.1f, 311.7f))) * 43758.5453f);
                float hy = glm::fract(glm::sin(glm::dot(wrapped, glm::vec2(269.5f, 183.3f))) * 43758.5453f);
                glm::vec2 feature = glm::vec2(hx, hy);

                glm::vec2 diff = neighbor + feature - fCell;
                float d = glm::length(diff);

                if (d < d0) {
                    d1 = d0;
                    d0 = d;
                }
                else if (d < d1) {
                    d1 = d;
                }
            }
        }

        float cellValue = (d1 - d0) / glm::sqrt(2);
        return glm::clamp(cellValue, 0.0f, 1.0f);
    }
    
    void TessellationMaterial::generateRockTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);

        // Precompute two layers of fBm to warp the cellular lookup
        const float fBmScale1 = 3.0f;  // coarse warp
        const float fBmScale2 = 12.0f; // fine detail

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = static_cast<float>(x) / width;
                float ny = static_cast<float>(y) / height;
                glm::vec2 uv = glm::vec2(nx, ny);

                float rawBase = tileableVoronoi(uv, 50);
                float sharpVal = glm::smoothstep(0.3f, 0.7f, rawBase);
                glm::vec3 baseColor = glm::mix(glm::vec3(0.20f, 0.18f, 0.17f),
                    glm::vec3(0.35f, 0.33f, 0.3f), sharpVal);

                float detailFBM = seamlessFbm(uv * 4.0f, 4, 2, 1.2f, 0.5f) * 0.15f;

                glm::vec3 rockRGB = baseColor;
                rockRGB += glm::vec3(detailFBM);

                unsigned char r = static_cast<unsigned char>(glm::clamp(rockRGB.r, 0.0f, 1.0f) * 255);
                unsigned char g = static_cast<unsigned char>(glm::clamp(rockRGB.g, 0.0f, 1.0f) * 255);
                unsigned char b = static_cast<unsigned char>(glm::clamp(rockRGB.b, 0.0f, 1.0f) * 255);
                unsigned char a = 255;

                int index = (y * width + x) * 4;
                textureData[index + 0] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }

        rockTextureMipLevels = createTextureFromImageData(textureData, width, height, 4, rockTextureImage, rockTextureImageMemory);
        rockTextureImageView = createImageView(rockTextureImage);
        createTextureSampler(static_cast<float>(rockTextureMipLevels), rockTextureSampler);
    }
    
    void TessellationMaterial::generateGrassTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                glm::vec2 guv = glm::vec2(static_cast<float>(x) / width, static_cast<float>(y) / height);

                float largeDetail = seamlessFbm(guv * 16.0f, 3, 3, 2.0f, 0.5f);
                float mediumDetail = seamlessFbm(guv * 32.0f, 2, 3, 2.0f, 0.4f);
                
                glm::vec2 grassWarp;
                grassWarp.x = tileableCellular(guv * 2.0f, 8.0f) * 0.08f;
                grassWarp.y = tileableCellular(guv * 2.0f + glm::vec2(0.5f), 8.0f) * 0.08f;
                glm::vec2 gUVw = glm::fract(guv + grassWarp);

                float grassCell = tileableCellular(gUVw, 256.0f);
                float grassMask = glm::smoothstep(0.25f, 0.55f, grassCell);

                glm::vec2 gUVw2 = glm::fract(guv + grassWarp * 1.5f + glm::vec2(0.25f));
                float insideNoise = tileableCellular(gUVw2, 64.0f) * 0.15f;

                float grassPattern = grassMask * (0.6f + insideNoise * 0.4f);

                glm::vec3 baseGreen = glm::vec3(0.1f, 0.35f, 0.025f);
                glm::vec3 colorVariation = glm::vec3(grassPattern * 0.15f,
                    grassPattern * 0.25f,
                    grassPattern * 0.1f);
                glm::vec3 grassRGB = baseGreen + colorVariation;

                unsigned char r = static_cast<unsigned char>(glm::clamp(grassRGB.r, 0.0f, 1.0f) * 255);
                unsigned char g = static_cast<unsigned char>(glm::clamp(grassRGB.g, 0.0f, 1.0f) * 255);
                unsigned char b = static_cast<unsigned char>(glm::clamp(grassRGB.b, 0.0f, 1.0f) * 255);
                unsigned char a = 255;
                
                int index = (y * width + x) * 4;
                textureData[index] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }
        
        grassTextureMipLevels = createTextureFromImageData(textureData, width, height, 4, grassTextureImage, grassTextureImageMemory);
        grassTextureImageView = createImageView(grassTextureImage);
        createTextureSampler(static_cast<float>(grassTextureMipLevels), grassTextureSampler);
    }
    
    void TessellationMaterial::generateSnowTexture(int width, int height) {
        std::vector<unsigned char> textureData(width * height * 4);
        
        const float noiseScale = 1.5f;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = static_cast<float>(x) / width * noiseScale;
                float ny = static_cast<float>(y) / height * noiseScale;
                
                float snowMask = seamlessFbm(glm::vec2(nx, ny) * 2.0f, 6, 3, 2.0f, 0.6f);

                float baseR = 0.90f, baseG = 0.92f, baseB = 1.00f;
                float blueTint = glm::smoothstep(0.4f, 0.7f, snowMask) * 0.05f;

                unsigned char r = static_cast<unsigned char>(glm::clamp(baseR - blueTint, 0.0f, 1.0f) * 255);
                unsigned char g = static_cast<unsigned char>(glm::clamp(baseG - blueTint, 0.0f, 1.0f) * 255);
                unsigned char b = static_cast<unsigned char>(glm::clamp(baseB, 0.0f, 1.0f) * 255);
                unsigned char a = 255;
                
                int index = (y * width + x) * 4;
                textureData[index] = r;
                textureData[index + 1] = g;
                textureData[index + 2] = b;
                textureData[index + 3] = a;
            }
        }
        
        snowTextureMipLevels = createTextureFromImageData(textureData, width, height, 4, snowTextureImage, snowTextureImageMemory);
        snowTextureImageView = createImageView(snowTextureImage);
        createTextureSampler(static_cast<float>(snowTextureMipLevels), snowTextureSampler);
    }

    DescriptorSet TessellationMaterial::getDescriptorSet(int frameIndex) const {
        DescriptorSet descriptorSet{};
        descriptorSet.binding = 1;
        descriptorSet.handle = textureDescriptorSets[frameIndex];
        descriptorSet.layout = descriptorSetLayout->getDescriptorSetLayout();

        return descriptorSet;
    }
}