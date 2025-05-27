#pragma once

#include "../../GameObject.h"

namespace vk {
	class WaterObject : public GameObject {

	public:

		struct WaterCreationSettings {
			float waterScale = 2000.0f;
			glm::vec3 position = glm::vec3{ 0.0f, -10.0f, 0.0f };
		};

		WaterObject(std::shared_ptr<Model> m, WaterCreationSettings waterCreationSettings);

		glm::mat4 computeModelMatrix() const override {
			return transformMat;
		}

		glm::mat4 computeNormalMatrix() const override;

		glm::vec3 getPosition() const override {
			return glm::vec3(transformMat[3]);
		}

		std::shared_ptr<Model> getModel() const override {
			return modelPtr;
		}

	private:

		std::shared_ptr<Model> modelPtr;
		glm::mat4 transformMat;
	};
}