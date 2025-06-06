#pragma once

#include "../GameObject.h"

#include "vulkan/vulkan.h"
#include "../scene/SceneManager.h"

namespace vk {

	struct ShadowUbo {
		glm::mat4 lightSpaceMatrix{1.0f};
		// x: shadow map size, y: PCF samples, z: bias, w: shadow strength
		glm::vec4 shadowParams{0.0f};
		// x = normal offset to prevent shadow acne, yzw = unused
		glm::vec4 normalOffset{0.01f, 0.0f, 0.0f, 0.0f};
	};

	struct GlobalUbo {
		glm::mat4 projection{1.0f};
		glm::mat4 view{1.0f};
		glm::mat4 uiOrthographicProjection{1.0f};
		glm::vec4 sunDirection{0.0f, -1.0f, 0.0f, 1.0f};
		glm::vec4 sunColor{1.0f, 1.0f, 1.0f, 0.0f};
		glm::vec4 cameraPosition = glm::vec4{0.0f};
	};

	struct FrameInfo {
		float frameTime;
		VkCommandBuffer commandBuffer;
		VkDescriptorSet globalDescriptorSet;
		// TODO use this for debug rendering with jolt debug renderer (implement DebugRenderer.h)
		bool isDebugPhysics = false;
		bool isShadowPass = false;
	};
}
