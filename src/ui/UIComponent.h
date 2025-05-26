#pragma once

#include "../GameObject.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>
#include "INIReader.h"

namespace vk {

	struct Transform {
		glm::vec3 pos;
		glm::quat rot;
		glm::vec3 scale;
	};

	class UIComponentCreationSettings {
	   public:
		std::shared_ptr<Model> model;
		std::string name;
		bool controllable = false;
		GLFWwindow* window = nullptr;
		bool anchorRight = false;
		bool anchorBottom = false;
		bool placeInMiddle = false;
	};

	class UIComponent : public GameObject {
	   public:
		UIComponent(UIComponentCreationSettings settings);
		virtual ~UIComponent() = default;

		void updatePosition(float deltaTime, glm::vec3 dir);
		void updateRotation(float deltaTime, glm::vec3 rotDir);
		void updateScale(float deltaTime, int scaleDir);

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<Model> getModel() const override;

		bool isControllable() const {
			return controllable;
		}

	   protected:
		// expose placement data to subclasses
		Transform getTransformData() const {
			return loadData();
		}
		GLFWwindow* getWindowPtr() const {
			return window;
		}
		bool getPlaceInMiddle() const {
			return placeInMiddle;
		}
		// allow subclasses to set the mesh
		void setModel(std::shared_ptr<Model> m) {
			model = std::move(m);
		}

	   private:
		Transform loadData() const;
		void saveData(const Transform& t) const;

		std::shared_ptr<Model> model;
		std::string name;
		bool controllable;
		GLFWwindow* window = nullptr;
		bool anchorRight = false;
		float offsetFromRight = 0.0f;
		bool anchorBottom = false;
		float offsetFromBottom = 0.0f;
		bool placeInMiddle = false;
	};

}  // namespace vk