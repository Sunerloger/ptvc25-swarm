#include "Skybox.h"
#include <glm/gtc/matrix_transform.hpp>

namespace vk {

    Skybox::Skybox(Device& device, const std::array<std::string, 6>& cubemapFaces)
        : device(device) {
        // Create the cube model using the factory method in Model
        skyboxModel = Model::createCubeModel(device);
        
        // Create the cubemap material and set it on the model
        auto material = std::make_shared<CubemapMaterial>(device, cubemapFaces);
        skyboxModel->setMaterial(material);
    }
    
    Skybox::Skybox(Device& device, const std::string& singleImagePath, bool isHorizontalStrip)
        : device(device) {
        // Create the cube model using the factory method in Model
        skyboxModel = Model::createCubeModel(device);
        
        // Create the cubemap material from a single image and set it on the model
        auto material = std::make_shared<CubemapMaterial>(device, singleImagePath, isHorizontalStrip);
        skyboxModel->setMaterial(material);
    }

    // GameObject interface implementation
    glm::mat4 Skybox::computeModelMatrix() const {
        // For a skybox, we typically want it to be centered around the camera
        // without any translation, so we just return an identity matrix
        return glm::mat4(1.0f);
    }

    glm::mat4 Skybox::computeNormalMatrix() const {
        // For a skybox, the normal matrix is also an identity matrix
        return glm::mat4(1.0f);
    }

    glm::vec3 Skybox::getPosition() const {
        // Skybox is centered at origin
        return glm::vec3(0.0f);
    }

    std::shared_ptr<Model> Skybox::getModel() const {
        return skyboxModel;
    }

} // namespace vk