#include "TextComponent.h"
#include <GLFW/glfw3.h>

namespace vk {

	TextComponent::TextComponent(Device &device,
		Font &font,
		const std::string &initialText,
		const std::string &name,
		bool controllable,
		bool placeInMiddle,
		GLFWwindow *window)
		: UIComponent(UIComponentCreationSettings{
			  /* model: */ nullptr,
			  /* name: */ name,
			  /* controllable: */ controllable,
			  /* window: */ window,
			  /* anchorRight: */ false,
			  /* anchorBottom: */ false,
			  /* placeInMiddle: */ placeInMiddle}),
		  device(device),
		  font(font),
		  textStr(initialText) {
		// white pixel atlas
		std::vector<unsigned char> whitePixel = {255, 255, 255, 255};
		material = std::make_shared<UIMaterial>(device, whitePixel, 1, 1, 4);
		rebuildMesh();
	}

	void TextComponent::setText(const std::string &text) {
		if (text != textStr) {
			textStr = text;
			rebuildMesh();
		}
	}

	void TextComponent::rebuildMesh() {
		std::vector<Model::Vertex> vertices;
		std::vector<uint32_t> indices;
		font.buildTextMesh(textStr, vertices, indices, 2.0f);

		if (vertices.size() < 3) {
			setModel(nullptr);
			return;
		}

		Model::Builder builder;
		builder.vertices = std::move(vertices);
		builder.indices = std::move(indices);
		builder.isUI = true;

		auto modelPtr = std::make_shared<Model>(device, builder);
		modelPtr->setMaterial(material);
		setModel(modelPtr);
	}

}  // namespace vk