#pragma once

#include <glm/glm.hpp>
#include "../GameObject.h"

namespace lighting {
	class PointLight : public vk::GameObject {

	public:

		PointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3{ 1.0f });
		virtual ~PointLight() = default;

		float lightIntensity = 1.0f;

		glm::mat4 computeModelMatrix() override;
		glm::mat4 computeNormalMatrix() override;

	private:

		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		// yaw, pitch, roll
		glm::vec3 rotation{};

	};
}