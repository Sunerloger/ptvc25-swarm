#pragma once

#include "../GameObject.h"

#include "vulkan/vulkan.h"
#include "../scene/SceneManager.h"

namespace vk {

#define MAX_LIGHTS 10

	struct PointLight {
		glm::vec4 position{};
		glm::vec4 color{};
	};

	struct GlobalUbo {
		glm::mat4 projection{1.0f};
		glm::mat4 view{1.0f};
		glm::mat4 inverseView{1.0f};
		glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f};
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
		float aspectRatio;
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
