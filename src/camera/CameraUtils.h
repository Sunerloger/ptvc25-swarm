#pragma once

#include <glm/glm.hpp>

// flips y axis
glm::mat4 getOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

// flips y axis
glm::mat4 getPerspectiveProjection(float fov, float aspect, float near, float far);

glm::mat4 getVulkanAxisInversionMatrix();