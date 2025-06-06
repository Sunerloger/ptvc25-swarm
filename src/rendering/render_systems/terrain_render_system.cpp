#include "terrain_render_system.h"

#include <stdexcept>
#include <glm/glm.hpp>

#include "../materials/TessellationMaterial.h"
#include "../../scene/SceneManager.h"


namespace vk {

	TerrainRenderSystem::TerrainRenderSystem(Device& device, Renderer& renderer, VkDescriptorSetLayout globalSetLayout)
		: device{ device }, renderer{ renderer }, globalSetLayout{ globalSetLayout } {
	}

	TerrainRenderSystem::~TerrainRenderSystem() {
		for (auto& [key, layout] : pipelineLayoutCache) {
			vkDestroyPipelineLayout(device.device(), layout, nullptr);
		}
	}

	void TerrainRenderSystem::getPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout) {
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
		pushConstantRange.size = sizeof(TerrainPushConstantData);

		std::vector<VkDescriptorSetLayout> setLayouts;
		setLayouts.push_back(globalSetLayout);
		setLayouts.push_back(materialSetLayout);
		
		if (shadowMapLayout != VK_NULL_HANDLE) {
			setLayouts.push_back(shadowMapLayout);
		}

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


	TerrainRenderSystem::PipelineInfo& TerrainRenderSystem::getPipeline(const Material& material) {
		// get the material's pipeline configuration
		PipelineConfigInfo config = material.getPipelineConfig();

		// get the descriptor set layout directly from the material
		VkDescriptorSetLayout materialSetLayout = material.getDescriptorSetLayout();

		return getPipeline(config, materialSetLayout);
	}
	
	TerrainRenderSystem::PipelineInfo& TerrainRenderSystem::getPipeline(PipelineConfigInfo& config, VkDescriptorSetLayout& materialSetLayout) {
		
		// create or retrieve pipeline layout
		VkPipelineLayout pipelineLayout;
		getPipelineLayout(materialSetLayout, pipelineLayout);
		
		// set the pipeline layout in the config
		config.pipelineLayout = pipelineLayout;
		
		// check if we already have a pipeline for this configuration
		auto it = pipelineCache.find(config);
		if (it != pipelineCache.end()) {
			return it->second;
		}
		
		// create pipeline because it doesn't exist yet
		PipelineInfo pipelineInfo{};
		pipelineInfo.pipelineLayout = pipelineLayout;
		
		pipelineInfo.pipeline = std::make_unique<Pipeline>(
			device,
			config
		);
		
		// cache and return
		return pipelineCache[config] = std::move(pipelineInfo);
	}

	void TerrainRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		SceneManager& sceneManager = SceneManager::getInstance();

		// render all terrain objects
		for (std::weak_ptr<GameObject> weakObj : sceneManager.getTerrainRenderObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject || !gameObject->getModel())
				continue;

			auto material = gameObject->getModel()->getMaterial();
			if (!material) continue;

			// writes current state into gpu buffer (ubo), implemented if material needs it
			material->updateDescriptorSet(renderer.getFrameIndex());

			PipelineInfo* pipelineInfoPtr = nullptr;
			
			if (frameInfo.isShadowPass) {
				PipelineConfigInfo shadowConfig = material->getPipelineConfig();
				VkRenderPass currentRenderPass = renderer.getCurrentRenderPass();
				Pipeline::terrainShadowPipelineConfigInfo(shadowConfig, currentRenderPass);
				
				VkDescriptorSetLayout materialSetLayout = material->getDescriptorSetLayout();
				pipelineInfoPtr = &getPipeline(shadowConfig, materialSetLayout);
			} else {
				// normal pipeline for main pass
				auto& pipelineInfo = getPipeline(*material);
				pipelineInfoPtr = &pipelineInfo;
			}

			pipelineInfoPtr->pipeline->bind(frameInfo.commandBuffer);

			// bind global descriptor set
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineInfoPtr->pipelineLayout,
				0,
				1,
				&frameInfo.globalDescriptorSet,
				0, nullptr
			);

			TerrainPushConstantData push{};

			// Use the game object's model matrix and normal matrix
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();

			// TODO put in texture ubo and not dependent on descriptor set
			// all objects rendered by this system should use TessellationMaterial
			int hasTexture = material->getDescriptorSet(renderer.getFrameIndex()) != VK_NULL_HANDLE ? 1 : 0;
			
			auto tessMat = std::static_pointer_cast<TessellationMaterial>(material);
			assert(tessMat && "All terrain objects must use TessellationMaterial");

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineInfoPtr->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
				VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				0,
				sizeof(TerrainPushConstantData),
				&push
			);

			// bind material descriptor set
			VkDescriptorSet materialDS = material->getDescriptorSet(renderer.getFrameIndex());
			if (materialDS != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(
					frameInfo.commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineInfoPtr->pipelineLayout,
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
	
	VkPipelineLayout TerrainRenderSystem::getPipelineLayout() const {
		// this assumes that all pipeline layouts have the same descriptor set layouts
		if (!pipelineCache.empty()) {
			return pipelineCache.begin()->second.pipelineLayout;
		}
		
		// no pipeline layout exists yet
		return VK_NULL_HANDLE;
	}
}