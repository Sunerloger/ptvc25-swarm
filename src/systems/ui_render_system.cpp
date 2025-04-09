#include "ui_render_system.h"
#include <stdexcept>
#include <array>
#include <iostream>

namespace vk {

	UIRenderSystem::UIRenderSystem(Device& device, VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		VkDescriptorSetLayout textureSetLayout)
		: device{device} {
		createPipelineLayout(globalSetLayout, Model::textureDescriptorSetLayout);

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(device, "ui_shader.vert", "ui_shader.frag", pipelineConfig);

		if (Model::textureDescriptorPool) {
			createDefaultTexture(Model::textureDescriptorPool->getPool(), Model::textureDescriptorSetLayout);
		}
	}

	UIRenderSystem::~UIRenderSystem() {
		if (defaultTextureSampler != VK_NULL_HANDLE) {
			vkDestroySampler(device.device(), defaultTextureSampler, nullptr);
		}
		if (defaultTextureImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device.device(), defaultTextureImageView, nullptr);
		}
		if (defaultTextureImage != VK_NULL_HANDLE) {
			vkDestroyImage(device.device(), defaultTextureImage, nullptr);
		}
		if (defaultTextureImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device.device(), defaultTextureImageMemory, nullptr);
		}
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void UIRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(UIPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, textureSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI render pipeline layout!");
		}
	}

	void UIRenderSystem::createDefaultTexture(VkDescriptorPool textureDescriptorPool, VkDescriptorSetLayout textureSetLayout) {
		// 1. Create a 1x1 white pixel.
		const int texWidth = 1;
		const int texHeight = 1;
		VkDeviceSize imageSize = texWidth * texHeight * 4;	// 4 bytes per pixel (RGBA)
		uint8_t whitePixel[4] = {255, 255, 255, 255};

		// 2. Create a staging buffer and copy the pixel data.
		Buffer stagingBuffer(device,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.map();
		stagingBuffer.writeToBuffer(whitePixel, imageSize);
		stagingBuffer.flush();

		// 3. Create the optimal image for the texture.
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, defaultTextureImage, defaultTextureImageMemory);

		// 4. Transition image layout, copy the buffer data into the image, then transition for shader access.
		device.transitionImageLayout(defaultTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		device.copyBufferToImage(stagingBuffer.getBuffer(), defaultTextureImage, texWidth, texHeight, 1);
		device.transitionImageLayout(defaultTextureImage, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// 5. Create an image view for the texture.
		defaultTextureImageView = device.createImageView(defaultTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		// 6. Create a sampler for the texture.
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &defaultTextureSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create default texture sampler!");
		}

		// 7. Allocate a descriptor set for the default texture.
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = textureDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &textureSetLayout;

		if (vkAllocateDescriptorSets(device.device(), &allocInfo, &defaultTextureDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate default texture descriptor set!");
		}

		// 8. Update the descriptor set with the image view and sampler.
		VkDescriptorImageInfo imageInfoDS{};
		imageInfoDS.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoDS.imageView = defaultTextureImageView;
		imageInfoDS.sampler = defaultTextureSampler;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = defaultTextureDescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfoDS;

		vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);
	}

	void UIRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		pipeline->bind(frameInfo.commandBuffer);
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0, nullptr);

		for (std::weak_ptr<GameObject> weakObj : frameInfo.sceneManager.getUIObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject)
				continue;

			std::shared_ptr<UIComponent> uiComponent = std::dynamic_pointer_cast<UIComponent>(gameObject);

			UIPushConstantData push{};
			push.modelMatrix = uiComponent->computeModelMatrix();
			push.normalMatrix = uiComponent->computeNormalMatrix();
			push.hasTexture = uiComponent->getModel()->hasTexture() ? 1 : 0;
			push.usePerspectiveProjection = uiComponent->getUsePerspectiveProjection();
			vkCmdPushConstants(frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(UIPushConstantData),
				&push);

			VkDescriptorSet textureDS = VK_NULL_HANDLE;
			if (uiComponent->getModel()->hasTexture()) {
				textureDS = uiComponent->getModel()->getTextureDescriptorSet();
			} else {
				textureDS = defaultTextureDescriptorSet;
			}
			vkCmdBindDescriptorSets(frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&textureDS,
				0, nullptr);

			uiComponent->getModel()->bind(frameInfo.commandBuffer);
			uiComponent->getModel()->draw(frameInfo.commandBuffer);
		}
	}

}  // namespace vk
