#include "PointLight.h"

#include <glm/gtc/matrix_transform.hpp>

namespace lighting {
	
	PointLight::PointLight(glm::vec3 color, glm::vec3 position) : position(position), color(color) {}

	glm::mat4 PointLight::computeModelMatrix() const {
		glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);

		glm::mat4 modelMat = translateMat;
		return modelMat;
	}

	glm::mat4 PointLight::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 PointLight::getPosition() const {
		return this->position;
	}

	void PointLight::setPosition(glm::vec3 newPosition) {
		this->position = newPosition;
	}
}