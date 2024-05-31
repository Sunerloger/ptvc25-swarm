#pragma once

#include <glm/glm.hpp>
#include "../GameObject.h"

namespace lighting {
	class PointLight : public vk::GameObject {

	public:

		PointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3{ 1.0f }, glm::vec3 position = glm::vec3{0.0f});
		virtual ~PointLight() = default;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;

		glm::vec3 getPosition() const override;
		std::shared_ptr<vk::Model> getModel() const override { return nullptr; }

		float getIntensity() const;
		float getRadius() const;

		void setPosition(glm::vec3 newPosition);

	private:

		glm::vec3 position{};
		float radius;
		float lightIntensity = 1.0f;

	};
}