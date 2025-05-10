// Font.h
#pragma once
#include <string>
#include <vector>
#include "../vk/vk_model.h"

/**
 * A minimal font abstraction using stb_easy_font (deferred to user-provided header).
 * Builds simple quads for ASCII text.
 */
namespace vk {

class Font {
public:
    Font() = default;
    ~Font() = default;

    /**
     * Measure the width in pixels of the given text at unit scale.
     */
    int getTextWidth(const std::string &text, float scale = 1.0f) const;

    /**
     * Build mesh data (vertices and indices) for the given text string.
     * The output vertices are in 2D (x,y) with z=0, color set to white, normal=(0,0,1), uv=(0,0).
     * No indices are emitted (draw with vkCmdDraw).  Scale applies to both x and y.
     */
    void buildTextMesh(const std::string &text,
                       std::vector<Model::Vertex> &outVertices,
                       std::vector<uint32_t> &outIndices,
                       float scale = 1.0f) const;
};

} // namespace vk