#include "vk_destruction_queue.h"
#include "vk_swap_chain.h"
#include <iostream>
#include <array>

namespace vk {

DestructionQueue::DestructionQueue(Device& device, SwapChain* swapChain)
    : device(device), swapChain(swapChain) {
}

DestructionQueue::~DestructionQueue() {
    cleanup();
}

void DestructionQueue::pushBuffer(VkBuffer buffer, VkDeviceMemory memory) {
    if (buffer != VK_NULL_HANDLE || memory != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue instead of frame-based queue
        // this ensures resources are properly tracked even during resize
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.buffers.push_back({buffer, memory});
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();

			std::cout << "Destruction Queue - Push Buffer: Current frame index in swapchain - " << frameIndex << std::endl;
            
        	frameDeletionQueues[frameIndex].buffers.push_back({buffer, memory});
        }
    }
}

void DestructionQueue::pushImage(VkImage image, VkDeviceMemory memory) {
    if (image != VK_NULL_HANDLE || memory != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.images.push_back({image, memory});
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].images.push_back({image, memory});
        }
    }
}

void DestructionQueue::pushImageView(VkImageView imageView) {
    if (imageView != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.imageViews.push_back(imageView);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].imageViews.push_back(imageView);
        }
    }
}

void DestructionQueue::pushSampler(VkSampler sampler) {
    if (sampler != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.samplers.push_back(sampler);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].samplers.push_back(sampler);
        }
    }
}

void DestructionQueue::pushPipeline(VkPipeline pipeline) {
    if (pipeline != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.pipelines.push_back(pipeline);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].pipelines.push_back(pipeline);
        }
    }
}

void DestructionQueue::pushPipelineLayout(VkPipelineLayout pipelineLayout) {
    if (pipelineLayout != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.pipelineLayouts.push_back(pipelineLayout);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].pipelineLayouts.push_back(pipelineLayout);
        }
    }
}

void DestructionQueue::pushDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout) {
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.descriptorSetLayouts.push_back(descriptorSetLayout);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].descriptorSetLayouts.push_back(descriptorSetLayout);
        }
    }
}

void DestructionQueue::pushDescriptorPool(VkDescriptorPool descriptorPool) {
    if (descriptorPool != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.descriptorPools.push_back(descriptorPool);
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].descriptorPools.push_back(descriptorPool);
        }
    }
}

void DestructionQueue::pushDescriptorSet(VkDescriptorSet descriptorSet, VkDescriptorPool parentPool) {
    if (descriptorSet != VK_NULL_HANDLE && parentPool != VK_NULL_HANDLE) {
        // during resize operations, add to immediate deletion queue
        if (device.getWindow().framebufferResized) {
            immediateDeletionQueue.descriptorSets.push_back({descriptorSet, parentPool});
        } else {
            uint32_t frameIndex = swapChain->getCurrentFrame();
            frameDeletionQueues[frameIndex].descriptorSets.push_back({descriptorSet, parentPool});
        }
    }
}

