#include "tessellation_render_system.h"

#include <stdexcept>
#include <glm/glm.hpp>

#include "../materials/TessellationMaterial.h"
#include "../../scene/SceneManager.h"


namespace vk {

	TessellationRenderSystem::TessellationRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: device{ device }, renderPass{ renderPass }, globalSetLayout{ globalSetLayout } {
	}

	TessellationRenderSystem::~TessellationRenderSystem() {
		for (auto& [key, layout] : pipelineLayoutCache) {
			vkDestroyPipelineLayout(device.device(), layout, nullptr);
		}
	}

	void TessellationRenderSystem::getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout) {
		// Check if we already have a pipeline layout for this material layout
		auto it = pipelineLayoutCache.find(materialSetLayout);
		if (it != pipelineLayoutCache.end()) {
			pipelineLayout = it->second;
			return;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | 
		                               VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(TessellationPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts = { globalSetLayout, materialSetLayout };

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


	TessellationRenderSystem::PipelineInfo& TessellationRenderSystem::getPipeline(const Material& material) {
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

	void TessellationRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		SceneManager& sceneManager = SceneManager::getInstance();

		// render all tessellation objects
		for (std::weak_ptr<GameObject> weakObj : sceneManager.getTessellationRenderObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject || !gameObject->getModel())
				continue;

			auto material = gameObject->getModel()->getMaterial();
			if (!material) continue;

			auto& pipelineInfo = getPipeline(*material);

			pipelineInfo.pipeline->bind(frameInfo.commandBuffer);

			// bind global descriptor set
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineInfo.pipelineLayout,
				0,
				1,
				&frameInfo.globalDescriptorSet,
				0, nullptr
			);

			TessellationPushConstantData push{};

			// Use the game object's model matrix and normal matrix
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();

			// all objects rendered by this system should use TessellationMaterial
			int hasTexture = material->getDescriptorSet() != VK_NULL_HANDLE ? 1 : 0;
			
			auto tessMat = std::static_pointer_cast<TessellationMaterial>(material);
			assert(tessMat && "All tess-objects must use TessellationMaterial");
			
			glm::vec2 textureRepetition = tessMat->getTextureRepetition();
			float maxTessLevel = tessMat->getMaxTessLevel();
			float minTessDistance = tessMat->getMinTessDistance();
			float maxTessDistance = tessMat->getMaxTessDistance();
			float heightScale = tessMat->getHeightScale();
			int useHeightmapTexture = tessMat->hasHeightmapTexture() ? 1 : 0;
			
			push.params1 = glm::vec4(hasTexture, textureRepetition.x, textureRepetition.y, maxTessLevel);
			push.params2 = glm::vec4(minTessDistance, maxTessDistance, heightScale, useHeightmapTexture);

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineInfo.pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | 
				VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				0,
				sizeof(TessellationPushConstantData),
				&push
			);

			// bind material descriptor set
			VkDescriptorSet materialDS = material->getDescriptorSet();
			if (materialDS != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(
					frameInfo.commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineInfo.pipelineLayout,
					1,
					1,
					&materialDS,
					0, nullptr
				);
			}

			gameObject->getModel()->bind(frameInfo.commandBuffer);
			gameObject->getModel()->draw(frameInfo.commandBuffer);
		}
	}

}