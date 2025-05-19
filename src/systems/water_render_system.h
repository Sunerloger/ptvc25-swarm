#pragma once

#include "../vk/vk_pipeline.h"
#include "../vk/vk_frame_info.h"
#include "../vk/vk_device.h"
#include "../rendering/materials/Material.h"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace vk {

	struct WaterPushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
		glm::vec2 uvOffset{0.0f};
		float time = 0.0f;
		int hasTexture = 0;
	};

	class WaterRenderSystem {
	   public:
		WaterRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~WaterRenderSystem();

		void renderGameObjects(FrameInfo& frameInfo);

	   private:
		struct PipelineInfo {
			std::unique_ptr<Pipeline> pipeline;
			VkPipelineLayout pipelineLayout;
		};

		PipelineInfo& getPipeline(const Material& material);
		void createPipelineLayout(VkDescriptorSetLayout materialSetLayout, VkPipelineLayout& pipelineLayout);

		struct PipelineKey {
			std::string vertShaderPath;
			std::string fragShaderPath;
			bool depthWriteEnable;
			VkCompareOp depthCompareOp;
			VkCullModeFlags cullMode;

			bool operator==(const PipelineKey& other) const {
				return vertShaderPath == other.vertShaderPath &&
					   fragShaderPath == other.fragShaderPath &&
					   depthWriteEnable == other.depthWriteEnable &&
					   depthCompareOp == other.depthCompareOp &&
					   cullMode == other.cullMode;
			}
		};

		struct PipelineKeyHash {
			std::size_t operator()(const PipelineKey& key) const {
				// Simple hash function
				std::size_t h1 = std::hash<std::string>{}(key.vertShaderPath);
				std::size_t h2 = std::hash<std::string>{}(key.fragShaderPath);
				std::size_t h3 = std::hash<bool>{}(key.depthWriteEnable);
				std::size_t h4 = std::hash<int>{}(static_cast<int>(key.depthCompareOp));
				std::size_t h5 = std::hash<int>{}(static_cast<int>(key.cullMode));
				return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
			}
		};

		Device& device;
		VkRenderPass renderPass;
		VkDescriptorSetLayout globalSetLayout;

		std::unordered_map<PipelineKey, PipelineInfo, PipelineKeyHash> pipelineCache;
		std::unordered_map<VkDescriptorSetLayout, VkPipelineLayout> pipelineLayoutCache;
		// Accumulated time for UV animation
		float elapsedTime{0.0f};
	};

}  // namespace vk
