#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../GameObject.h"

namespace vk {
	class UIComponentCreationSettings {
	   public:
		std::shared_ptr<Model> model;

		glm::vec2 position;
		glm::vec3 scale;

		// angle around axis in radians (pitch, yaw, roll)
		glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);

		float windowWidth = 1.0f;
		float windowHeight = 1.0f;

		// specifies if there is a file to read from / write to
		bool controllable = false;
	};

	class UIComponent : public GameObject {
	   public:
		UIComponent(UIComponentCreationSettings settings);
		virtual ~UIComponent() = default;

		void updateTransform(int placementTransform = -1);

		glm::mat4 computeModelMatrix() const override;

		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<Model> getModel() const override;
		void updateWindowDimensions(float screenWidth, float screenHeight);
		
		// if no filename use ui_state_index.txt
		void saveData(const std::string& filename = "");
		void loadData(const std::string& filename = "");

		static int nextIndex;

	   private:
		std::shared_ptr<Model> model;

		glm::vec2 position;
		glm::quat orientation;
		glm::vec3 scale;

		float windowWidth;
		float windowHeight;

		bool controllable = false;

		int index;
	};
}