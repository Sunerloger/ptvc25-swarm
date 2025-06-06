#pragma once

#include "../vk/vk_device.h"
#include "../vk/vk_buffer.h"
#include "../lighting/Sun.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <glm/glm.hpp>
#include <vector>

namespace vk {

class ShadowMap {
public:
    struct ShadowUbo {
        glm::mat4 lightViewMatrix{1.0f};
        glm::mat4 lightProjectionMatrix{1.0f};
        // x: shadow map size, y: PCF samples, z: bias, w: shadow strength
        glm::vec4 shadowParams{0.0f};
    };

    struct ShadowMapSettings {
        uint32_t width = 2048;
        uint32_t height = 2048;
        float bias = 0.005f; // prevent shadow acne
        int pcfSamples = 3; // antialiasing for shadows (1 = no PCF, 2 = 2x2, 3 = 3x3, etc.)
        float shadowStrength = 0.7f;  // [0,1]
        float orthoSize = 50.0f; // size of the orthographic projection (left/right/top/bottom)
        float nearPlane = 1.0f;
        float farPlane = 150.0f;
    };

    ShadowMap(Device& device, ShadowMapSettings settings = {});
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    
    VkRenderPass getRenderPass() const { return renderPass; }
    
    VkFramebuffer getFramebuffer() const { return framebuffer; }
    
    VkExtent2D getExtent() const { return {settings.width, settings.height}; }
    
    std::vector<VkClearValue> getClearValues() const;
    
    VkDescriptorImageInfo getDescriptorInfo() const;
    
    void updateShadowUbo(int frameIndex);
    
    const ShadowUbo& getShadowUbo() const { return shadowUbo; }
    
    VkDescriptorBufferInfo getShadowUboBufferInfo(int frameIndex) const;
    
    VkDescriptorSet getDescriptorSet(int frameIndex) const;
    
    void bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int frameIndex) const;
    
    VkDescriptorSetLayout getDescriptorSetLayout() const;

private:
    void createDepthResources();
    void createRenderPass();
    void createFramebuffer();
    void cleanup();
    
    void createDescriptorSetLayoutIfNeeded(Device& device);
    void createShadowUboBuffers();
    void createDescriptorSets();
    static void cleanupStaticResources();

    Device& device;
    ShadowMapSettings settings;
    ShadowUbo shadowUbo;
    
    std::vector<std::unique_ptr<Buffer>> shadowUboBuffers;
    std::vector<VkDescriptorSet> shadowDescriptorSets;
    
    static std::unique_ptr<DescriptorPool> descriptorPool;
    static std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
    static int instanceCount;

    VkImage depthImage{VK_NULL_HANDLE};
    VkDeviceMemory depthImageMemory{VK_NULL_HANDLE};
    VkImageView depthImageView{VK_NULL_HANDLE};
    VkSampler depthSampler{VK_NULL_HANDLE};

    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkFramebuffer framebuffer{VK_NULL_HANDLE};
};

}