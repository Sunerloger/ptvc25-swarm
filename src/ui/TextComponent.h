#pragma once

#include "UIComponent.h"
#include "Font.h"

#include "../vk/vk_model.h"
#include "../rendering/materials/UIMaterial.h"

#include <memory>

namespace vk {

	class TextComponent : public UIComponent {
	   public:
		TextComponent(Device &device, Font &font,
			const std::string &initialText,
			const std::string &name,
			bool controllable = false);
		~TextComponent() override = default;

		void setText(const std::string &text);

	   private:
		void rebuildMesh();

		Device &device;
		Font &font;
		std::string textStr;
		std::shared_ptr<Material> material;
	};

}  // namespace vk