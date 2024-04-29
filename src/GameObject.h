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
        virtual glm::vec3 getPosition() = 0;
        
        // returns a nullptr if object has no model
        virtual Model* getModel() = 0;

        glm::vec3 color{};

    protected:

        GameObject() : id(nextID++) {}

        const id_t id;
    };
}