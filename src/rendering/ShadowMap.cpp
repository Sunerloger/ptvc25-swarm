#include "ShadowMap.h"
#include "../vk/vk_utils.hpp"
#include "../scene/SceneManager.h"
#include "../Engine.h"
#include "../camera/CameraUtils.h"

#include <stdexcept>
#include <iostream>

namespace vk {

    std::unique_ptr<DescriptorPool> ShadowMap::descriptorPool;
    std::unique_ptr<DescriptorSetLayout> ShadowMap::descriptorSetLayout;
    int ShadowMap::instanceCount = 0;
    
    ShadowMap::ShadowMap(Device& device, ShadowMapSettings settings)
        : device(device), settings(settings) {
        
        instanceCount++;
        
        createDescriptorSetLayoutIfNeeded(device);
        createDepthResources();
        createRenderPass();
        createFramebuffer();
        createShadowUboBuffers();
        createDescriptorSets();
        
        shadowUbo.shadowParams = glm::vec4(
            static_cast<float>(settings.width),
            static_cast<float>(settings.pcfSamples),
            settings.bias,
            settings.shadowStrength
        );
    }
    
    ShadowMap::~ShadowMap() {
        auto* destructionQueue = Engine::getDestructionQueue();
        
        if (destructionQueue) {
            for (int i = 0; i < shadowDescriptorSets.size(); i++) {
                if (shadowDescriptorSets[i] != VK_NULL_HANDLE && descriptorPool) {
                    destructionQueue->pushDescriptorSet(shadowDescriptorSets[i], descriptorPool->getPool());
                    shadowDescriptorSets[i] = VK_NULL_HANDLE;
                }
            }
            
            for (auto& buffer : shadowUboBuffers) {
                if (buffer) {
                    buffer->scheduleDestroy(*destructionQueue);
                }
            }
            
            if (depthSampler != VK_NULL_HANDLE) {
                destructionQueue->pushSampler(depthSampler);
                depthSampler = VK_NULL_HANDLE;
            }
            
            if (depthImageView != VK_NULL_HANDLE) {
                destructionQueue->pushImageView(depthImageView);
                depthImageView = VK_NULL_HANDLE;
            }
            
            if (depthImage != VK_NULL_HANDLE && depthImageMemory != VK_NULL_HANDLE) {
                destructionQueue->pushImage(depthImage, depthImageMemory);
                depthImage = VK_NULL_HANDLE;
                depthImageMemory = VK_NULL_HANDLE;
            }
            
            if (framebuffer != VK_NULL_HANDLE) {
                destructionQueue->pushFramebuffer(framebuffer);
                framebuffer = VK_NULL_HANDLE;
            }
            
            if (renderPass != VK_NULL_HANDLE) {
                destructionQueue->pushRenderPass(renderPass);
                renderPass = VK_NULL_HANDLE;
            }
        } else {
            cleanup();
        }
        
        instanceCount--;
        if (instanceCount == 0) {
            std::cout << "Cleaning up ShadowMap static resources" << std::endl;
            cleanupStaticResources();
        }
    }
    
    void ShadowMap::cleanup() {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
        
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device.device(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
        
        if (depthSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.device(), depthSampler, nullptr);
            depthSampler = VK_NULL_HANDLE;
        }
        
        if (depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device.device(), depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        }
        
        if (depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device.device(), depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        
        if (depthImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.device(), depthImageMemory, nullptr);
            depthImageMemory = VK_NULL_HANDLE;
        }
    }
    
