#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../GameObject.h"

namespace vk {
    class UIComponent : public GameObject {

    public:

        UIComponent(Model* model, glm::vec3 position, glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f), bool isDrawLines = false, bool isEscapeMenu = false, 
            glm::vec3 color = glm::vec3(0.0f));
        virtual ~UIComponent() = default;

        glm::mat4 computeModelMatrix() const override;

        glm::mat4 computeNormalMatrix() const override;

        glm::vec3 getPosition() const override;

        Model* getModel() const override;

        glm::vec3 getScale() const;

        // create subclasses if hud elements need extra logic
        bool isDrawLines;
        bool isEscapeMenu;

    private:

        Model* model;

        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;

    };
}