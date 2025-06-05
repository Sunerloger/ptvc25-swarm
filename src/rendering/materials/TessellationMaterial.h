#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_buffer.h"
#include "../../vk/vk_swap_chain.h"
#include <glm/glm.hpp>
#include <cstdint>	// Required for uint32_t

namespace vk {

    class TessellationMaterial : public Material {
    public:
        struct MaterialData {
            // x = maxTessLevel, max tessellation subdivisions
            // y = minTessDistance, within minTessDistance the tessellation has maxTessLevels
            // z = maxTessDistance, tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
            // w = heightScale
            glm::vec4 tessParams{ 16.0f, 20.0f, 100.0f, 1.0f };

            // xy = textureRepetition, how often the texture repeats across the whole tessellation object
            // z = hasTexture
            // w = useHeightmapTexture
            glm::vec4 textureParams{ 0.25f, 0.25f, 1.0f, 1.0f };

            // x: ambient factor, y: diffuse factor, z: specular factor, w: shininess
            glm::vec4 lightingProperties{ 0.3f, 0.65f, 0.05f, 1.0f };
        };

        struct MaterialCreationData {
            // max tessellation subdivisions
            float maxTessLevel = 16.0f;

            // within minTessDistance the tessellation has maxTessLevels
            float minTessDistance = 20.0f;

            // tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
            float maxTessDistance = 100.0f;

            // height from center to highest peak / lowest valley
            float heightScale = 1.0f;

            // how often the texture repeats across the whole tessellation object
            glm::vec2 textureRepetition = glm::vec2{ 0.25f, 0.25f };
            
            // lighting properties
            float ka = 0.3f;
            float kd = 0.65f;
            float ks = 0.05f;
            float alpha = 1.0f;
        };

        // Constructor with separate color and heightmap textures and shader paths
        TessellationMaterial(Device& device, const std::string& texturePath, const std::string& heightmapPath,
                       const std::string& vertShaderPath = "texture_shader.vert",
                       const std::string& fragShaderPath = "texture_shader.frag",
                       const std::string& tessControlShaderPath = "",
                       const std::string& tessEvalShaderPath = "",
                       uint32_t patchControlPoints = 4);
        
        ~TessellationMaterial() override;

        VkDescriptorSet getDescriptorSet(int frameIndex) const override { return textureDescriptorSets[frameIndex]; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }
        
        void setParams(MaterialCreationData creationData);
        
        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;
        
        static void cleanupResources();

    private:
        uint32_t createTextureImage(const std::string& texturePath, VkImage& image, VkDeviceMemory& imageMemory);
        uint32_t createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                       int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createImageView(VkImage image);
        void createTextureSampler(float maxLod, VkSampler& sampler);
        void createDescriptorSets();

        void updateDescriptorSet(int frameIndex) override;
        
        static void createDescriptorSetLayoutIfNeeded(Device& device);
        
        // fractal Brownian motion noise
        float seamlessFbm(glm::vec2 uv, float scale, int octaves, float lacunarity, float gain);
        float tileableVoronoi(glm::vec2 uv, float cellCount);
        float tileableCellular(glm::vec2 uv, float cellCount);
        
        // Generate procedural textures
        void generateRockTexture(int width, int height);
        void generateGrassTexture(int width, int height);
        void generateSnowTexture(int width, int height);
        
        // Rock texture resources
        VkImage rockTextureImage = VK_NULL_HANDLE;
        VkDeviceMemory rockTextureImageMemory = VK_NULL_HANDLE;
        VkImageView rockTextureImageView = VK_NULL_HANDLE;
        VkSampler rockTextureSampler = VK_NULL_HANDLE;
        uint32_t rockTextureMipLevels = 1;
        
        // Grass texture resources
        VkImage grassTextureImage = VK_NULL_HANDLE;
        VkDeviceMemory grassTextureImageMemory = VK_NULL_HANDLE;
        VkImageView grassTextureImageView = VK_NULL_HANDLE;
        VkSampler grassTextureSampler = VK_NULL_HANDLE;
        uint32_t grassTextureMipLevels = 1;
        
        // Snow texture resources
        VkImage snowTextureImage = VK_NULL_HANDLE;
        VkDeviceMemory snowTextureImageMemory = VK_NULL_HANDLE;
        VkImageView snowTextureImageView = VK_NULL_HANDLE;
        VkSampler snowTextureSampler = VK_NULL_HANDLE;
        uint32_t snowTextureMipLevels = 1;
        
        // Heightmap resources (optional)
        VkImage heightmapImage = VK_NULL_HANDLE;
        VkDeviceMemory heightmapImageMemory = VK_NULL_HANDLE;
        VkImageView heightmapImageView = VK_NULL_HANDLE;
        VkSampler heightmapSampler = VK_NULL_HANDLE;
        uint32_t heightmapMipLevels = 1;
        
        std::vector<VkDescriptorSet> textureDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };

        std::vector<std::unique_ptr<Buffer>> paramsBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
        
        MaterialData materialData{};
    };
}