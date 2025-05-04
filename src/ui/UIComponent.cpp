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
		windowWidth(settings.windowWidth),
		windowHeight(settings.windowHeight),
		controllable(settings.controllable),
		position(settings.position),
		orientation(settings.rotation),
		scale(settings.scale),
		index(nextIndex++)
	{
		if (controllable) {
			loadData();
		}
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
		glm::mat4 R = glm::toMat4(orientation);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
		return T * R * S;
	}

	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return glm::vec3(position, 0.0f);
	}

	void UIComponent::updateWindowDimensions(float windowWidth, float windowHeight) {
		this->windowWidth = windowWidth;
		this->windowHeight = windowHeight;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return this->model;
	}

	void UIComponent::updateTransform(int placementTransform) {
		if (!controllable) {return;}

		float step = 1.0f;
		switch (placementTransform) {
		case GLFW_KEY_LEFT:
			position.x -= step;
			break;
		case GLFW_KEY_RIGHT:
			position.x += step;
			break;
		case GLFW_KEY_UP:
			position.y -= step;
			break;
		case GLFW_KEY_DOWN:
			position.y += step;
			break;
		case GLFW_KEY_EQUAL:
			scale *= (1.0f + step);
			break;
		case GLFW_KEY_MINUS:
			scale *= (1.0f - step);
			scale = glm::max(scale, glm::vec3(0.0001f));
			break;
		case GLFW_KEY_Z:
			orientation = orientation * glm::angleAxis(step, glm::vec3(1, 0, 0));
			break;
		case GLFW_KEY_X:
			orientation = orientation * glm::angleAxis(-step, glm::vec3(1, 0, 0));
			break;
		case GLFW_KEY_C:
			orientation = orientation * glm::angleAxis(step, glm::vec3(0, 1, 0));
			break;
		case GLFW_KEY_V:
			orientation = orientation * glm::angleAxis(-step, glm::vec3(0, 1, 0));
			break;
		case GLFW_KEY_B:
			orientation = orientation * glm::angleAxis(step, glm::vec3(0, 0, 1));
			break;
		case GLFW_KEY_N:
			orientation = orientation * glm::angleAxis(-step, glm::vec3(0, 0, 1));
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
		}
		else {
			name = filename;
		}

		std::stringstream ss;
		ss << position.x << "," << position.y << ",";
		ss << orientation.x << "," << orientation.y << "," << orientation.z << "," << orientation.w << ",";
		ss << scale.x << "," << scale.y << "," << scale.z;
		AssetManager::getInstance().saveTxtFile("ui/" + name, ss.str());
	}
	
	void UIComponent::loadData(const std::string& filename) {

		std::string name;
		if (filename.empty()) {
			name = "ui_state_" + std::to_string(index) + ".txt";
		}
		else {
			name = filename;
		}

		std::string content = AssetManager::getInstance().readTxtFile("ui/" + name);

		if (content.empty()) return;

		std::vector<float> vals;
		std::stringstream ss(content);
		std::string tok;
		while (std::getline(ss, tok, ',')) {
			try { vals.push_back(std::stof(tok)); }
			catch (...) { return; }
		}
		if (vals.size() != 9) return;

		position = glm::vec2(vals[0], vals[1]);
		orientation = glm::quat(vals[5], vals[2], vals[3], vals[4]);
		scale = glm::vec3(vals[6], vals[7], vals[8]);
	}
}