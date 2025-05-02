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
                       const std::string& tessEvalShaderPath);
        
        // Constructor with color texture and default shader paths
        TessellationMaterial(Device& device, const std::string& texturePath);
        
        // Constructor with separate color and heightmap textures and shader paths
        TessellationMaterial(Device& device, const std::string& texturePath, const std::string& heightmapPath,
                       const std::string& vertShaderPath = "texture_shader.vert",
                       const std::string& fragShaderPath = "texture_shader.frag",
                       const std::string& tessControlShaderPath = "",
                       const std::string& tessEvalShaderPath = "");
        
        // Constructor with embedded texture data and shader paths
        TessellationMaterial(Device& device, const std::vector<unsigned char>& imageData,
                       int width, int height, int channels,
                       const std::string& vertShaderPath = "texture_shader.vert",
                       const std::string& fragShaderPath = "texture_shader.frag",
                       const std::string& tessControlShaderPath = "",
                       const std::string& tessEvalShaderPath = "");
        
        ~TessellationMaterial() override;

        VkDescriptorSet getDescriptorSet() const override { return descriptorSet; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }
        
        // Tessellation parameters
        void setTessellationParams(float maxLevel, float distance, float minDistance, float heightScale);
        
        // Texture tiling parameters
        void setTileScale(float x, float y);
        
        // Get current parameters
        glm::vec2 getTileScale() const { return tileScale; }
        float getMaxTessLevel() const { return maxTessLevel; }
        float getTessDistance() const { return tessDistance; }
        float getMinTessDistance() const { return minTessDistance; }
        float getHeightScale() const { return heightScale; }
        bool hasHeightmap() const { return hasHeightmapTexture; }
        
        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;
        
        static void cleanupResources(Device& device);

    private:
        void createTextureImage(const std::string& texturePath);
        void createHeightmapImage(const std::string& heightmapPath);
        void createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                      int width, int height, int channels);
        void createTextureImageView();
        void createHeightmapImageView();
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
        bool hasHeightmapTexture = false;
        
        // Descriptor set
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        
        // Tessellation parameters
        float maxTessLevel = 16.0f;
        float tessDistance = 20.0f;
        float minTessDistance = 100.0f;
        float heightScale = 1.0f;
        
        // Texture tiling parameters
        glm::vec2 tileScale = glm::vec2(0.1f, 0.1f);
    };
}