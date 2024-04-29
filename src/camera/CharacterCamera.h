#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cmath>

#include <Jolt/Jolt.h>

using namespace JPH;

struct CharacterCameraSettings {
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

    const glm::mat4 calculateViewMat() const;
    const glm::mat4 getProjMat() const;
    const glm::vec3 getPosition() const;

    void setPhysicsPosition(Vec3 physicsPosition);

    void addRotation(float deltaYaw, float deltaPitch, float deltaTime);

    const glm::vec3 getFront() const;

    float getYaw();

    void setViewDirection(glm::vec3 direction);
    void setViewTarget(glm::vec3 target);

    // flips y axis
    void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

    // flips y axis
    void setPerspectiveProjection(float fov, float aspect, float near, float far);

private:

    CharacterCameraSettings* settings;

    glm::vec3 position = glm::vec3(0);
    glm::mat4 projMatrix = glm::mat4(1);

    float yaw = 0.0f;
    float pitch = 0.0f;

    void setYaw(float yaw);
    void setPitch(float pitch);
};