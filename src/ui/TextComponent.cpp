#include "TextComponent.h"
#include <algorithm>

namespace vk {

	TextComponent::TextComponent(Device &device,
		Font &font,
		const std::string &initialText,
		const std::string &name,
		bool controllable,
		bool centerHorizontal,
		float horizontalOffset,
		bool centerVertical,
		float verticalOffset,
		bool anchorRight,
		bool anchorBottom,
		bool isDebugMenuComponent,
		GLFWwindow *window)
		: UIComponent(UIComponentCreationSettings{
			  /*model*/ nullptr,
			  /*name*/ name,
			  /*controllable*/ controllable,
			  /*window*/ window,
			  /*anchorRight*/ anchorRight,
			  /*anchorBottom*/ anchorBottom,
			  /*centerHorizontal*/ centerHorizontal,
			  /*centerVertical*/ centerVertical,
			  /*isDebugMenuComponent*/ isDebugMenuComponent}),
		  device(device),
		  font(font),
		  textStr(initialText),
		  horizontalOffset(horizontalOffset),
		  verticalOffset(verticalOffset) {
		std::vector<unsigned char> white = {255, 255, 255, 255};
		material = std::make_shared<UIMaterial>(device, white, 1, 1, 4);
		rebuildMesh();
	}

	void TextComponent::setText(const std::string &text) {
		if (text != textStr) {
			textStr = text;
			rebuildMesh();
		}
	}

	void TextComponent::rebuildMesh() {
		std::vector<Model::Vertex> verts;
		std::vector<uint32_t> inds;
		font.buildTextMesh(textStr, verts, inds, 2.0f);

		if (verts.size() < 3) {
			setModel(nullptr);
			textSize = {0, 0};
			return;
		}

		float minX = verts[0].position.x, maxX = minX;
		float minY = verts[0].position.y, maxY = minY;
		for (auto &v : verts) {
			minX = std::min(minX, v.position.x);
			maxX = std::max(maxX, v.position.x);
			minY = std::min(minY, v.position.y);
			maxY = std::max(maxY, v.position.y);
		}
		textSize = {maxX - minX, maxY - minY};

		Model::Builder b;
		b.vertices = std::move(verts);
		b.indices = std::move(inds);
		b.isUI = true;
		auto mdl = std::make_shared<Model>(device, b);
		mdl->setMaterial(material);
		setModel(mdl);
	}

	glm::mat4 TextComponent::computeModelMatrix() const {
		Transform t = getTransformData();
		glm::vec3 pos = t.pos;

		if (auto wnd = getWindowPtr()) {
			int w, h;
			glfwGetFramebufferSize(wnd, &w, &h);
			if (anchorRight)
				pos.x = w - t.pos.x;
			if (anchorBottom)
				pos.y = t.pos.y - h;
			if (getCenterHorizontal()) {
				pos.x = (w / 2.0f - (textSize.x * t.scale.x) * 0.5f) + horizontalOffset;
			}
			if (getCenterVertical()) {
				pos.y = (-h / 2.0f + (textSize.y * t.scale.y) * 0.5f) + verticalOffset;
			}
		}

		glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
		glm::mat4 R = glm::toMat4(t.rot);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
		return T * R * S;
	}

}  // namespace vk