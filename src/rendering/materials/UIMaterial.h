#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"

namespace vk {

    class UIMaterial : public Material {

    public:
        UIMaterial(Device& device, const std::string& texturePath);
        UIMaterial(Device& device, const std::string& texturePath,
            const std::string& vertShaderPath, const std::string& fragShaderPath);
        // Constructor for embedded textures
        UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
            int width, int height, int channels);
        // Constructor for embedded textures with custom shaders
        UIMaterial(Device& device, const std::vector<unsigned char>& imageData,
            int width, int height, int channels,
            const std::string& vertShaderPath, const std::string& fragShaderPath);
        ~UIMaterial() override;

        VkDescriptorSet getDescriptorSet() const override { return textureDescriptorSet; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }

        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;

        static void cleanupResources();

    private:
        void createTextureImage(const std::string& texturePath);
        void createTextureFromImageData(const std::vector<unsigned char>& imageData,
            int width, int height, int channels);
        void createTextureImageView();
        void createTextureSampler();
        void createDescriptorSet();

        static void createDescriptorSetLayoutIfNeeded(Device& device);

        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
    };
}