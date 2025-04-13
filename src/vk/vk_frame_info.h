#pragma once

#include "../GameObject.h"

#include "vulkan/vulkan.h"
#include "../scene/SceneManager.h"

namespace vk {

	struct GlobalUbo {
		glm::mat4 projection{1.0f};
		glm::mat4 view{1.0f};
		glm::mat4 uiOrthographicProjection{1.0f};
		glm::mat4 uiPerspectiveProjection{1.0f};
		glm::mat4 uiView{1.0f};
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
