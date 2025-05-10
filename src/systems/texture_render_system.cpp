#include "texture_render_system.h"
#include <stdexcept>

namespace vk {

	TextureRenderSystem::TextureRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: device{ device }, renderPass{ renderPass }, globalSetLayout{ globalSetLayout } {
	}

	TextureRenderSystem::~TextureRenderSystem() {
		// wait for the device to finish operations before destroying resources
		vkDeviceWaitIdle(device.device());
		
		for (auto& [key, layout] : pipelineLayoutCache) {
			vkDestroyPipelineLayout(device.device(), layout, nullptr);
		}
	}

	void TextureRenderSystem::createPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout) {
		// Check if we already have a pipeline layout for this material layout
		auto it = pipelineLayoutCache.find(materialSetLayout);
		if (it != pipelineLayoutCache.end()) {
			pipelineLayout = it->second;
			return;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

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


	TextureRenderSystem::PipelineInfo& TextureRenderSystem::getPipeline(const Material& material) {
		// Get the material's pipeline configuration
		const auto& config = material.getPipelineConfig();
		
		// Create a key for the pipeline cache
		PipelineKey key{
			config.vertShaderPath,
			config.fragShaderPath,
			config.depthStencilInfo.depthWriteEnable == VK_TRUE,
			config.depthStencilInfo.depthCompareOp,
			config.rasterizationInfo.cullMode
		};

		// Check if we already have a pipeline for this configuration
		auto it = pipelineCache.find(key);
		if (it != pipelineCache.end()) {
			return it->second;
		}

		// Get the descriptor set layout directly from the material
		VkDescriptorSetLayout materialSetLayout = material.getDescriptorSetLayout();

		// Create pipeline layout
		VkPipelineLayout pipelineLayout;
		createPipelineLayout(materialSetLayout, pipelineLayout);

		// Create pipeline config
		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);

		// Apply material properties
		pipelineConfig.depthStencilInfo.depthWriteEnable = config.depthStencilInfo.depthWriteEnable;
		pipelineConfig.depthStencilInfo.depthCompareOp = config.depthStencilInfo.depthCompareOp;
		pipelineConfig.rasterizationInfo.cullMode = config.rasterizationInfo.cullMode;

		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;

		// Create pipeline
		PipelineInfo pipelineInfo{};
		pipelineInfo.pipelineLayout = pipelineLayout;
		
		// Create standard pipeline
		pipelineInfo.pipeline = std::make_unique<Pipeline>(
			device,
			config.vertShaderPath,
			config.fragShaderPath,
			pipelineConfig
		);

		// Cache and return
		return pipelineCache[key] = std::move(pipelineInfo);
	}

	void TextureRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
		// Render all standard objects (non-tessellated)
		for (std::weak_ptr<GameObject> weakObj : frameInfo.sceneManager.getStandardRenderObjects()) {
			std::shared_ptr<GameObject> gameObject = weakObj.lock();
			if (!gameObject || !gameObject->getModel())
				continue;

			auto material = gameObject->getModel()->getMaterial();
			if (!material) continue;

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
				0, nullptr
			);

			// Set push constants
			SimplePushConstantData push{};

			// Use the game object's model matrix and normal matrix
			// The skybox GameObject class overrides these methods to return identity matrices
			push.modelMatrix = gameObject->computeModelMatrix();
			push.normalMatrix = gameObject->computeNormalMatrix();

			push.hasTexture = material->getDescriptorSet() != VK_NULL_HANDLE ? 1 : 0;
			
			// No type checking - trust the implementation

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
				sizeof(SimplePushConstantData),
				&push
			);

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
					0, nullptr
				);
			}

			// Draw
			gameObject->getModel()->bind(frameInfo.commandBuffer);
			gameObject->getModel()->draw(frameInfo.commandBuffer);
		}
	}

} // namespace vk