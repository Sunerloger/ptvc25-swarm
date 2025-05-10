#include "Font.h"
#include "stb_easy_font.h"
#include <cstring>

namespace vk {

	int Font::getTextWidth(const std::string &text, float scale) const {
		// stb_easy_font reports width in pixels at scale=1
		return stb_easy_font_width(const_cast<char *>(text.c_str())) * scale;
	}

	void Font::buildTextMesh(const std::string &text,
		std::vector<Model::Vertex> &outVertices,
		std::vector<uint32_t> &outIndices,
		float scale) const {
		// Build text mesh using stb_easy_font
		// Allocate a buffer (~270 bytes per character)
		int max_buffer_size = static_cast<int>(text.size()) * 270 + 100;
		std::vector<char> buffer(max_buffer_size);
		// Generate quad segments (4 vertices each)
		int quadCount = stb_easy_font_print(
			0.0f, 0.0f,
			const_cast<char *>(text.c_str()),
			nullptr,
			buffer.data(), max_buffer_size);

		outVertices.clear();
		outIndices.clear();
		if (quadCount <= 0)
			return;
		// Each quad -> two triangles -> 6 vertices
		outVertices.reserve(static_cast<size_t>(quadCount) * 6);

		// Raw vertex layout: x,y,z + 4-byte color = 16 bytes per vertex
		struct RawVert {
			float x, y, z;
			unsigned char r, g, b, a;
		};
		RawVert *vbuf = reinterpret_cast<RawVert *>(buffer.data());
		// Triangulate quads
		for (int q = 0; q < quadCount; ++q) {
			int base = q * 4;
			auto makeVert = [&](int idx) {
				RawVert &rv = vbuf[idx];
				Model::Vertex vert{};
				vert.position = glm::vec3(rv.x * scale, rv.y * scale, 0.0f);
				vert.color = glm::vec3(1.0f);
				vert.normal = glm::vec3(0.0f, 0.0f, 1.0f);
				vert.uv = glm::vec2(0.0f);
				return vert;
			};
			// Triangle 1
			outVertices.push_back(makeVert(base + 0));
			outVertices.push_back(makeVert(base + 1));
			outVertices.push_back(makeVert(base + 2));
			// Triangle 2
			outVertices.push_back(makeVert(base + 0));
			outVertices.push_back(makeVert(base + 2));
			outVertices.push_back(makeVert(base + 3));
		}
	}

}  // namespace vk