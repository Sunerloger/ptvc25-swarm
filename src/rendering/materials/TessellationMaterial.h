#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_buffer.h"
#include "../../vk/vk_swap_chain.h"
#include <glm/glm.hpp>

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
        uint32_t createTextureImage(const std::string& texturePath);
        uint32_t createHeightmapImage(const std::string& heightmapPath);
        uint32_t createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                      int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createImageView(VkImage image);
        void createTextureSampler(float maxLod, VkSampler& sampler);
        void createDescriptorSets();

        void updateDescriptorSet(int frameIndex) override;
        
        static void createDescriptorSetLayoutIfNeeded(Device& device);
        
        // Texture resources
        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        
        // Heightmap resources (optional)
        VkImage heightmapImage = VK_NULL_HANDLE;
        VkDeviceMemory heightmapImageMemory = VK_NULL_HANDLE;
        VkImageView heightmapImageView = VK_NULL_HANDLE;
        VkSampler heightmapSampler = VK_NULL_HANDLE;
        
        std::vector<VkDescriptorSet> textureDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };

        std::vector<std::unique_ptr<Buffer>> paramsBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
        
        MaterialData materialData{};
    };
}