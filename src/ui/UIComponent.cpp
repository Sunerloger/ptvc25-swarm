// UIComponent.cpp
#include "UIComponent.h"
#include "INIReader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <glm/gtx/quaternion.hpp>

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings)
		: model(std::move(settings.model)),
		  name(std::move(settings.name)),
		  controllable(settings.controllable) {
		// on construction: if controllable, ensure an .ini entry exists
		if (controllable) {
			Transform t = loadData();  // will write defaults if missing
			saveData(t);
		}
	}

	Transform UIComponent::loadData() const {
		auto iniPath = std::filesystem::current_path() / "ui_placements.ini";
		INIReader reader(iniPath.string());

		Transform t;
		std::string section = "UIComponent_" + name;

		// defaults
		t.pos = {89.9749f, -92.3596f, -97.9395f};
		t.rot = {0.504159f, 0.495496f, 0.498390f, 0.501947f};  // w,x,y,z
		t.scale = {182.921f, 182.921f, 182.921f};

		if (reader.ParseError() < 0)
			return t;
		if (reader.Get(section, "pos", "") == "")
			return t;

		// parse pos
		{
			std::stringstream ss(reader.Get(section, "pos", ""));
			char c;
			ss >> t.pos.x >> c >> t.pos.y >> c >> t.pos.z;
		}
		// parse rot
		{
			std::stringstream ss(reader.Get(section, "rot", ""));
			char c;
			ss >> t.rot.x >> c >> t.rot.y >> c >> t.rot.z >> c >> t.rot.w;
		}
		// parse scale
		{
			std::stringstream ss(reader.Get(section, "scale", ""));
			char c;
			ss >> t.scale.x >> c >> t.scale.y >> c >> t.scale.z;
		}
		return t;
	}

	void UIComponent::saveData(const Transform &t) const {
		auto iniPath = std::filesystem::current_path() / "ui_placements.ini";

		// read all lines except our section
		std::vector<std::string> lines;
		std::ifstream ifs(iniPath);
		std::string header = "[UIComponent_" + name + "]";
		std::string line;
		bool skipping = false;
		while (std::getline(ifs, line)) {
			if (line == header) {
				skipping = true;
				continue;
			}
			if (skipping && !line.empty() && line.front() == '[')
				skipping = false;
			if (!skipping)
				lines.push_back(line);
		}
		ifs.close();

		// write back + our section
		std::ofstream ofs(iniPath, std::ios::trunc);
		for (auto &l : lines) ofs << l << "\n";

		ofs << header << "\n";
		ofs << "pos=" << t.pos.x << "," << t.pos.y << "," << t.pos.z << "\n";
		ofs << "rot=" << t.rot.x << "," << t.rot.y << "," << t.rot.z << "," << t.rot.w << "\n";
		ofs << "scale=" << t.scale.x << "," << t.scale.y << "," << t.scale.z << "\n";
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		Transform t = loadData();
		glm::mat4 T = glm::translate(glm::mat4(1.0f), t.pos);
		glm::mat4 R = glm::toMat4(t.rot);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
		return T * R * S;
	}

	glm::mat4 UIComponent::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(computeModelMatrix()));
	}

	glm::vec3 UIComponent::getPosition() const {
		return loadData().pos;
	}

	std::shared_ptr<Model> UIComponent::getModel() const {
		return model;
	}

	void UIComponent::updateTransform(float deltaTime, int placementTransform) {
		if (!controllable || placementTransform == -1)
			return;

		Transform t = loadData();

		float ps = 100.0f * deltaTime;
		float rs = 0.1f * deltaTime;
		float ss = 1.25f * deltaTime;

		switch (placementTransform) {
			case GLFW_KEY_LEFT:
				t.pos.x -= ps;
				break;
			case GLFW_KEY_RIGHT:
				t.pos.x += ps;
				break;
			case GLFW_KEY_UP:
				t.pos.y += ps;
				break;
			case GLFW_KEY_DOWN:
				t.pos.y -= ps;
				break;
			case GLFW_KEY_COMMA:
				t.pos.z += ps;
				break;
			case GLFW_KEY_PERIOD:
				t.pos.z -= ps;
				break;

			case GLFW_KEY_EQUAL:
				t.scale *= (1.0f + ss);
				break;
			case GLFW_KEY_MINUS:
				t.scale *= (1.0f - ss);
				t.scale = glm::max(t.scale, glm::vec3(0.0001f));
				break;

			case GLFW_KEY_Z:
				t.rot = glm::angleAxis(rs, glm::vec3(1, 0, 0)) * t.rot;
				break;
			case GLFW_KEY_X:
				t.rot = glm::angleAxis(-rs, glm::vec3(1, 0, 0)) * t.rot;
				break;
			case GLFW_KEY_C:
				t.rot = glm::angleAxis(rs, glm::vec3(0, 1, 0)) * t.rot;
				break;
			case GLFW_KEY_V:
				t.rot = glm::angleAxis(-rs, glm::vec3(0, 1, 0)) * t.rot;
				break;
			case GLFW_KEY_B:
				t.rot = glm::angleAxis(rs, glm::vec3(0, 0, 1)) * t.rot;
				break;
			case GLFW_KEY_N:
				t.rot = glm::angleAxis(-rs, glm::vec3(0, 0, 1)) * t.rot;
				break;
			default:
				break;
		}

		saveData(t);
	}

}  // namespace vk