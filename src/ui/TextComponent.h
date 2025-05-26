#pragma once

#include "UIComponent.h"
#include "Font.h"
#include "../vk/vk_model.h"
#include "../rendering/materials/UIMaterial.h"

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace vk {

	class TextComponent : public UIComponent {
	   public:
		TextComponent(Device &device,
			Font &font,
			const std::string &initialText,
			const std::string &name,
			bool controllable = false,
			bool centerHorizontal = false,
			float horizontalOffset = 0.0f,
			bool centerVertical = false,
			float verticalOffset = 0.0f,
			bool anchorRight = false,
			bool anchorBottom = false,
			GLFWwindow *window = nullptr);

		~TextComponent() override = default;

		void setText(const std::string &text);
		glm::mat4 computeModelMatrix() const override;

	   private:
		void rebuildMesh();

		Device &device;
		Font &font;
		std::string textStr;
		std::shared_ptr<Material> material;
		glm::vec2 textSize{0.0f, 0.0f};
		float horizontalOffset = 0.0f;
		float verticalOffset = 0.0f;
	};

}  // namespace vk