void DestructionQueue::cleanupDeletionQueue(DeletionQueue& queue) {
	// destroy all resources in the queue in the correct order
	// (reverse order of creation to handle dependencies)
	
	// first free descriptor sets before destroying pools
	if (queue.descriptorSets.size() > 0) {
		std::cout << "DestructionQueue: Freeing " << queue.descriptorSets.size() << " descriptor sets" << std::endl;
		
		// group descriptor sets by their parent pool for more efficient freeing
		std::unordered_map<VkDescriptorPool, std::vector<VkDescriptorSet>> poolToSets;
		
		for (auto& pair : queue.descriptorSets) {
			VkDescriptorSet set = pair.first;
			VkDescriptorPool pool = pair.second;
			
			if (set != VK_NULL_HANDLE && pool != VK_NULL_HANDLE) {
				poolToSets[pool].push_back(set);
			}
		}
		
		// free descriptor sets for each pool
		for (auto& entry : poolToSets) {
			VkDescriptorPool pool = entry.first;
			std::vector<VkDescriptorSet>& sets = entry.second;
			
			if (pool != VK_NULL_HANDLE && !sets.empty()) {
				std::cout << "  Freeing " << sets.size() << " descriptor sets from pool "
					<< std::hex << (uint64_t)pool << std::dec << std::endl;
				
				vkFreeDescriptorSets(
					device.device(),
					pool,
					static_cast<uint32_t>(sets.size()),
					sets.data()
				);
			}
		}
	}
	queue.descriptorSets.clear();
	
	if (queue.descriptorPools.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.descriptorPools.size() << " descriptor pools" << std::endl;
		for (auto pool : queue.descriptorPools) {
			if (pool != VK_NULL_HANDLE) {
				vkDestroyDescriptorPool(device.device(), pool, nullptr);
			}
		}
	}
	queue.descriptorPools.clear();
	
	if (queue.descriptorSetLayouts.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.descriptorSetLayouts.size() << " descriptor set layouts" << std::endl;
		for (auto layout : queue.descriptorSetLayouts) {
			if (layout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(device.device(), layout, nullptr);
			}
		}
	}
	queue.descriptorSetLayouts.clear();
	
	if (queue.pipelineLayouts.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.pipelineLayouts.size() << " pipeline layouts" << std::endl;
		for (auto layout : queue.pipelineLayouts) {
			if (layout != VK_NULL_HANDLE) {
				vkDestroyPipelineLayout(device.device(), layout, nullptr);
			}
		}
	}
	queue.pipelineLayouts.clear();
	
	if (queue.pipelines.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.pipelines.size() << " pipelines" << std::endl;
		for (auto pipeline : queue.pipelines) {
			if (pipeline != VK_NULL_HANDLE) {
				vkDestroyPipeline(device.device(), pipeline, nullptr);
			}
		}
	}
	queue.pipelines.clear();
	
	if (queue.samplers.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.samplers.size() << " samplers" << std::endl;
		for (auto sampler : queue.samplers) {
			if (sampler != VK_NULL_HANDLE) {
				vkDestroySampler(device.device(), sampler, nullptr);
			}
		}
	}
	queue.samplers.clear();
	
	if (queue.imageViews.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.imageViews.size() << " image views" << std::endl;
		for (auto view : queue.imageViews) {
			if (view != VK_NULL_HANDLE) {
				vkDestroyImageView(device.device(), view, nullptr);
			}
		}
	}
	queue.imageViews.clear();
	
	if (queue.images.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.images.size() << " images and memory" << std::endl;
		for (auto& pair : queue.images) {
			if (pair.first != VK_NULL_HANDLE) {
				vkDestroyImage(device.device(), pair.first, nullptr);
			}
			if (pair.second != VK_NULL_HANDLE) {
				vkFreeMemory(device.device(), pair.second, nullptr);
			}
		}
	}
	queue.images.clear();
	
	if (queue.buffers.size() > 0) {
		std::cout << "DestructionQueue: Destroying " << queue.buffers.size() << " buffers and memory" << std::endl;
		for (auto& pair : queue.buffers) {
			if (pair.first != VK_NULL_HANDLE) {
				vkDestroyBuffer(device.device(), pair.first, nullptr);
			}
			if (pair.second != VK_NULL_HANDLE) {
				vkFreeMemory(device.device(), pair.second, nullptr);
			}
		}
	}
	queue.buffers.clear();
}

