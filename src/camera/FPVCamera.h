#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>



class FPVCamera {
public:
    glm::vec3 position;
    float yaw;
    float pitch;
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 projMatrix;

    FPVCamera(float fov, float aspectRatio, float nearPlane, float farPlane);
    virtual ~FPVCamera();

    void setYaw(float newYaw);

    void setPitch(float newPitch);

    void addYaw(float deltaYaw);

    void addPitch(float deltaPitch);

    glm::mat4 getViewProjMatrix();

    glm::vec3 getPosition();

    void moveForward(float delta);

    void moveBackward(float delta);

    void moveLeft(float delta);

    void moveRight(float delta);

private:

    glm::vec3 getFront();

    glm::mat4 getViewMatrix();

};