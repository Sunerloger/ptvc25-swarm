#pragma once

#include "../materials/CubemapMaterial.h"
#include "../../GameObject.h"
#include <memory>
#include <array>
#include <string>
#include <glm/glm.hpp>

namespace vk {

    class Skybox : public GameObject {
    public:
        // Create a skybox from 6 separate face images
        Skybox(Device& device, const std::array<std::string, 6>& cubemapFaces);
        
        // Create a skybox from a single image (either a horizontal or vertical strip)
        Skybox(Device& device, const std::string& singleImagePath, bool isHorizontalStrip = true);
        
        ~Skybox() override = default;

        // GameObject interface implementation
        glm::mat4 computeModelMatrix() const override;
        glm::mat4 computeNormalMatrix() const override;
        glm::vec3 getPosition() const override;
        std::shared_ptr<Model> getModel() const override;

        bool enableFrustumCulling() const override { return false; }

    private:
        Device& device;
        std::shared_ptr<Model> skyboxModel;
    };

} // namespace vk