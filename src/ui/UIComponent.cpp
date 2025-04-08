#include "UIComponent.h"

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings) {
		this->model = settings.model;
		this->rotation = settings.rotation;
		this->posX = settings.posX;
		this->posY = settings.posY;
		this->width = settings.width;
		this->height = settings.height;
		this->windowWidth = settings.windowWidth;
		this->windowHeight = settings.windowHeight;
	}

	void UIComponent::updateWindowDimension(float screenWidth, float screenHeight) {
		this->windowWidth = screenWidth;
		this->windowHeight = screenHeight;
	}

	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return glm::vec3();
	}

	glm::vec3 UIComponent::getScale() const {
		return glm::vec3();
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		float ndcX = (this->posX / this->windowWidth) * 2.0f - 1.0f;
		float ndcY = 1.0f - (this->posY / this->windowHeight) * 2.0f;

		float ndcWidth = (this->width / this->windowWidth) * 2.0f;
		float ndcHeight = (this->height / this->windowHeight) * 2.0f;

		float aspectCorrection = this->windowWidth / this->windowHeight;

		glm::mat4 translation;
		if (aspectCorrection < 1.0f) {
			translation = glm::translate(glm::mat4(1.0f), glm::vec3(ndcX, ndcY / aspectCorrection, 0.0f));
		} else {
			translation = glm::translate(glm::mat4(1.0f), glm::vec3(ndcX * aspectCorrection, ndcY, 0.0f));
		}
		glm::mat4 pivotOffset = glm::translate(glm::mat4(1.0f),
			glm::vec3(ndcWidth * 0.5f * aspectCorrection, -ndcHeight * 0.5f, 0.0f));
		glm::mat4 scale = glm::scale(glm::mat4(1.0f),
			glm::vec3(ndcWidth * aspectCorrection, ndcHeight, 1.0f));

		return translation * pivotOffset * scale;
	}
}