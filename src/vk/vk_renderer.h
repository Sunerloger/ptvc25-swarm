#pragma once

#include "vk_swap_chain.h"

#include <memory>
#include <vector>
#include <cassert>

namespace vk {
	class Renderer {
	   public:
		Renderer(Window& window, Device& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const {
			return m_swapChain->getRenderPass();
		}
		float getAspectRatio() const {
			return m_swapChain->extentAspectRatio();
		}
		bool isFrameInProgress() const {
			return isFrameStarted;
		}
		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress.");
			return m_commandBuffers[currentRenderFrameIndex];
		}
		
		VkRenderPass getCurrentRenderPass() const {
			return currentRenderPass;
		}

		VkCommandBuffer beginFrame();
		void endFrame();

		void beginRenderPass(
			VkCommandBuffer commandBuffer,
			VkRenderPass renderPass,
			VkFramebuffer framebuffer,
			VkExtent2D extent,
			const std::vector<VkClearValue>& clearValues);
		void endRenderPass(VkCommandBuffer commandBuffer);

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
			return currentRenderFrameIndex;
		}
		void recreateSwapChain();
		
		SwapChain& getSwapChain() const {
			return *m_swapChain;
		}

	private:
		void createFramePools();
		void allocateCommandBuffers();
		void freeFramePoolsAndCommandBuffers();

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> m_swapChain;
		std::vector<VkCommandPool> m_framePools;
		std::vector<VkCommandBuffer> m_commandBuffers;

		uint32_t currentImageIndex;
		int currentRenderFrameIndex{0};
		bool isFrameStarted{false};
		VkRenderPass currentRenderPass{VK_NULL_HANDLE};
	};
}
