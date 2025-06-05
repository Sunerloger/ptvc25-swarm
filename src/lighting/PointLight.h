#pragma once

#include <glm/glm.hpp>
#include "../GameObject.h"

namespace lighting {
	class PointLight : public vk::GameObject {

	public:

		PointLight(glm::vec3 color = glm::vec3{ 1.0f }, glm::vec3 position = glm::vec3{0.0f});
		virtual ~PointLight() = default;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;

		glm::vec3 getPosition() const override;
		inline std::shared_ptr<vk::Model> getModel() const override { return nullptr; }

		void setPosition(glm::vec3 newPosition);

	private:

		glm::vec3 position{ 0.0f };
		glm::vec3 color{ 1.0f };

	};
}