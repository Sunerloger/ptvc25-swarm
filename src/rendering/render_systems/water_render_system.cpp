#include "water_render_system.h"
#include <stdexcept>

namespace vk {

	WaterRenderSystem::WaterRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: device{device}, renderPass{renderPass}, globalSetLayout{globalSetLayout} {
	}

	WaterRenderSystem::~WaterRenderSystem() {
		for (auto& [key, layout] : pipelineLayoutCache) {
			vkDestroyPipelineLayout(device.device(), layout, nullptr);
		}
	}

	void WaterRenderSystem::getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout) {
		// Check if we already have a pipeline layout for this material layout
		auto it = pipelineLayoutCache.find(materialSetLayout);
		if (it != pipelineLayoutCache.end()) {
			pipelineLayout = it->second;
			return;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(WaterPushConstantData);

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

	WaterRenderSystem::PipelineInfo& WaterRenderSystem::getPipeline(const Material& material) {
		// Get the material's pipeline configuration
		PipelineConfigInfo config = material.getPipelineConfig();

		// Get the descriptor set layout directly from the material
		VkDescriptorSetLayout materialSetLayout = material.getDescriptorSetLayout();

		// Create or retrieve pipeline layout
		VkPipelineLayout pipelineLayout;
		getPipelineLayout(materialSetLayout, pipelineLayout);

		config.renderPass = renderPass;
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

	void WaterRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		// Update elapsed time for water animation
		elapsedTime += frameInfo.frameTime;
		for (std::weak_ptr<GameObject> weakObj : SceneManager::getInstance().getWaterObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject || !gameObject->getModel())
				continue;

			auto material = gameObject->getModel()->getMaterial();
			if (!material)
				continue;

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
			WaterPushConstantData push{};

			// Use the game object's model matrix and normal matrix
			// The skybox GameObject class overrides these methods to return identity matrices
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();
			// Pass elapsed time for wave animation
			push.timeData.x = elapsedTime;

			// Determine shader stages to push constants to
			VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			// If using tessellation, include those shader stages
			if (material->getPipelineConfig().useTessellation) {
				stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			}

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineInfo.pipelineLayout,
				stageFlags,
				0,
				sizeof(WaterPushConstantData),
				&push);

			// Bind material descriptor set
			VkDescriptorSet materialDS = material->getDescriptorSet();
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