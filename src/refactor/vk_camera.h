//
// Created by Vlad Dancea on 29.03.24.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


namespace vk{
    class Camera {

    public:
        void updateCameraVectors();

        void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

        void setViewDirection(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});

        void setViewTarget(const glm::vec3 position, const glm::vec3 target, const glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});

        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

        const glm::mat4& getProjection() const { return projectionMatrix; }

        const glm::mat4& getView() const { return viewMatrix; }

        const glm::mat4& getInverseView() const { return inverseViewMatrix; }

        void setYaw(float yaw) { this->yaw = yaw; }

        void setPitch(float pitch) { this->pitch = pitch; }

        float getYaw() const { return yaw; }

        float getPitch() const { return pitch; }

    private:
        float yaw = 0.0f;
        float pitch = 0.0f;
        glm::mat4 projectionMatrix{1.0f};
        glm::mat4 viewMatrix{1.0f};
        glm::mat4 inverseViewMatrix{1.0f};
    };
}