#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include <glm/glm.hpp>

namespace vk {

    class TessellationMaterial : public Material {
    public:
        // Constructor with color texture and shader paths
        TessellationMaterial(Device& device, const std::string& texturePath,
                       const std::string& vertShaderPath,
                       const std::string& fragShaderPath,
                       const std::string& tessControlShaderPath,
                       const std::string& tessEvalShaderPath,
                       uint32_t patchControlPoints = 4);
        
        // Constructor with color texture and default shader paths
        TessellationMaterial(Device& device, const std::string& texturePath, uint32_t patchControlPoints = 4);
        
        // Constructor with separate color and heightmap textures and shader paths
        TessellationMaterial(Device& device, const std::string& texturePath, const std::string& heightmapPath,
                       const std::string& vertShaderPath = "texture_shader.vert",
                       const std::string& fragShaderPath = "texture_shader.frag",
                       const std::string& tessControlShaderPath = "",
                       const std::string& tessEvalShaderPath = "",
                       uint32_t patchControlPoints = 4);
        
        // Constructor with embedded texture data and shader paths
        TessellationMaterial(Device& device, const std::vector<unsigned char>& imageData,
                       int width, int height, int channels,
                       const std::string& vertShaderPath = "texture_shader.vert",
                       const std::string& fragShaderPath = "texture_shader.frag",
                       const std::string& tessControlShaderPath = "",
                       const std::string& tessEvalShaderPath = "",
                       uint32_t patchControlPoints = 4);
        
        ~TessellationMaterial() override;

        VkDescriptorSet getDescriptorSet() const override { return descriptorSet; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }
        
        void setTessellationParams(float maxLevel, float minDistance, float maxDistance, float heightScale);
        
        void setTextureRepetition(glm::vec2 textureRepetition);
        
        // Get current parameters
        glm::vec2 getTextureRepetition() const { return textureRepetition; }
        float getMaxTessLevel() const { return maxTessLevel; }
        float getMinTessDistance() const { return minTessDistance; }
        float getMaxTessDistance() const { return maxTessDistance; }
        float getHeightScale() const { return heightScale; }
        bool hasHeightmapTexture() const { return m_hasHeightmapTexture; }
        
        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;
        
        static void cleanupResources();

    private:
        void createTextureImage(const std::string& texturePath);
        void createHeightmapImage(const std::string& heightmapPath);
        void createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                      int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory, bool createHeightmapTexture = false);
        VkImageView createImageView(VkImage image);
        void createTextureSampler();
        void createDescriptorSet();
        
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
        bool m_hasHeightmapTexture = false;
        
        // Descriptor set
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        
        // max tessellation subdivisions
        float maxTessLevel = 16.0f;

        // within minTessDistance, the tessellation has maxTessLevels
        float minTessDistance = 20.0f;

        // tessellation decreases linearly until maxTessDistance (minimum tessellation level, here: no subdivisions)
        float maxTessDistance = 100.0f;

        // multiplication factor for heightmap
        float heightScale = 1.0f;
        
        // how often the texture repeats across the whole tessellation object
        glm::vec2 textureRepetition = glm::vec2(1.0f, 1.0f);
    };
}