//
// Created by Vlad Dancea on 29.03.24.
//

#pragma once

#include "vk_game_object.h"
#include "vk_window.h"
#include "vk_camera.h"
#include "vk_frame_info.h"

#include "vk_model.h"

#include <iostream>
#include "simulation/objects/actors/Player.h"

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
            int jump = GLFW_KEY_SPACE;
        };

        void handleMovement(GLFWwindow* window,
                           physics::Player& player);

        void handleRotation(GLFWwindow* window,
                           float deltaTime,
                           physics::Player& player);

        void handleEscMenu(GLFWwindow* window);

        void handleClicking(GLFWwindow* window, float deltaTime, Camera& camera, FrameInfo& frameInfo);

        bool escapeMenuOpen = false;
    private:
        KeyMappings keys{};

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