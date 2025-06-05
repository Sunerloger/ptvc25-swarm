#include "ui_render_system.h"

#include "../../scene/SceneManager.h"


namespace vk {

	UIRenderSystem::UIRenderSystem(Device& device, Renderer& renderer, VkDescriptorSetLayout globalSetLayout)
		: device{device}, renderer{renderer}, globalSetLayout{globalSetLayout} {
	}

	UIRenderSystem::~UIRenderSystem() {
		for (auto& [key, layout] : pipelineLayoutCache) {
			vkDestroyPipelineLayout(device.device(), layout, nullptr);
		}
	}

	void UIRenderSystem::getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout) {
		// Check if we already have a pipeline layout for this material layout
		auto it = pipelineLayoutCache.find(materialSetLayout);
		if (it != pipelineLayoutCache.end()) {
			pipelineLayout = it->second;
			return;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(UIPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, materialSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// Cache the pipeline layout
		pipelineLayoutCache[materialSetLayout] = pipelineLayout;
	}

	UIRenderSystem::PipelineInfo& UIRenderSystem::getPipeline(const Material& material) {
		// Get the material's pipeline configuration
		PipelineConfigInfo config = material.getPipelineConfig();

		// Get the descriptor set layout directly from the material
		VkDescriptorSetLayout materialSetLayout = material.getDescriptorSetLayout();

		// Create or retrieve pipeline layout
		VkPipelineLayout pipelineLayout;
		getPipelineLayout(materialSetLayout, pipelineLayout);

		config.renderPass = renderer.getSwapChainRenderPass();
		config.pipelineLayout = pipelineLayout;

		// Check if we already have a pipeline for this configuration
		auto it = pipelineCache.find(config);
		if (it != pipelineCache.end()) {
			return it->second;
		}

		// Create pipeline because it doesn't exist yet
		PipelineInfo pipelineInfo{};
		pipelineInfo.pipelineLayout = pipelineLayout;

		pipelineInfo.pipeline = std::make_unique<Pipeline>(
			device,
			config
		);

		// Cache and return
		return pipelineCache[config] = std::move(pipelineInfo);
	}

	void UIRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		SceneManager& sceneManager = SceneManager::getInstance();

		// Collect UI objects and sort by z-index (back to front)
		auto uiWeakObjs = sceneManager.getUIObjects();
		std::vector<std::shared_ptr<GameObject>> uiGameObjects;
		uiGameObjects.reserve(uiWeakObjs.size());
		for (auto& weakObj : uiWeakObjs) {
			if (auto gameObject = weakObj.lock()) {
				if (gameObject->getModel() && gameObject->getModel()->getMaterial()) {
					uiGameObjects.push_back(gameObject);
				}
			}
		}
		// Sort by z position (ascending: furthest first)
		std::sort(uiGameObjects.begin(), uiGameObjects.end(),
			[](const std::shared_ptr<GameObject>& a, const std::shared_ptr<GameObject>& b) {
				return a->getPosition().z < b->getPosition().z;
			});
		// Render sorted UI objects
		for (auto& gameObject : uiGameObjects) {
			auto material = gameObject->getModel()->getMaterial();
			if (!material)
				continue;

			// writes current state into gpu buffer (ubo), implemented if material needs it
			material->updateDescriptorSet(renderer.getFrameIndex());

			// Get pipeline for this material
			auto& pipelineInfo = getPipeline(*material);

			// Bind pipeline
			pipelineInfo.pipeline->bind(frameInfo.commandBuffer);

			// Bind global descriptor set
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineInfo.pipelineLayout,
				0,
				1,
				&frameInfo.globalDescriptorSet,
				0, nullptr);

			// Set push constants
			UIPushConstantData push{};

			// Use the game object's model matrix and normal matrix
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();

			// TODO put in texture ubo and not dependent on descriptor set
			push.hasTexture = material->getDescriptorSet(renderer.getFrameIndex()) != VK_NULL_HANDLE ? 1 : 0;

			// Determine shader stages to push constants to
			VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineInfo.pipelineLayout,
				stageFlags,
				0,
				sizeof(UIPushConstantData),
				&push);

			// Bind material descriptor set
			VkDescriptorSet materialDS = material->getDescriptorSet(renderer.getFrameIndex());
			if (materialDS != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(
					frameInfo.commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineInfo.pipelineLayout,
					1,
					1,
					&materialDS,
					0, nullptr);
			}

			gameObject->getModel()->bind(frameInfo.commandBuffer);
			gameObject->getModel()->draw(frameInfo.commandBuffer);
		}
	}

}