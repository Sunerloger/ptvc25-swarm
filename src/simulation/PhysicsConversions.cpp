#include "PhysicsConversions.h"

glm::mat4 RMat44ToGLM(const JPH::RMat44& joltMat) {
	glm::mat4 glmMat = {};
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			glmMat[col][row] = joltMat(row, col);
		}
	}
	return glmMat;
}

glm::vec3 RVec3ToGLM(const JPH::RVec3& joltVec) {
	return glm::vec3(joltVec.GetX(), joltVec.GetY(), joltVec.GetZ());
}

JPH::RVec3 GLMToRVec3(const glm::vec3& glmVec) {
	return JPH::RVec3(glmVec.x, glmVec.y, glmVec.z);
}

glm::quat QuatToGLM(const JPH::Quat& joltQuat) {
	return glm::quat(joltQuat.GetW(), joltQuat.GetX(), joltQuat.GetY(), joltQuat.GetZ());
}