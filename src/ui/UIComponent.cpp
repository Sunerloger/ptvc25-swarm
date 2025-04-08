#include "UIComponent.h"
#include <iostream>

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings)
		: model(settings.model),
		  rotation(settings.rotation),
		  objectX(settings.objectX),
		  objectY(settings.objectY),
		  objectWidth(settings.objectWidth),
		  objectHeight(settings.objectHeight),
		  windowWidth(settings.windowWidth),
		  windowHeight(settings.windowHeight) {}

	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return glm::vec3(objectX, objectY, 0.0f);
	}

	glm::vec3 UIComponent::getScale() const {
		glm::vec4 scaleX = computeModelMatrix() * glm::vec4(1, 0, 0, 0);
		glm::vec4 scaleY = computeModelMatrix() * glm::vec4(0, 1, 0, 0);
		return glm::vec3(glm::length(glm::vec3(scaleX)), glm::length(glm::vec3(scaleY)), 1.0f);
	}

	void UIComponent::updateWindowDimensions(float windowWidth, float windowHeight) {
		std::cout << "UIComponent: window dimensions updated from "
				  << this->windowWidth << "x" << this->windowHeight << std::endl;
		std::cout << "UIComponent: window dimensions updated to "
				  << windowWidth << "x" << windowHeight << std::endl;
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		float ndcX = (objectX / windowWidth) * 2.0f - 1.0f;
		float ndcY = 1.0f - (objectY / windowHeight) * 2.0f;

		float ndcWidth = (objectWidth / windowWidth) * 2.0f;
		float ndcHeight = (objectHeight / windowHeight) * 2.0f;

		float aspectCorrection = windowWidth / windowHeight;

		glm::mat4 translation;
		glm::mat4 scale;
		glm::mat4 pivotOffset;
		std::cout << aspectCorrection << std::endl;
		if (aspectCorrection < 1.0f) {
			translation = glm::translate(glm::mat4(1.0f), glm::vec3(ndcX, ndcY / aspectCorrection, 0.0f));
			scale = glm::scale(glm::mat4(1.0f),
				glm::vec3(ndcWidth, ndcHeight / aspectCorrection, 1.0f));
			pivotOffset = glm::translate(glm::mat4(1.0f),
				glm::vec3(ndcWidth * 0.5f, -ndcHeight * 0.5f / aspectCorrection, 0.0f));
		} else {
			translation = glm::translate(glm::mat4(1.0f), glm::vec3(ndcX * aspectCorrection, ndcY, 0.0f));
			scale = glm::scale(glm::mat4(1.0f),
				glm::vec3(ndcWidth * aspectCorrection, ndcHeight, 1.0f));
			pivotOffset = glm::translate(glm::mat4(1.0f),
				glm::vec3(ndcWidth * 0.5f * aspectCorrection, -ndcHeight * 0.5f, 0.0f));
		}

		return translation *
			   pivotOffset * scale;
	}
}