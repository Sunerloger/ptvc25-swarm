#include "animation_render_system.h"

#include <stdexcept>
#include <array>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

namespace vk {

	struct AnimPushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	AnimationRenderSystem::AnimationRenderSystem(Device& device, VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		VkDescriptorSetLayout textureSetLayout,
		VkDescriptorSetLayout animationSetLayout)
		: device(device) {
		// Create a pipeline layout with three descriptor set layouts:
		// set 0: global UBO
		// set 1: texture (Model's static textureDescriptorSetLayout)
		// set 2: animation (Model's static animationDescriptorSetLayout)

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(AnimPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, textureSetLayout, animationSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create animation render pipeline layout!");
		}

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(device, "animation_shader.vert", "animation_shader.frag", pipelineConfig);
	}

	AnimationRenderSystem::~AnimationRenderSystem() {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void AnimationRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout, VkDescriptorSetLayout animationSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(AnimPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, textureSetLayout, animationSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create animation render pipeline layout!");
		}
	}

	void AnimationRenderSystem::createPipeline(VkRenderPass renderPass) {
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(device, "animation_shader.vert", "animation_shader.frag", pipelineConfig);
	}

	void AnimationRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		// Bind the animation pipeline.
		pipeline->bind(frameInfo.commandBuffer);

		// Bind the global descriptor set (set 0).
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		// Iterate through game objects and render only animated ones.
		for (std::weak_ptr<GameObject> weakObj : frameInfo.sceneManager.getRenderObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject)
				continue;

			// Only render this object if its model supports animations.
			// if (!gameObject->getModel()->hasAnimation())
			// 	continue;

			// Set up push constants with the model and normal matrices.
			AnimPushConstantData push{};
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();

			vkCmdPushConstants(frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(AnimPushConstantData),
				&push);

			// Bind the texture descriptor set (set 1), as before.
			VkDescriptorSet textureDS = VK_NULL_HANDLE;
			if (gameObject->getModel()->hasTexture()) {
				textureDS = gameObject->getModel()->getTextureDescriptorSet();
			}
			vkCmdBindDescriptorSets(frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&textureDS,
				0,
				nullptr);

			// Bind the animation descriptor set (set 2).

			// VkDescriptorSet animDS = gameObject->getModel()->getAnimationDescriptorSet();
			// vkCmdBindDescriptorSets(frameInfo.commandBuffer,
			// 	VK_PIPELINE_BIND_POINT_GRAPHICS,
			// 	pipelineLayout,
			// 	2,
			// 	1,
			// 	&animDS,
			// 	0,
			// 	nullptr);

			// Bind vertex/index buffers and issue draw command.
			gameObject->getModel()->bind(frameInfo.commandBuffer);
			gameObject->getModel()->draw(frameInfo.commandBuffer);
		}
	}

}  // namespace vk