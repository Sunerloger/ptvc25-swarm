#pragma once

#include <glm/glm.hpp>

struct MovementIntent {
	glm::vec3 direction;
	bool jump;
};