#include "UIComponent.h"
#include <iostream>
#include <fstream>

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings)
		: model(settings.model),
		  rotation(settings.rotation),
		  objectX(settings.objectX),
		  objectY(settings.objectY),
		  objectZ(settings.objectZ),
		  objectWidth(settings.objectWidth),
		  objectHeight(settings.objectHeight),
		  windowWidth(settings.windowWidth),
		  windowHeight(settings.windowHeight),
		  usePerspectiveProjection(settings.usePerspectiveProjection) {}

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
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}

	void saveModelMatrix(const glm::mat4& matrix, const std::string& filename) {
		std::ofstream file(filename);
		if (file.is_open()) {
			// write the matrix to the file
			// it should be written so that someone can copy-paste it directly as c++ code
			file << "glm::mat4(";
			file << matrix[0][0] << ", " << matrix[0][1] << ", " << matrix[0][2] << ", " << matrix[0][3] << ", ";
			file << matrix[1][0] << ", " << matrix[1][1] << ", " << matrix[1][2] << ", " << matrix[1][3] << ", ";
			file << matrix[2][0] << ", " << matrix[2][1] << ", " << matrix[2][2] << ", " << matrix[2][3] << ", ";
			file << matrix[3][0] << ", " << matrix[3][1] << ", " << matrix[3][2] << ", " << matrix[3][3] << ");";
			file << std::endl;
			file.close();
		} else {
			std::cerr << "Unable to open file: " << filename << std::endl;
		}
	}

	glm::mat4 UIComponent::computeModelMatrix(glm::mat4 transform) const {
		// For some reason the z index needs to be bigger than 0.0f to correctly show the hud
		// I assume this is because the z index is used to sort the objects in the scene
		// and if it is smaller, then then HUD is draw behind the player

		if (modelMatrix != glm::mat4(1.0f)) {
			modelMatrix = transform * modelMatrix;
			saveModelMatrix(modelMatrix, "model_matrix.txt");
			return modelMatrix;
		}

		if (usePerspectiveProjection == 0) {
			glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(objectX, -objectY, objectZ));

			glm::mat4 pivotOffset = glm::translate(glm::mat4(1.0f),
				glm::vec3(objectWidth * 0.5f, -objectHeight * 0.5f, 0.0f));

			glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 R = Rz * Ry * Rx;

			glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(objectWidth, objectHeight, 1.0f));

			return T * pivotOffset * R * S;
		} else {
			// Translation to put the element at calculated world-space coordinates;
			// note: we translate along -Z to move it into view.
			glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(objectX, -objectY, objectZ));

			glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 R = Rz * Ry * Rx;

			glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(objectWidth, objectHeight, 1.0f));

			modelMatrix = T * R * S;
			return modelMatrix;
		}
	}

}