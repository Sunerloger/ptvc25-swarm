#include "UIComponent.h"
#include <iostream>

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

	glm::mat4 UIComponent::computeModelMatrix() const {
		// For some reason the z index needs to be bigger than 0.0f to correctly show the hud
		// I assume this is because the z index is used to sort the objects in the scene
		// and if it is smaller, then then HUD is draw behind the player
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
			// --- Perspective branch ---
			// Choose a fixed depth (d) for the perspective UI element.
			const float d = objectZ;  // Adjust as needed.
			const float fov = glm::radians(45.0f);
			float aspect = windowWidth / windowHeight;
			// Compute visible world dimensions at depth d.
			float visibleHeight = 2.0f * d * tan(fov / 2.0f);
			float visibleWidth = visibleHeight * aspect;

			// Convert objectX, objectY (pixels) to normalized device coordinates.
			// Assuming (0,0) is top-left.
			float ndcX = (objectX / windowWidth) * 2.0f - 1.0f;
			float ndcY = 1.0f - (objectY / windowHeight) * 2.0f;

			// Map NDC to world space at depth d.
			float worldX = ndcX * (visibleWidth / 2.0f);
			float worldY = ndcY * (visibleHeight / 2.0f);

			// Translation to put the element at calculated world-space coordinates;
			// note: we translate along -Z to move it into view.
			glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(worldX, worldY, -d));

			// Build rotation matrix from component rotations.
			glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));
			glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0));
			glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));
			glm::mat4 R = Rz * Ry * Rx;

			// Scaling: objectWidth and objectHeight are now interpreted in world–space units.
			glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(objectWidth, objectHeight, 1.0f));

			return T * R * S;
		}
	}

}