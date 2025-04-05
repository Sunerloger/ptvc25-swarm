#include "keyboard_movement_controller.h"

namespace controls {

    void KeyboardMovementController::handleRotation(GLFWwindow* window,
                                                   physics::Player& player) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);


        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        // right = negative rotation around y axis
        double xOffset = lastMouseX - mouseX;
        // down = negative rotation around x axis
        double yOffset = lastMouseY - mouseY;

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        // check if rotation is significant enough for processing
        if (std::numeric_limits<float>::epsilon() < glm::abs(glm::radians(xOffset)) ||
            std::numeric_limits<float>::epsilon() < glm::abs(glm::radians(yOffset))) {
            player.handleRotation(xOffset, yOffset);
        }
    }

    MovementIntent KeyboardMovementController::getMovementIntent(GLFWwindow* window) {

        glm::vec3 movementDirection = glm::vec3();

        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
            movementDirection += glm::vec3{ 0,0,-1 };
        }
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
            movementDirection += glm::vec3{ 0,0,1 };
        }
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
            movementDirection += glm::vec3{ -1,0,0 };
        }
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
            movementDirection += glm::vec3{ 1,0,0 };
        }

        float length = glm::length(movementDirection);

        if (length > 0.0f) {
            movementDirection /= length;
        }

        bool isJump = glfwGetKey(window, keys.jump) == GLFW_PRESS;

        return { movementDirection, isJump };
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
}