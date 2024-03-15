//
// Created by Vlad Dancea on 11.03.24.
//

#ifndef GCGPROJECT_VK_CAMERA_H
#define GCGPROJECT_VK_CAMERA_H

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>



class Camera {
public:
    glm::vec3 position;
    float yaw;
    float pitch;
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;

    Camera(float fov, float aspectRatio, float nearPlane, float farPlane) {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        yaw = 0.0f; // Initial yaw, looking along -Z axis
        pitch = 0.0f; // Initial pitch
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
        viewMatrix = glm::lookAt(position, position + getFront(), up); // TODO maybe delete
    }

    glm::mat4 getViewProjMatrix() const {
        return projMatrix * viewMatrix;
    }

    glm::vec3 getPosition() const {
        return position;
    }

    void moveForward(float delta) {
        position += getFront() * delta;
        updateCameraVectors();
    }

    void moveBackward(float delta) {
        position -= getFront() * delta;
        updateCameraVectors();
    }

    void moveLeft(float delta) {
        glm::vec3 right = glm::normalize(glm::cross(getFront(), up));

        position -= right * delta;
        updateCameraVectors();
    }

    void moveRight(float delta) {
        glm::vec3 right = glm::normalize(glm::cross(getFront(), up));

        position += right * delta;
        updateCameraVectors();
    }

    glm::vec3 getFront() {
        glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::rotate(front, yaw, up);
        glm::rotate(front, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        return front;
    }

};





#endif //GCGPROJECT_VK_CAMERA_H
