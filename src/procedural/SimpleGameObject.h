#pragma once

#include "../GameObject.h"
#include "../vk/vk_model.h"
#include <memory>
#include <glm/glm.hpp>

namespace procedural {

	// A simple game object that holds a model and transform
	class SimpleGameObject : public vk::GameObject {
	   public:
		SimpleGameObject(
			std::shared_ptr<vk::Model> model,
			const glm::vec3& position = glm::vec3(0.0f),
			const glm::vec3& scale = glm::vec3(1.0f))
			: model_(model), position_(position), scale_(scale) {}

		virtual ~SimpleGameObject() = default;

		// GameObject interface
		glm::mat4 computeModelMatrix() const override {
			glm::mat4 T = glm::translate(glm::mat4(1.0f), position_);
			glm::mat4 R = glm::mat4(1.0f);
			glm::mat4 S = glm::scale(glm::mat4(1.0f), scale_);
			return T * R * S;
		}

		glm::mat4 computeNormalMatrix() const override {
			return glm::transpose(glm::inverse(computeModelMatrix()));
		}

		glm::vec3 getPosition() const override {
			return position_;
		}

		std::shared_ptr<vk::Model> getModel() const override {
			return model_;
		}

	   private:
		std::shared_ptr<vk::Model> model_;
		glm::vec3 position_{0.0f};
		glm::vec3 scale_{1.0f};
	};

}  // namespace procedural
