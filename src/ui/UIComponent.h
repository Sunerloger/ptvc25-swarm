#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../GameObject.h"

namespace vk {
	class UIComponent : public GameObject {
	   public:
		UIComponent(std::shared_ptr<Model> model, glm::vec3 position = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f),
			glm::vec3 rotation = glm::vec3(0.0f));
		UIComponent(std::shared_ptr<Model> model, glm::mat4 modelMatrix);
		virtual ~UIComponent() = default;

		glm::mat4 computeModelMatrix() const override;

		glm::mat4 computeNormalMatrix() const override;

		glm::vec3 getPosition() const override;

		std::shared_ptr<Model> getModel() const override;

		glm::vec3 getScale() const;

	   private:
		std::shared_ptr<Model> model;

		glm::vec3 position;
		glm::vec3 scale;
		glm::vec3 rotation;
	};
}