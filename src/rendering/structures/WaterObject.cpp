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

	void WaterObject::toggleWireframeModeIfSupported(bool toWireframe) {
		vk::PipelineConfigInfo& configInfo = this->modelPtr->getMaterial()->getPipelineConfigRef();
		if (toWireframe) {
			configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
		} else {
			configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		}
	}
}