#pragma once

#include <glm/glm.hpp>

#include <Jolt/Jolt.h>

#include "VulkanLaunchpad.h"

using namespace JPH;

class CharacterCamera {
public:
    
    CharacterCamera(float fov, float aspectRatio, float nearPlane, float farPlane);
    virtual ~CharacterCamera();

    glm::mat4 getViewProjMatrix();
    void setViewMatrix(RMat44 physicsViewMatrix);

private:

    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
};