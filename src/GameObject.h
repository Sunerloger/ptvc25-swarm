#pragma once

#include "vk_model.h"
#include "SceneManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace vk {

    using id_t = unsigned int;
    constexpr id_t INVALID_OBJECT_ID = 0;

    class GameObject {

    private:

        static inline id_t nextID = 1;

    public:

        virtual ~GameObject() {
            if (auto manager = this->sceneManager.lock()) {
                manager->removeGameObject(this->getId());
            }
        }

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;
        GameObject(GameObject &&) = default;
        GameObject &operator=(GameObject &&) = default;

        id_t getId() const { return id; }

        virtual glm::mat4 computeModelMatrix() const = 0;

        virtual glm::mat4 computeNormalMatrix() const = 0;

        virtual glm::vec3 getPosition() const = 0;
        
        // returns a nullptr if object has no model (e.g. light)
        virtual Model* getModel() const = 0;

        std::weak_ptr<SceneManager> sceneManager;

        glm::vec3 color{};

    protected:

        GameObject() : id(nextID++) {}

        const id_t id;
    };
}