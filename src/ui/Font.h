#pragma once

#include "../vk/vk_model.h"

#include <string>
#include <vector>
#include <cstring>

#include "stb_easy_font.h"

namespace vk {

	class Font {
	   public:
		Font() = default;
		~Font() = default;

		int getTextWidth(const std::string &text, float scale = 1.0f) const;

		void buildTextMesh(const std::string &text,
			std::vector<Model::Vertex> &outVertices,
			std::vector<uint32_t> &outIndices,
			float scale = 1.0f) const;
	};

}  // namespace vk