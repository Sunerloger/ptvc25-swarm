#include "UIComponent.h"

namespace vk {
	UIComponent::UIComponent(Model* model, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, bool isDrawLines, bool isEscapeMenu, glm::vec3 color) : model(model), 
		position(position), scale(scale), rotation(rotation), isDrawLines(isDrawLines), isEscapeMenu(isEscapeMenu) {
		this->color = color;
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		glm::mat4 model_mat = glm::mat4(1.0f);

		model_mat = glm::translate(model_mat, position);

		model_mat = glm::rotate(model_mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // roll
		model_mat = glm::rotate(model_mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // pitch
		model_mat = glm::rotate(model_mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // yaw

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

	Model* UIComponent::getModel() const {
		return this->model;
	}
}