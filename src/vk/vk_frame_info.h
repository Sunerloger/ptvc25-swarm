#pragma once

#include "../GameObject.h"

#include "vulkan/vulkan.h"
#include "../scene/SceneManager.h"

namespace vk {

	struct GlobalUbo {
		glm::mat4 projection{1.0f};
		glm::mat4 view{1.0f};
		glm::mat4 uiOrthographicProjection{1.0f};
		glm::vec4 sunDirection{0.0f, -1.0f, 0.0f, 1.0f};
		glm::vec4 sunColor{1.0f, 1.0f, 1.0f, 0.0f};
		std::vector<lighting::PointLight> pointLights;
	};

	struct FrameInfo {
		float frameTime;
		VkCommandBuffer commandBuffer;
		VkDescriptorSet globalDescriptorSet;
		SceneManager& sceneManager;
		// TODO use this for debug rendering with jolt debug renderer (implement DebugRenderer.h)
		bool isDebugPhysics = false;
	};
}