void DestructionQueue::flush() {
	static bool wasResizing = false;
	bool isResizing = device.getWindow().framebufferResized;
	
	if (wasResizing && !isResizing) {
		std::cout << "DestructionQueue: Resize operation completed, cleaning up immediate deletion queue" << std::endl;
		cleanupDeletionQueue(immediateDeletionQueue);
	}
	
	wasResizing = isResizing;
	
	if (isResizing) {
		return;
	}
	
	uint32_t currentFrame = swapChain->getCurrentFrame();
	
	// calculate the frame that was just completed
	uint32_t frameToCleanup = (currentFrame + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	
	if (frameToCleanup >= frameDeletionQueues.size()) {
		std::cout << "DestructionQueue: ERROR - frameToCleanup " << frameToCleanup
			<< " is out of bounds (max: " << (frameDeletionQueues.size() - 1) << ")" << std::endl;
		return;
	}
	
	// wait for the fence of the frame we're about to clean up
	// this ensures the GPU has finished using the resources
	VkFence fenceToWait = swapChain->getInFlightFence(frameToCleanup);
	if (fenceToWait != VK_NULL_HANDLE) {
		VkResult result = vkWaitForFences(
			device.device(),
			1,
			&fenceToWait,
			VK_TRUE,
			UINT64_MAX
		);
		
		if (result != VK_SUCCESS) {
			std::cout << "DestructionQueue: Warning - Failed to wait for fence during flush" << std::endl;
			return; // skip cleanup if fence wait fails
		}
	}
	
	// clean up the resources for this frame
	cleanupDeletionQueue(frameDeletionQueues[frameToCleanup]);
}

void DestructionQueue::cleanup() {
	std::cout << "DestructionQueue: Starting cleanup" << std::endl;
	
	logResourceCounts();
	
	// Wait for all in-flight fences from the swapchain
	// more targeted than vkDeviceWaitIdle and allows other swapchains to continue working
	swapChain->waitForAllFences();
	
	std::cout << "DestructionQueue: All fences signaled, cleaning up resources" << std::endl;
	
	// clean up all frame deletion queues
	for (size_t i = 0; i < frameDeletionQueues.size(); i++) {
		std::cout << "DestructionQueue: Cleaning up frame " << i << " deletion queue" << std::endl;
		cleanupDeletionQueue(frameDeletionQueues[i]);
	}
	
	// clean up the immediate deletion queue
	std::cout << "DestructionQueue: Cleaning up immediate deletion queue" << std::endl;
	cleanupDeletionQueue(immediateDeletionQueue);
	
	std::cout << "DestructionQueue: Cleanup complete" << std::endl;
}

void DestructionQueue::logResourceCounts() {
	size_t totalBuffers = 0;
	size_t totalImages = 0;
	size_t totalImageViews = 0;
	size_t totalSamplers = 0;
	size_t totalPipelines = 0;
	size_t totalPipelineLayouts = 0;
	size_t totalDescriptorSetLayouts = 0;
	size_t totalDescriptorPools = 0;
	size_t totalDescriptorSets = 0;
	
	// count resources in frame deletion queues
	for (size_t i = 0; i < frameDeletionQueues.size(); i++) {
		const auto& queue = frameDeletionQueues[i];
		totalBuffers += queue.buffers.size();
		totalImages += queue.images.size();
		totalImageViews += queue.imageViews.size();
		totalSamplers += queue.samplers.size();
		totalPipelines += queue.pipelines.size();
		totalPipelineLayouts += queue.pipelineLayouts.size();
		totalDescriptorSetLayouts += queue.descriptorSetLayouts.size();
		totalDescriptorPools += queue.descriptorPools.size();
		totalDescriptorSets += queue.descriptorSets.size();
	}
	
	// count resources in immediate deletion queue
	totalBuffers += immediateDeletionQueue.buffers.size();
	totalImages += immediateDeletionQueue.images.size();
	totalImageViews += immediateDeletionQueue.imageViews.size();
	totalSamplers += immediateDeletionQueue.samplers.size();
	totalPipelines += immediateDeletionQueue.pipelines.size();
	totalPipelineLayouts += immediateDeletionQueue.pipelineLayouts.size();
	totalDescriptorSetLayouts += immediateDeletionQueue.descriptorSetLayouts.size();
	totalDescriptorPools += immediateDeletionQueue.descriptorPools.size();
	totalDescriptorSets += immediateDeletionQueue.descriptorSets.size();
	
	if (totalBuffers > 0 || totalImages > 0 || totalImageViews > 0 || totalSamplers > 0 ||
		totalPipelines > 0 || totalPipelineLayouts > 0 || totalDescriptorSetLayouts > 0 ||
		totalDescriptorPools > 0 || totalDescriptorSets > 0) {
		
		std::cout << "DestructionQueue: Total resources to clean up - "
			<< "Buffers: " << totalBuffers
			<< ", Images: " << totalImages
			<< ", ImageViews: " << totalImageViews
			<< ", Samplers: " << totalSamplers
			<< ", Pipelines: " << totalPipelines
			<< ", PipelineLayouts: " << totalPipelineLayouts
			<< ", DescriptorSetLayouts: " << totalDescriptorSetLayouts
			<< ", DescriptorPools: " << totalDescriptorPools
			<< ", DescriptorSets: " << totalDescriptorSets
			<< std::endl;
	}
}

}