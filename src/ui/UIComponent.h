#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../GameObject.h"

namespace vk {
	class UIComponentCreationSettings {
	   public:
		std::shared_ptr<Model> model;
		glm::vec3 rotation;

		float objectX;
		float objectY;

		float objectWidth;
		float objectHeight;

		float windowWidth;
		float windowHeight;
	};

	class UIComponent : public GameObject {
	   public:
		UIComponent(UIComponentCreationSettings settings);
		virtual ~UIComponent() = default;

		glm::mat4 computeModelMatrix() const override;
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

		float objectWidth;
		float objectHeight;

		float windowWidth;
		float windowHeight;
	};
}