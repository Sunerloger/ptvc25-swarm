//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_VK_GAME_OBJECT_H
#define GCGPROJECT_VK_VK_GAME_OBJECT_H

#endif //GCGPROJECT_VK_VK_GAME_OBJECT_H

#pragma once

#include "vk_model.h"
#include <memory>

struct Transform2dComponent {
    glm::vec2 translation{};
    glm::vec2 scale{1.0f, 1.0f};
    float rotation;

    glm::mat2 mat2() {
        const float s = glm::sin(rotation);
        const float c = glm::cos(rotation);
        glm::mat2 rotMat{{c, s},
                              {-s, c}};

        glm::mat2 scaleMat{{scale.x, 0.0f},
                           {0.0f, scale.y}};
        return rotMat * scaleMat;
    }
};

namespace vk {
    class GameObject {
    public:
        using id_t = unsigned int;

        static GameObject createGameObject() {
            static id_t currentId = 0;
            return GameObject{currentId++};
        }

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;
        GameObject(GameObject &&) = default;
        GameObject &operator=(GameObject &&) = default;

        id_t getId() const { return id; }

        std::shared_ptr<Model> model;
        glm::vec3 color{};
        Transform2dComponent transform2d{};

    private:
        GameObject(id_t objId) : id{objId} {}

        id_t id;
    };
}