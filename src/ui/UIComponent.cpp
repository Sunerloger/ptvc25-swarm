#include "UIComponent.h"
#include <fstream>
#include <sstream>
#include "../asset_utils/AssetManager.h"
#include <string>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vk {

	int UIComponent::nextIndex = 0;

	UIComponent::UIComponent(UIComponentCreationSettings settings) : model(settings.model),
																	 controllable(settings.controllable),
																	 position(settings.position),
																	 orientation(glm::radians(settings.rotation)),
																	 scale(settings.scale),
																	 index(nextIndex++) {
		if (controllable) {
			loadData();
		}
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 R = glm::toMat4(orientation);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
		return T * R * S;
	}

	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return this->position;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}

	void UIComponent::updateTransform(float deltaTime, int placementTransform) {
		if (!controllable) {
			return;
		}

		float positionStep = 100.0f * deltaTime;
		float rotationStep = 0.1f * deltaTime;
		float scaleStep = 1.25f * deltaTime;

		switch (placementTransform) {
			case GLFW_KEY_LEFT:
				position.x -= positionStep;
				break;
			case GLFW_KEY_RIGHT:
				position.x += positionStep;
				break;
			case GLFW_KEY_UP:
				position.y += positionStep;
				break;
			case GLFW_KEY_DOWN:
				position.y -= positionStep;
				break;
			case GLFW_KEY_COMMA:
				position.z += positionStep;
				break;
			case GLFW_KEY_PERIOD:
				position.z -= positionStep;
				break;
			case GLFW_KEY_EQUAL:
				scale *= (1.0f + scaleStep);
				break;
			case GLFW_KEY_MINUS:
				scale *= (1.0f - scaleStep);
				scale = glm::max(scale, glm::vec3(0.0001f));
				break;
			case GLFW_KEY_Z:
				orientation = glm::angleAxis(rotationStep, glm::vec3(1, 0, 0)) * orientation;
				break;
			case GLFW_KEY_X:
				orientation = glm::angleAxis(-rotationStep, glm::vec3(1, 0, 0)) * orientation;
				break;
			case GLFW_KEY_C:
				orientation = glm::angleAxis(rotationStep, glm::vec3(0, 1, 0)) * orientation;
				break;
			case GLFW_KEY_V:
				orientation = glm::angleAxis(-rotationStep, glm::vec3(0, 1, 0)) * orientation;
				break;
			case GLFW_KEY_B:
				orientation = glm::angleAxis(rotationStep, glm::vec3(0, 0, 1)) * orientation;
				break;
			case GLFW_KEY_N:
				orientation = glm::angleAxis(-rotationStep, glm::vec3(0, 0, 1)) * orientation;
				break;
			default:
				break;
		}

		saveData();
	}

	void UIComponent::saveData(const std::string& filename) {
		std::string name;
		if (filename.empty()) {
			name = "ui_state_" + std::to_string(index) + ".txt";
		} else {
			name = filename;
		}

		std::stringstream ss;
		ss << position.x << "," << position.y << "," << position.z << ",";
		ss << orientation.x << "," << orientation.y << "," << orientation.z << "," << orientation.w << ",";
		ss << scale.x << "," << scale.y << "," << scale.z;
		AssetManager::getInstance().saveTxtFile("ui/" + name, ss.str());
	}

	void UIComponent::loadData(const std::string& filename) {
		std::string name;
		if (filename.empty()) {
			name = "ui_state_" + std::to_string(index) + ".txt";
		} else {
			name = filename;
		}

		std::string content = AssetManager::getInstance().readTxtFile("ui/" + name);

		if (content.empty())
			return;

		std::vector<float> vals;
		std::stringstream ss(content);
		std::string tok;
		while (std::getline(ss, tok, ',')) {
			try {
				vals.push_back(std::stof(tok));
			} catch (...) {
				return;
			}
		}
		if (vals.size() != 10)
			return;

		position = glm::vec3(vals[0], vals[1], vals[2]);
		orientation = glm::quat(vals[6], vals[3], vals[4], vals[5]);
		scale = glm::vec3(vals[7], vals[8], vals[9]);
	}
}