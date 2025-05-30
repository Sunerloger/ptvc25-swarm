#include "TextureUtils.h"

#include "../vk/vk_device.h"

#include <cmath>

LoadedTexture loadTexture(Device& device, const std::vector<unsigned char>& pixels, int width, int height, int channels, const TextureOptions& opts) {

    uint32_t mipLevels = opts.generateMipmaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;

    VkImageCreateInfo imageInfo{};
    imageInfo.mipLevels = mipLevels;

    VkImage image;
    VkDeviceMemory mem;
    device.createImageWithInfo(imageInfo, /*device-local*/ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, mem);

    // transition 0->TRANSFER_DST, copy, then either:
    if (opts.generateMipmaps) {
        device.transitionImageLayout(image, imageInfo.format,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(/*…*/);
        device.generateMipmaps(image, imageInfo.format, width, height, mipLevels);
    }
    else {
        device.transitionImageLayout(image, imageInfo.format,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(/*…*/);
        device.transitionImageLayout(image, imageInfo.format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // 2) make the view
    VkImageViewCreateInfo viewInfo{/*…*/ };
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    VkImageView view = device.createImageView(image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    // 3) make the sampler
    VkSamplerCreateInfo sampInfo{/*…*/ };
    sampInfo.magFilter = VK_FILTER_LINEAR;
    sampInfo.minFilter = VkFilter(opts.trilinearFiltering
        ? VK_FILTER_LINEAR
        : VK_FILTER_NEAREST);
    sampInfo.mipmapMode = VkSamplerMipmapMode(opts.trilinearFiltering
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR
        : VK_SAMPLER_MIPMAP_MODE_NEAREST);
    sampInfo.anisotropyEnable = opts.anisotropicFilter ? VK_TRUE : VK_FALSE;
    sampInfo.maxAnisotropy = opts.anisotropicFilter ? opts.maxAnisotropy : 1.0f;
    sampInfo.minLod = 0.0f;
    sampInfo.maxLod = float(mipLevels);
    VkSampler sampler;
    vkCreateSampler(device.device(), &sampInfo, nullptr, &sampler);

    return { image, mem, view, sampler, mipLevels };
}
