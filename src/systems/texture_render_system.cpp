#include "texture_render_system.h"
#include <stdexcept>
#include <array>

namespace vk {

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	TextureRenderSystem::TextureRenderSystem(Device& device, VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		VkDescriptorSetLayout /*textureSetLayout*/)
		: device{device} {
		// Create a pipeline layout with two descriptor set layouts:
		// set 0: global UBO; set 1: texture (we use the Model's static textureDescriptorSetLayout)
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, Model::textureDescriptorSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture render pipeline layout!");
		}

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(device, "texture_shader.vert", "texture_shader.frag", pipelineConfig);
	}

	TextureRenderSystem::~TextureRenderSystem() {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void TextureRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout textureSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, textureSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture render pipeline layout!");
		}
	}

	void TextureRenderSystem::createPipeline(VkRenderPass renderPass) {
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(device, "texture_shader.vert", "texture_shader.frag", pipelineConfig);
	}

	void TextureRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		pipeline->bind(frameInfo.commandBuffer);
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0, nullptr);

		for (std::weak_ptr<GameObject> weakObj : frameInfo.sceneManager.getRenderObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject)
				continue;

			SimplePushConstantData push{};
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();
			vkCmdPushConstants(frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

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
				0, nullptr);

			gameObject->getModel()->bind(frameInfo.commandBuffer);
			gameObject->getModel()->draw(frameInfo.commandBuffer);
		}
	}

}  // namespace vk