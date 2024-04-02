//
// Created by Vlad Dancea on 29.03.24.
//

#include "keyboard_movement_controller.h"

#include <iostream>

namespace vk {

    void KeyboardMovementController::lookInPlaneXY(GLFWwindow *window,
                                                   float dt,
                                                   vk::GameObject &gameObject) {
        if(escapeMenuOpen) {
            return;
        }
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        float xOffset = (mouseX - lastMouseX) * lookSpeed;
        float yOffset = (lastMouseY - mouseY) * lookSpeed;

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        if(std::numeric_limits<float>::epsilon() < glm::abs(xOffset) ||
           std::numeric_limits<float>::epsilon() < glm::abs(yOffset)) {
            gameObject.transform.rotation.y += glm::radians(xOffset * dt);
            gameObject.transform.rotation.x += glm::radians(yOffset * dt);
        }

        // limit pitch values between +//- 85ish degrees
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
    }

    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window,
                                                   float dt,
                                                   GameObject& gameObject) {
        if(escapeMenuOpen) {
            return;
        }
        glm::vec3 rotate{0};
        if(glfwGetKey(window, keys.lookRight) == GLFW_PRESS) {
            rotate.y += 1.f;
        }
        if(glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) {
            rotate.y -= 1.f;
        }
        if(glfwGetKey(window, keys.lookUp) == GLFW_PRESS) {
            rotate.x += 1.f;
        }
        if(glfwGetKey(window, keys.lookDown) == GLFW_PRESS) {
            rotate.x -= 1.f;
        }

        if(glm::dot(rotate,rotate) > std::numeric_limits<float>::epsilon()) {
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
        if(glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
            moveDir += forwardDir;
        }
        if(glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
            moveDir -= forwardDir;
        }
        if(glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
            moveDir += rightDir;
        }
        if(glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
            moveDir -= rightDir;
        }
        if(glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
            moveDir += upDir;
        }
        if(glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
            moveDir -= upDir;
        }

        if(glm::dot(moveDir,moveDir) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }

    void KeyboardMovementController::controlGame(GLFWwindow* window, float dt) {
        bool escKeyPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        // Check if ESC was not pressed last frame but is pressed now
        if (!escKeyPressedLastFrame && escKeyPressed) {
            escapeMenuOpen = !escapeMenuOpen; // Toggle the menu state
            std::cout << "Escape menu open: " << escapeMenuOpen << std::endl;

            // Optional: Handle cursor visibility and capturing based on the menu state
            if (escapeMenuOpen) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }

        // If the escape menu is open, allow other controls like F1 to close the window
        if(escapeMenuOpen) {
            if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        // Update the last frame key state
        escKeyPressedLastFrame = escKeyPressed;
    }
}