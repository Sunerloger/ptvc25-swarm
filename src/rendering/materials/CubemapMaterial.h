#pragma once

#include "Material.h"
#include "../../vk/vk_descriptors.h"
#include <array>

namespace vk {

    class CubemapMaterial : public Material {

    public:
        CubemapMaterial(Device& device, const std::array<std::string, 6>& facePaths);
        CubemapMaterial(Device& device, const std::string& singleImagePath, bool isHorizontalStrip = true);

        ~CubemapMaterial() override;

        VkDescriptorSet getDescriptorSet() const override { return cubemapDescriptorSet; }
        VkDescriptorSetLayout getDescriptorSetLayout() const override {
            return descriptorSetLayout ? descriptorSetLayout->getDescriptorSetLayout() : VK_NULL_HANDLE;
        }

        static std::unique_ptr<DescriptorPool> descriptorPool;
        static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
        static int instanceCount;

        static void cleanupResources();

    private:
        void createCubemapFromFaces(const std::array<std::string, 6>& facePaths);
        void createCubemapFromSingleImage(const std::string& imagePath, bool isHorizontalStrip);

        void createCubemapImageView();
        void createCubemapSampler();
        void createDescriptorSet();

        static void createDescriptorSetLayoutIfNeeded(Device& device);

        VkImage cubemapImage = VK_NULL_HANDLE;
        VkDeviceMemory cubemapImageMemory = VK_NULL_HANDLE;
        VkImageView cubemapImageView = VK_NULL_HANDLE;
        VkSampler cubemapSampler = VK_NULL_HANDLE;
        VkDescriptorSet cubemapDescriptorSet = VK_NULL_HANDLE;
    };
}