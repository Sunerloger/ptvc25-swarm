//
// Created by Vlad Dancea on 11.03.24.
//

#ifndef GCGPROJECT_VK_CAMERA_H
#define GCGPROJECT_VK_CAMERA_H

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>



class Camera {
public:
    glm::vec3 position;
    float yaw;
    float pitch;
    glm::vec3 up;
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;

    Camera(float fov, float aspectRatio, float nearPlane, float farPlane) {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        yaw = -90.0f; // Initial yaw, looking along -Z axis
        pitch = 0.0f; // Initial pitch
        up = glm::vec3(0.0f, -1.0f, 0.0f); // Up is +Y axis
        projMatrix = createPerspectiveProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);
        updateCameraVectors();
    }

    static glm::mat4 createPerspectiveProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane) {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    void setYaw(float newYaw) {
        yaw = newYaw;
        updateCameraVectors();
    }

    void setPitch(float newPitch) {
        pitch = glm::clamp(newPitch, -89.0f, 89.0f); // Limit pitch to avoid gimbal lock
        updateCameraVectors();
    }

    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        viewMatrix = glm::lookAt(position, position + front, up);
    }

    glm::mat4 getViewProjMatrix() const {
        return projMatrix * viewMatrix;
    }

    glm::vec3 getPosition() const {
        return position;
    }

    void moveForward(float delta) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(0.0f)); // Assuming pitch does not affect forward movement
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front = glm::normalize(front);

        position += front * delta;
        updateCameraVectors();
    }

    void moveBackward(float delta) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(0.0f)); // Assuming pitch does not affect forward movement
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front = glm::normalize(front);

        position -= front * delta;
        updateCameraVectors();
    }

    void moveLeft(float delta) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(front, up));

        position -= right * delta;
        updateCameraVectors();
    }

    void moveRight(float delta) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(0.0f));
        front = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(front, up));

        position += right * delta;
        updateCameraVectors();
    }

};





#endif //GCGPROJECT_VK_CAMERA_H
