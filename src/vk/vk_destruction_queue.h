#pragma once

#include "vk_device.h"
#include "vk_swap_chain.h"
#include <vector>
#include <deque>
#include <memory>
#include <array>

namespace vk {

struct DeletionQueue {
    std::vector<std::pair<VkBuffer, VkDeviceMemory>> buffers;
    std::vector<std::pair<VkImage, VkDeviceMemory>> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkSampler> samplers;
    std::vector<VkPipeline> pipelines;
    std::vector<VkPipelineLayout> pipelineLayouts;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorPool> descriptorPools;
    // track descriptor sets with their parent pool
    std::vector<std::pair<VkDescriptorSet, VkDescriptorPool>> descriptorSets;
};

class DestructionQueue {

public:
    DestructionQueue(Device& device, SwapChain& swapChain);
    ~DestructionQueue();

    void pushBuffer(VkBuffer buffer, VkDeviceMemory memory);
    void pushImage(VkImage image, VkDeviceMemory memory);
    void pushImageView(VkImageView imageView);
    void pushSampler(VkSampler sampler);
    void pushPipeline(VkPipeline pipeline);
    void pushPipelineLayout(VkPipelineLayout pipelineLayout);
    void pushDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
    void pushDescriptorPool(VkDescriptorPool descriptorPool);
    void pushDescriptorSet(VkDescriptorSet descriptorSet, VkDescriptorPool parentPool);

    // called at the end of each frame to process deletions
    void flush();
    
    // called during shutdown to clean up all resources
    void cleanup();

private:
    Device& device;
    SwapChain& swapChain;
    
    // keep deletion queues for each frame in flight
    std::array<DeletionQueue, SwapChain::MAX_FRAMES_IN_FLIGHT> frameDeletionQueues; // Using 2 for MAX_FRAMES_IN_FLIGHT
    
    // immediate deletion queue for resources that need to be deleted right away
    // this is used during cleanup
    DeletionQueue immediateDeletionQueue;
    
    // helper method to clean up a deletion queue
    void cleanupDeletionQueue(DeletionQueue& queue);
    
    // helper method to log resource counts
    void logResourceCounts();
};

}