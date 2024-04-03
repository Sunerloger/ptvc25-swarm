//
// Created by Vlad Dancea on 29.03.24.
//

#include "keyboard_movement_controller.h"

#include <iostream>

namespace vk {

    void KeyboardMovementController::lookInPlaneXY(GLFWwindow *window,
                                                   float dt,
                                                   vk::GameObject &gameObject) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        double xOffset = (mouseX - lastMouseX) * lookSpeed;
        double yOffset = (lastMouseY - mouseY) * lookSpeed;

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        if (std::numeric_limits<float>::epsilon() < glm::abs(glm::radians(xOffset)) ||
            std::numeric_limits<float>::epsilon() < glm::abs(glm::radians(yOffset))) {
            gameObject.transform.rotation.y += xOffset * dt;
            gameObject.transform.rotation.x += yOffset * dt;
        }

        // limit pitch values between +//- 85ish degrees
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
    }

    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow *window,
                                                   float dt,
                                                   GameObject &gameObject) {
        glm::vec3 rotate{0};
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) {
            rotate.y += 1.f;
        }
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) {
            rotate.y -= 1.f;
        }
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) {
            rotate.x += 1.f;
        }
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) {
            rotate.x -= 1.f;
        }

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
        }

        // limit pitch values between +//- 85ish degrees
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
        const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
        const glm::vec3 upDir{0.f, 1.f, 0.f};

        glm::vec3 moveDir{0.f};
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
            moveDir += forwardDir;
        }
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
            moveDir -= forwardDir;
        }
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
            moveDir += rightDir;
        }
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
            moveDir -= rightDir;
        }
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
            moveDir += upDir;
        }
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
            moveDir -= upDir;
        }

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }

    void KeyboardMovementController::handleEscMenu(GLFWwindow *window) {
        bool escKeyPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        bool f2KeyPressed = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;

        // Handle ESC key for opening/closing the escape menu
        if (!escKeyPressedLastFrame && escKeyPressed) {
            escapeMenuOpen = !escapeMenuOpen; // Toggle the menu state

            if (escapeMenuOpen) {
                // Game is being paused, store cursor position
                glfwGetCursorPos(window, &lastCursorPosX, &lastCursorPosY);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                // Game is resuming, restore cursor position
                glfwSetCursorPos(window, lastCursorPosX, lastCursorPosY);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            // Optional: Handle cursor visibility and capturing based on the menu state
            if (escapeMenuOpen) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }

        // If the escape menu is open, allow other controls like F1 to close the window
        if (escapeMenuOpen) {
            if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        // Handle F2 key for toggling full-screen mode
        if (!f2KeyPressedLastFrame && f2KeyPressed) {
            if (glfwGetWindowMonitor(window) == NULL) {
                // Switch to full-screen
                glfwGetWindowPos(window, &xPos, &yPos); // Save the window position
                glfwGetWindowSize(window, &width, &height); // Save the window size
                refreshRate = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height,
                                     mode->refreshRate);
            } else {
                // Switch back to windowed mode
                // Restore the window size and position as desired
                glfwSetWindowMonitor(window, NULL, xPos, yPos, width, height, refreshRate);
            }
        }

        // Update the last frame key states
        escKeyPressedLastFrame = escKeyPressed;
        f2KeyPressedLastFrame = f2KeyPressed;
    }

    void KeyboardMovementController::handleClicking(GLFWwindow *window, float dt, Camera &camera, FrameInfo &frameInfo) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glm::vec3 cameraForward = camera.getDirection();
            glm::vec3 cameraPosition = camera.getPosition();
            // Go through each game object in the scene
            for (auto it = frameInfo.gameObjects.begin(); it != frameInfo.gameObjects.end(); ) {
                auto& obj = it->second;
                if(obj.isEnemy == nullptr || !(*obj.isEnemy)) {
                    ++it;
                    continue;
                }

                // Calculate vector from camera to object
                glm::vec3 toObject = obj.transform.translation - cameraPosition;
                float distanceToObject = glm::length(toObject);
                glm::vec3 dirToObject = glm::normalize(toObject);

                std::cout << "Distance to object: " << distanceToObject << std::endl;

                // Check if object is in front of the camera using dot product
                // and within a "reasonable" distance (for this example, say 100 units)
                if (glm::dot(cameraForward, dirToObject) > 0.95f && distanceToObject < 100.0f) {
                    // Assume the object is "hit" and remove it
                    // Realistically, you'd want to call some sort of "destroy" or "onHit" method
                    it = frameInfo.gameObjects.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // Right mouse button is pressed
            // Handle the click event
        }
    }

}