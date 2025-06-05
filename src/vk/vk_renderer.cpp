#include "vk_renderer.h"

#include "vk_destruction_queue.h"

// std
#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace vk {

	Renderer::Renderer(Window& window, Device& device)
		: window{window}, device{device} {
		recreateSwapChain();
		createFramePools();
		allocateCommandBuffers();
	}

	Renderer::~Renderer() {
		freeFramePoolsAndCommandBuffers();
	}

	void Renderer::recreateSwapChain() {
		while (window.getWidth() == 0 || window.getHeight() == 0) {
			glfwWaitEvents();
		}

		if (m_swapChain == nullptr) {
			m_swapChain = std::make_unique<SwapChain>(device, window.getExtent());
		} else {
			std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
			m_swapChain = std::make_unique<SwapChain>(device, window.getExtent(), oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*m_swapChain.get())) {
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
	}

	void Renderer::createFramePools() {
		m_framePools.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
			device.createCommandPool(m_framePools[i]);
		}
	}

	void Renderer::allocateCommandBuffers() {
		m_commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandPool = m_framePools[i];
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			if (vkAllocateCommandBuffers(device.device(), &allocInfo, &m_commandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate command buffer!");
		}
	}

	void Renderer::freeFramePoolsAndCommandBuffers() {
		std::cout << "Renderer: Starting to free command buffers and pools" << std::endl;
		
		// wait for all fences to complete before freeing resources
		if (m_swapChain) {
			m_swapChain->waitForAllFences();
		}
		
		std::cout << "Renderer: All fences signaled, now freeing command buffers and pools" << std::endl;
		
		for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
			if (m_commandBuffers[i] != VK_NULL_HANDLE && m_framePools[i] != VK_NULL_HANDLE) {
				vkFreeCommandBuffers(device.device(), m_framePools[i], 1, &m_commandBuffers[i]);
				vkDestroyCommandPool(device.device(), m_framePools[i], nullptr);
			}
		}
		m_commandBuffers.clear();
		m_framePools.clear();
		std::cout << "Renderer: Finished freeing command buffers and pools" << std::endl;
	}

	VkCommandBuffer Renderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		auto result = m_swapChain->acquireNextImage(&currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// reset all command buffers in the pool of the frame (ready for new commands but does not release memory) -> this is safe because of the fence in acquireNextImage
		vkResetCommandPool(device.device(), m_framePools[currentRenderFrameIndex], 0);

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		return commandBuffer;
	}

	void Renderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
		auto commandBuffer = getCurrentCommandBuffer();
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		auto result = m_swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
			window.framebufferResized) {
			window.framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentRenderFrameIndex = (currentRenderFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_swapChain->getRenderPass();
		renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(m_swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{{0, 0}, m_swapChain->getSwapChainExtent()};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't end render pass on command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
	}

}  // namespace lve