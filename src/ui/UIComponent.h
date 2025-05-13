// UIComponent.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../GameObject.h"
#include <memory>
#include <string>
// For dynamic anchoring of UI components
#include <GLFW/glfw3.h>

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
		// Optional window handle for anchoring
		GLFWwindow* window = nullptr;
		// If true, position.x is interpreted relative to right edge
		bool anchorRight = false;
		bool anchorBottom = false;
	};

	class UIComponent : public GameObject {
	   public:
		UIComponent(UIComponentCreationSettings settings);
		virtual ~UIComponent() = default;

		// only re-load & save when handling placement keys
		void updateTransform(float deltaTime, int placementTransform = -1);

		// always read from disk / defaults
		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<Model> getModel() const override;
		bool isControllable() const {
			return controllable;
		}

	   protected:
		/**
		 * Update the underlying model for this UI component.  Subclasses may rebuild the mesh
		 * and upload a new Model instance.
		 */
		void setModel(std::shared_ptr<Model> m) {
			model = std::move(m);
		}

	   private:
		// Load/write transform (pos, rot, scale) from INI
		Transform loadData() const;
		void saveData(const Transform& t) const;

		std::shared_ptr<Model> model;
		std::string name;
		bool controllable;
		// Optional window handle for dynamic anchoring
		GLFWwindow* window = nullptr;
		// If true, pos.x is anchored from right edge using offsetFromRight
		bool anchorRight = false;
		float offsetFromRight = 0.0f;
		bool anchorBottom = false;
		float offsetFromBottom = 0.0f;
	};

}  // namespace vk