//
// Created by Vlad Dancea on 29.03.24.
//

#pragma once

#include "vk_game_object.h"
#include "vk_window.h"
#include "vk_camera.h"
#include "vk_frame_info.h"

namespace vk {
    class KeyboardMovementController {
    public:

        KeyboardMovementController(int WIDTH,
                                   int HEIGHT) {
            lastMouseX = WIDTH / 2;
            lastMouseY = HEIGHT / 2;
        }

        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
        };

        void moveInPlaneXZ(GLFWwindow *window,
                           float dt,
                           GameObject &gameObject);

        void lookInPlaneXY(GLFWwindow *window,
                           float dt,
                           GameObject &gameObject);

        void handleEscMenu(GLFWwindow *window);

        void handleClicking(GLFWwindow *window, float dt, Camera &camera, FrameInfo &frameInfo);

        bool escapeMenuOpen = false;
    private:
        KeyMappings keys{};
        float moveSpeed{3.0f};
        float lookSpeed{0.1f};

        double lastMouseX = 0;
        double lastMouseY = {0};
        bool firstMouse = true;

        bool escKeyPressedLastFrame = false;
        bool f2KeyPressedLastFrame = false;
        bool fullScreen = false;
        int xPos, yPos, width, height, refreshRate;
        double lastCursorPosX, lastCursorPosY;
    };

}