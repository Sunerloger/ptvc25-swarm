#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../GameObject.h"

namespace vk {
	class UIComponentCreationSettings {
	   public:
		std::shared_ptr<Model> model;
		glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);

		float objectX = 0.0f;
		float objectY = 0.0f;
		float objectZ = -5.0f;

		float objectWidth = 1.0f;
		float objectHeight = 1.0f;

		float windowWidth = 1.0f;
		float windowHeight = 1.0f;

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		bool controllable = false;
	};

	class UIComponent : public GameObject {
	   public:
		UIComponent(UIComponentCreationSettings settings);
		virtual ~UIComponent() = default;

		glm::mat4 computeModelMatrix(int placementTransform) const;
		glm::mat4 computeModelMatrix() const override {
			return computeModelMatrix(-1);
		}
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<Model> getModel() const override;
		glm::vec3 getScale() const;
		void updateWindowDimensions(float screenWidth, float screenHeight);

	   private:
		std::shared_ptr<Model> model;
		glm::vec3 rotation;

		float objectX;
		float objectY;
		float objectZ = -5.0f;

		float objectWidth;
		float objectHeight;

		float windowWidth;
		float windowHeight;

		mutable glm::mat4 modelMatrix = glm::mat4(1.0f);
		bool controllable = false;
	};
}