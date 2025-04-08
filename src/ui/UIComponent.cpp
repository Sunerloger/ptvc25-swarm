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
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(objectX, objectY, 0.0f));

		glm::mat4 pivotOffset = glm::translate(glm::mat4(1.0f),
			glm::vec3(objectWidth * 0.5f, -objectHeight * 0.5f, 0.0f));

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(objectWidth, objectHeight, 1.0f));

		return translate * pivotOffset * scale;
	}

}