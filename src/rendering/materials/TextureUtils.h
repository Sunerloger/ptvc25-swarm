/*#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct TextureOptions {
    bool generateMipmaps = true;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    float maxAnisotropy = 1.0f; // 1.0 = off
};

struct LoadedTexture {
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
    uint32_t       mipLevels;
};

LoadedTexture loadTexture(Device& device, const std::vector<unsigned char>& pixels, int width, int height, int channels, const TextureOptions& opts);

*/