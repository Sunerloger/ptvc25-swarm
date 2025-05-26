#include "UIComponent.h"
#include "../asset_utils/AssetLoader.h"

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings)
		: model(std::move(settings.model)),
		  name(std::move(settings.name)),
		  controllable(settings.controllable) {
		window = settings.window;
		anchorRight = settings.anchorRight;
		anchorBottom = settings.anchorBottom;
		placeInMiddle = settings.placeInMiddle;
		if ((anchorRight || anchorBottom) && window) {
			Transform t = loadData();
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);

			if (anchorRight)
				offsetFromRight = static_cast<float>(w) - t.pos.x;
			if (anchorBottom)
				offsetFromBottom = static_cast<float>(h) - t.pos.y;
		}
	}

	Transform UIComponent::loadData() const {
		auto iniPath = vk::AssetLoader::getInstance().resolvePath("settings:ui_placements.ini");
		INIReader reader(iniPath);

		Transform t;
		std::string section = "UIComponent_" + name;

		// defaults
		t.pos = {89.9749f, -92.3596f, -97.9395f};
		t.rot = {0.0f, 0.0f, 0.0f, 0.0f};
		t.scale = {5.0f, 5.0f, 5.0};

		if (reader.ParseError() < 0)
			return t;
		if (reader.Get(section, "pos", "") == "")
			return t;

		{
			std::stringstream ss(reader.Get(section, "pos", ""));
			char c;
			ss >> t.pos.x >> c >> t.pos.y >> c >> t.pos.z;
		}
		{
			std::stringstream ss(reader.Get(section, "rot", ""));
			char c;
			ss >> t.rot.x >> c >> t.rot.y >> c >> t.rot.z >> c >> t.rot.w;
		}
		{
			std::stringstream ss(reader.Get(section, "scale", ""));
			char c;
			ss >> t.scale.x >> c >> t.scale.y >> c >> t.scale.z;
		}
		return t;
	}

	void UIComponent::saveData(const Transform &t) const {
		auto iniPath = vk::AssetLoader::getInstance().resolvePath("settings:ui_placements.ini", true);

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

		std::ofstream ofs(iniPath, std::ios::trunc);
		for (auto &l : lines) ofs << l << "\n";

		ofs << header << "\n";
		ofs << "pos=" << t.pos.x << "," << t.pos.y << "," << t.pos.z << "\n";
		ofs << "rot=" << t.rot.x << "," << t.rot.y << "," << t.rot.z << "," << t.rot.w << "\n";
		ofs << "scale=" << t.scale.x << "," << t.scale.y << "," << t.scale.z << "\n";
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		Transform t = loadData();
		glm::vec3 pos = t.pos;

		if (window) {
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);

			if (anchorRight) {
				pos.x = w - t.pos.x;
			}
			if (anchorBottom) {
				pos.y = t.pos.y - h;
			}
			if (placeInMiddle) {
				pos.x = w / 2.0f;
				pos.y = -h / 2.0f;
			}
		}

		glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
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

	void UIComponent::updatePosition(float deltaTime, glm::vec3 dir) {
		if (!controllable || dir == glm::vec3(0.0f))
			return;

		Transform t = loadData();

		float ps = 100.0f * deltaTime;

		t.pos += ps * dir;

		if ((anchorRight || anchorBottom) && window) {
			Transform t = loadData();
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			if (anchorBottom)
				offsetFromBottom = static_cast<float>(h) - t.pos.y;
			if (anchorRight)
				offsetFromRight = static_cast<float>(w) - t.pos.x;
		}
		saveData(t);
	}

	void UIComponent::updateRotation(float deltaTime, glm::vec3 rotDir) {
		if (!controllable || rotDir == glm::vec3(0.0f))
			return;

		Transform t = loadData();

		float rs = 0.1f * deltaTime;

		t.rot = glm::angleAxis(rs, rotDir) * t.rot;

		saveData(t);
	}

	void UIComponent::updateScale(float deltaTime, int scaleDir) {
		if (!controllable || scaleDir == 0)
			return;

		Transform t = loadData();

		float ss = 1.25f * deltaTime;

		t.scale *= (1.0 + scaleDir * ss);
		t.scale = glm::max(t.scale, glm::vec3(0.0001f));

		saveData(t);
	}
}