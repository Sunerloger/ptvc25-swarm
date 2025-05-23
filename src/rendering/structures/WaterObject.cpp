#include "WaterObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vk {
	WaterObject::WaterObject(std::shared_ptr<Model> m, WaterCreationSettings waterCreationSettings) : modelPtr(m) {
		transformMat = glm::translate(glm::mat4(1.0f), waterCreationSettings.position) * glm::scale(glm::mat4(1.0f), glm::vec3(waterCreationSettings.waterScale, 1.0f, waterCreationSettings.waterScale));
	}

	glm::mat4 WaterObject::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(transformMat));
	}
}