#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include "../../vk/vk_buffer.h"
#include "../../vk/vk_swap_chain.h"
#include "../../Engine.h"

#include <glm/glm.hpp>

namespace vk {

    class StandardMaterial : public Material {

    public:

        struct MaterialData {
            // x = ka, y = kd, z = ks, w = alpha
            glm::vec4 lightingProperties = glm::vec4{ 0.15f, 0.6f, 0.25f, 10.0f };
            // x = hasTexture, yzw = unused
            glm::vec4 flags = glm::vec4{ 1.0f };
        };

        struct MaterialCreationData {
            float ka = 0.15f;
            float kd = 0.6f;
            float ks = 0.25f;
            float shininess = 10.0f;
        };

        StandardMaterial(Device& device, const std::string& texturePath);
        StandardMaterial(Device& device, const std::string& texturePath,
                        const std::string& vertShaderPath, const std::string& fragShaderPath);
        // Constructor for embedded textures
        StandardMaterial(Device& device, const std::vector<unsigned char>& imageData,
                        int width, int height, int channels);
        // Constructor for embedded textures with custom shaders
        StandardMaterial(Device& device, const std::vector<unsigned char>& imageData,
                        int width, int height, int channels,
                        const std::string& vertShaderPath, const std::string& fragShaderPath);
        ~StandardMaterial() override;

        VkDescriptorSet getDescriptorSet(int frameIndex) const override { return textureDescriptorSets[frameIndex]; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }

        void setMaterialData(MaterialCreationData creationData = {});

        void updateDescriptorSet(int frameIndex) override;

        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;

        static void cleanupResources();

    private:
        void createTextureImage(const std::string& texturePath, VkImage& image, VkDeviceMemory& imageMemory);
        void createTextureFromImageData(const std::vector<unsigned char>& imageData,
                                       int width, int height, int channels, VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createTextureImageView(VkImage& image);
        void createTextureSampler(VkSampler& sampler);
        void createDescriptorSets();

        static void createDescriptorSetLayoutIfNeeded(Device& device);

        VkImage textureImage = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> textureDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };

        std::vector<std::unique_ptr<Buffer>> paramsBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };

        MaterialData materialData;
    };
}