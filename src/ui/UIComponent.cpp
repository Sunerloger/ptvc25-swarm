#include "UIComponent.h"
#include "../asset_utils/AssetLoader.h"
#include <fstream>
#include <sstream>

namespace vk {

	UIComponent::UIComponent(UIComponentCreationSettings settings)
		: model(std::move(settings.model)),
		  name(std::move(settings.name)),
		  controllable(settings.controllable),
		  window(settings.window),
		  anchorRight(settings.anchorRight),
		  anchorBottom(settings.anchorBottom),
		  centerHorizontal(settings.centerHorizontal),
		  centerVertical(settings.centerVertical),
		  isDebugMenuComponent(settings.isDebugMenuComponent) {
		loadData();

		if ((anchorRight || anchorBottom) && window) {
			Transform t = cachedTransform;
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			if (anchorRight)
				offsetFromRight = float(w) - t.pos.x;
			if (anchorBottom)
				offsetFromBottom = float(h) - t.pos.y;
		}
	}

	Transform UIComponent::loadData() const {
		if (cacheValid) {
			return cachedTransform;
		}

		auto iniPath = AssetLoader::getInstance().resolvePath("settings:ui_placements.ini");
		INIReader reader(iniPath);

		Transform t{};
		t.pos = glm::vec3{0.0f};
		t.rot = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
		t.scale = glm::vec3{1.0f};

		std::string sect = "UIComponent_" + name;
		if (reader.ParseError() < 0 || reader.Get(sect, "pos", "").empty()) {
			cachedTransform = t;
			cacheValid = true;
			return t;
		}

		{
			std::stringstream ss(reader.Get(sect, "pos", ""));
			char c;
			ss >> t.pos.x >> c >> t.pos.y >> c >> t.pos.z;
		}
		{
			std::stringstream ss(reader.Get(sect, "rot", ""));
			char c;
			ss >> t.rot.x >> c >> t.rot.y >> c >> t.rot.z >> c >> t.rot.w;
		}
		{
			std::stringstream ss(reader.Get(sect, "scale", ""));
			char c;
			ss >> t.scale.x >> c >> t.scale.y >> c >> t.scale.z;
		}

		cachedTransform = t;
		cacheValid = true;
		return t;
	}

	void UIComponent::saveData(const Transform &t) const {
		auto iniPath = AssetLoader::getInstance().resolvePath("settings:ui_placements.ini", true);
		std::vector<std::string> lines;
		std::ifstream ifs(iniPath);
		std::string header = "[UIComponent_" + name + "]";
		bool skipping = false;
		std::string line;
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

		cachedTransform = t;
		cacheValid = true;
	}

	void UIComponent::invalidateCache() {
		cacheValid = false;
	}

	glm::mat4 UIComponent::computeModelMatrix() const {
		Transform t = loadData();
		glm::vec3 pos = t.pos;

		if (window) {
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			if (anchorRight)
				pos.x = w - t.pos.x;
			if (anchorBottom)
				pos.y = t.pos.y - h;
			if (centerHorizontal)
				pos.x = w / 2.0f;
			if (centerVertical)
				pos.y = -h / 2.0f;
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

	void UIComponent::updatePosition(float dt, glm::vec3 dir) {
		if (!controllable || dir == glm::vec3{0.0f})
			return;
		Transform t = loadData();
		t.pos += dir * (100.0f * dt);
		saveData(t);
	}

	void UIComponent::updateRotation(float dt, glm::vec3 rotDir) {
		if (!controllable || rotDir == glm::vec3{0.0f})
			return;
		Transform t = loadData();
		t.rot = glm::angleAxis(0.1f * dt, rotDir) * t.rot;
		saveData(t);
	}

	void UIComponent::updateScale(float dt, int scaleDir) {
		if (!controllable || scaleDir == 0)
			return;
		Transform t = loadData();
		t.scale *= (1.0f + scaleDir * 1.25f * dt);
		saveData(t);
	}

}  // namespace vk