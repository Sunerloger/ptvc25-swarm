#pragma once

#include <glm/glm.hpp>
#include <Jolt/Jolt.h>

glm::mat4 RMat44ToGLM(const JPH::RMat44& joltMat);

glm::vec3 RVec3ToGLM(const JPH::RVec3& joltVec);

JPH::RVec3 GLMToRVec3(const glm::vec3& glmVec);