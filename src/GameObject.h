#pragma once

#include "vk_model.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace vk {

    using id_t = unsigned int;

    class GameObject {

    private:

        static inline id_t nextID = 0;

    public:

        virtual ~GameObject() = default;

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;
        GameObject(GameObject &&) = default;
        GameObject &operator=(GameObject &&) = default;

        id_t getId() const { return id; }

        virtual glm::mat4 computeModelMatrix() = 0;
        virtual glm::mat4 computeNormalMatrix() = 0;

        glm::vec3 color{};

        // TODO does every game object have a model? e.g. light doesn't have one
        std::shared_ptr<Model> model{};

    protected:

        GameObject() : id(nextID++) {}

        const id_t id;
    };
}