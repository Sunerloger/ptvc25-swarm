#include "Sun.h"

namespace lighting {

	Sun::Sun(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float intensity) : position(position), direction(direction), intensity(intensity) {
		this->color = color;
	}

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
}