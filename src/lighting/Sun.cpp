#include "Sun.h"
#include <glm/gtc/matrix_transform.hpp>

namespace lighting {

	Sun::Sun(glm::vec3 position, glm::vec3 direction, glm::vec3 color) : position(position), direction(direction), color(color) {}

	glm::mat4 Sun::computeModelMatrix() const {
		return glm::mat4(1.0f);
	}

	glm::mat4 Sun::computeNormalMatrix() const {
		return glm::mat4(1.0f);
	}

	glm::vec3 Sun::getPosition() const {
		return this->position;
	}

	glm::vec3 Sun::getDirection() const {
		return this->direction;
	}

	void Sun::setDirection(const glm::vec3& newDirection) {
		this->direction = glm::normalize(newDirection);
	}

	void Sun::setPosition(const glm::vec3& newPosition) {
		this->position = newPosition;
	}

	glm::mat4 Sun::computeLightViewMatrix() const {
		glm::vec3 target = position + direction;
		
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		
		// if direction is parallel to up, use a different up vector (would be weird for a sun though in this context)
		if (glm::abs(glm::dot(glm::normalize(direction), up)) > 0.99f) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		
		return glm::lookAt(position, target, up);
	}
}