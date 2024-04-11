#include "PointLight.h"

namespace lighting {
	
	PointLight::PointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3{ 1.0f }) : GameObject() {
		this->lightIntensity = intensity;
		this->color = color;
		this->scale.x = radius;
	}

	glm::mat4 PointLight::computeModelMatrix() {
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
		glm::mat4 rotXMat = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
		glm::mat4 rotYMat = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
		glm::mat4 rotZMat = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));
		glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), translation);

		// Matrix corresponds to Translate Rz * Ry * Rx * Scale
		glm::mat4 modelMat = translateMat * rotZMat * rotYMat * rotXMat * scaleMat;
		return modelMat;
	}

	glm::mat4 PointLight::computeNormalMatrix() {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}
}