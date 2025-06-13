#pragma once

#include "../GameObject.h"

namespace lighting {
	class Sun : public vk::GameObject {
	   public:
		Sun(glm::vec3 position = glm::vec3{0.0f}, glm::vec3 direction = glm::vec3{0.0f, -1.0f, 0.0f}, glm::vec3 color = glm::vec3{1.0f});
		virtual ~Sun() = default;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		inline std::shared_ptr<vk::Model> getModel() const override {
			return nullptr;
		}

		glm::vec3 getDirection() const;
		void setDirection(const glm::vec3& newDirection);
		void setPosition(const glm::vec3& newPosition);

		glm::vec3 getColor() const {
			return color;
		}
		
		glm::mat4 computeLightViewMatrix() const;

	private:
		glm::vec3 position{ 0.0f };
		glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
		glm::vec3 color{ 1.0f};
		
		glm::mat4 lightViewMatrix{1.0f};
		glm::mat4 lightSpaceMatrix{1.0f};
	};
}