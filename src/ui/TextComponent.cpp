// TextComponent.cpp
#include "TextComponent.h"
#include "../rendering/materials/UIMaterial.h"
#include "../vk/vk_model.h"
#include <memory>

namespace vk {

TextComponent::TextComponent(Device &device, Font &font,
                             const std::string &initialText,
                             const std::string &name,
                             bool controllable)
    : UIComponent(UIComponentCreationSettings{nullptr, name, controllable}),
      device(device), font(font), textStr(initialText) {
    // Create a white font atlas material (no texture, color-only)
    // For now, use a 1x1 white texture via UIMaterial with embedded data
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
    std::vector<Model::Vertex> verts;
    std::vector<uint32_t> inds;
    // Build the text mesh (scale increased for visibility)
    font.buildTextMesh(textStr, verts, inds, 2.0f);
    // If no sufficient vertices were generated, clear the model (nothing to render)
    if (verts.size() < 3) {
        setModel(nullptr);
        return;
    }
    // Build a new model from the generated mesh
    Model::Builder builder{};
    builder.vertices = std::move(verts);
    builder.indices = std::move(inds);
    builder.isUI = true;
    auto modelPtr = std::make_shared<Model>(device, builder);
    modelPtr->setMaterial(material);
    // Update UIComponent's model
    setModel(modelPtr);
}

} // namespace vk