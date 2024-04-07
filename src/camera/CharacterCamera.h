#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <Jolt/Jolt.h>

#include "VulkanLaunchpad.h"

using namespace JPH;

struct CharacterCameraSettings {
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    glm::vec3 cameraOffsetFromCharacter = glm::vec3(0);

    // update per second
    float cameraSpeed = 20;
};

class CharacterCamera {
public:
    
    CharacterCamera(CharacterCameraSettings* cameraSettings);
    virtual ~CharacterCamera();

    glm::mat4 getViewProjMatrix();
    glm::vec3 getPosition();

    // sets viewMatrix of physics character and applies locally stored rotation
    void setViewMatrix(RMat44 physicsViewMatrix);

    void addRotation(float deltaYaw, float deltaPitch, float deltaTime);

    glm::vec3 getFront();

private:

    CharacterCameraSettings* settings;

    glm::vec3 position = glm::vec3(0);
    glm::mat4 projMatrix;

    float yaw = 0;
    float pitch = 0;

    void setYaw(float yaw);
    void setPitch(float pitch);
};