#include "PointLight.h"

namespace lighting {
	
	PointLight::PointLight(float intensity, float radius, glm::vec3 color, glm::vec3 position) : lightIntensity(intensity), radius(radius), position(position) {
		this->color = color;
	}

	glm::mat4 PointLight::computeModelMatrix() const {
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
		glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);

		glm::mat4 modelMat = translateMat * scaleMat;
		return modelMat;
	}

	glm::mat4 PointLight::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 PointLight::getPosition() const {
		return this->position;
	}

	float PointLight::getIntensity() const {
		return this->lightIntensity;
	}

	float PointLight::getRadius() const {
		return this->radius;
	}

	void PointLight::setPosition(glm::vec3 newPosition) {
		this->position = newPosition;
	}
}