// TextComponent.h
#pragma once
#include "UIComponent.h"
#include "Font.h"

namespace vk {

/**
 * A UIComponent that renders a dynamic text string in screen space.
 * Use setText() to update the displayed string (rebuilds mesh).
 */
class TextComponent : public UIComponent {
public:
    /**
     * @param device    Vulkan device for buffer uploads
     * @param font      Font helper that builds text meshes
     * @param initialText  Initial string to display
     * @param name      Unique name for placement in ui_placements.ini
     * @param controllable  If true, can be moved via placement keys
     */
    TextComponent(Device &device, Font &font,
                  const std::string &initialText,
                  const std::string &name,
                  bool controllable = false);
    ~TextComponent() override = default;

    /**
     * Update the displayed text; rebuilds the mesh.
     */
    void setText(const std::string &text);

private:
    void rebuildMesh();

    Device &device;
    Font &font;
    std::string textStr;
    std::shared_ptr<Material> material;
};

} // namespace vk