#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <Jolt/Jolt.h>

using namespace JPH;

struct CharacterCameraSettings {
    float fov = 60.0f;
    float aspectRatio;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float initialYaw = 0.0f;
    float initialPitch = 0.0f;
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

    void setPhysicsPosition(Vec3 physicsPosition);

    void addRotation(float deltaYaw, float deltaPitch, float deltaTime);

    glm::vec3 getFront();

    float getYaw();

private:

    CharacterCameraSettings* settings;

    glm::vec3 position = glm::vec3(0);
    glm::mat4 projMatrix;

    float yaw = 0;
    float pitch = 0;

    void setYaw(float yaw);
    void setPitch(float pitch);
};