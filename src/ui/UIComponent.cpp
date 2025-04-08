#include "UIComponent.h"

namespace vk {
	UIComponent::UIComponent(std::shared_ptr<Model> model, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation) : model(model),
																													  position(position),
																													  scale(scale),
																													  rotation(rotation) {
	}

	UIComponent::UIComponent(std::shared_ptr<Model> model, glm::mat4 modelMatrix) : model(model) {
		this->position = glm::vec3(modelMatrix[3]);
		this->scale = glm::vec3(glm::length(modelMatrix[0]), glm::length(modelMatrix[1]), glm::length(modelMatrix[2]));
		this->rotation = glm::vec3(0.0f);
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		glm::mat4 model_mat = glm::mat4(1.0f);

		model_mat = glm::translate(model_mat, position);

		model_mat = glm::rotate(model_mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));	// roll
		model_mat = glm::rotate(model_mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));	// pitch
		model_mat = glm::rotate(model_mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));	// yaw

		model_mat = glm::scale(model_mat, scale);

		return model_mat;
	}

	// although probably not necessary for UI
	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return this->position;
	}

	glm::vec3 UIComponent::getScale() const {
		return this->scale;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}
}