    void ShadowMap::createDescriptorSetLayoutIfNeeded(Device& device) {
        if (!descriptorSetLayout) {
            auto layoutBuilder = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            descriptorSetLayout = layoutBuilder.build();
            
            descriptorPool = DescriptorPool::Builder(device)
                .setMaxSets(2 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        }
    }
    
    void ShadowMap::createDepthResources() {
        VkFormat depthFormat = device.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = settings.width;
        imageInfo.extent.height = settings.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory);
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map depth image view");
        }
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;  // Enable comparison for PCF
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        
        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &depthSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map sampler");
        }
        
        // transition to a layout suitable for attachment
        device.transitionImageLayout(
            depthImage,
            depthFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
    
    void ShadowMap::createRenderPass() {
        VkFormat depthFormat = device.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        
        if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map render pass");
        }
    }
    
    void ShadowMap::createFramebuffer() {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &depthImageView;
        framebufferInfo.width = settings.width;
        framebufferInfo.height = settings.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map framebuffer");
        }
    }
    
    void ShadowMap::createShadowUboBuffers() {
        shadowUboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        
        for (int i = 0; i < shadowUboBuffers.size(); i++) {
            shadowUboBuffers[i] = std::make_unique<Buffer>(
                device,
                sizeof(ShadowUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            shadowUboBuffers[i]->map();
        }
    }
    
    void ShadowMap::createDescriptorSets() {
        shadowDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        
        for (int i = 0; i < shadowDescriptorSets.size(); i++) {
            auto bufferInfo = shadowUboBuffers[i]->descriptorInfo();
            
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            imageInfo.imageView = depthImageView;
            imageInfo.sampler = depthSampler;
            
            DescriptorWriter(*descriptorSetLayout, *descriptorPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &imageInfo)
                .build(shadowDescriptorSets[i]);
        }
    }
    
    std::vector<VkClearValue> ShadowMap::getClearValues() const {
        std::vector<VkClearValue> clearValues(1);
        clearValues[0].depthStencil = {1.0f, 0};
        return clearValues;
    }
    
    VkDescriptorImageInfo ShadowMap::getDescriptorInfo() const {
        VkDescriptorImageInfo descriptorInfo{};
        descriptorInfo.sampler = depthSampler;
        descriptorInfo.imageView = depthImageView;
        descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        return descriptorInfo;
    }
    
    void ShadowMap::updateShadowUbo(int frameIndex) {
        SceneManager& sceneManager = SceneManager::getInstance();
        
        auto sun = sceneManager.getSun();
        auto player = sceneManager.getPlayer();
        
        if (!sun || !player) {
            return;
        }
        
        glm::vec3 playerPos = player->getPosition();
        
        shadowUbo.lightViewMatrix = sun->computeLightViewMatrix();

        float sunToPlayerDistance = glm::length(sun->getPosition() - playerPos);
        float effectiveFarPlane = sunToPlayerDistance * 1.5f;
        
        shadowUbo.lightProjectionMatrix = getOrthographicProjection(
            -settings.orthoSize, settings.orthoSize,
            -settings.orthoSize, settings.orthoSize,
            settings.nearPlane, effectiveFarPlane
        );
        
        shadowUboBuffers[frameIndex]->writeToBuffer(&shadowUbo);
        shadowUboBuffers[frameIndex]->flush();
    }
    
    DescriptorSet ShadowMap::getDescriptorSet(int frameIndex) const {
        DescriptorSet descriptorSet{};
        descriptorSet.binding = 2;
        descriptorSet.handle = shadowDescriptorSets[frameIndex];
        descriptorSet.layout = descriptorSetLayout->getDescriptorSetLayout();
    
        return descriptorSet;
    }
    
    VkDescriptorBufferInfo ShadowMap::getShadowUboBufferInfo(int frameIndex) const {
        return shadowUboBuffers[frameIndex]->descriptorInfo();
    }
    
    void ShadowMap::cleanupStaticResources() {
        auto* destructionQueue = Engine::getDestructionQueue();
        
        if (descriptorPool) {
            if (destructionQueue) {
                VkDescriptorPool poolHandle = descriptorPool->getPool();
                destructionQueue->pushDescriptorPool(poolHandle);
                descriptorPool.reset();
            } else {
                descriptorPool->resetPool();
                descriptorPool.reset();
            }
        }
        
        if (descriptorSetLayout && descriptorSetLayout->getDescriptorSetLayout() != VK_NULL_HANDLE) {
            if (destructionQueue) {
                VkDescriptorSetLayout layoutHandle = descriptorSetLayout->getDescriptorSetLayout();
                destructionQueue->pushDescriptorSetLayout(layoutHandle);
                descriptorSetLayout.reset();
            } else {
                descriptorSetLayout.reset();
            }
        }
    }
}