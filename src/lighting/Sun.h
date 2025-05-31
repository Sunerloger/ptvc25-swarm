#pragma once

#include "../GameObject.h"

namespace lighting {
	class Sun : public vk::GameObject {

	public:

		Sun(glm::vec3 position, glm::vec3 direction, glm::vec3 color);
		virtual ~Sun() = default;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		inline std::shared_ptr<vk::Model> getModel() const override { return nullptr; }

		glm::vec3 getDirection() const;

		glm::vec3 getColor() const { return color; }

	private:

		glm::vec3 position{ 0.0f };
		glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
		glm::vec3 color{ 1.0f };
	};
}