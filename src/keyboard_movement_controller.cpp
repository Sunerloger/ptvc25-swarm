#include "keyboard_movement_controller.h"

namespace vk {

    void KeyboardMovementController::handleRotation(GLFWwindow* window,
                                                   float deltaTime,
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
            player.handleRotation(xOffset, yOffset, deltaTime);
        }
    }

    void KeyboardMovementController::handleMovement(GLFWwindow* window,
                                                   physics::Player& player) {

        Vec3 movementDirection = RVec3::sZero();

        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
            movementDirection += Vec3{ 0,0,-1 };
        }
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
            movementDirection += Vec3{ 0,0,1 };
        }
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
            movementDirection += Vec3{ -1,0,0 };
        }
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
            movementDirection += Vec3{ 1,0,0 };
        }

        movementDirection = movementDirection.NormalizedOr(Vec3{ 0,0,0 });

        bool isJump = glfwGetKey(window, keys.moveForward) == GLFW_PRESS;

        // only update if something happened
        if (movementDirection != Vec3{ 0,0,0 } || isJump) {
            player.handleMovement(movementDirection, isJump);
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

    void KeyboardMovementController::handleClicking(GLFWwindow *window, float dt, FrameInfo &frameInfo) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glm::vec3 cameraForward = frameInfo.sceneManager.getPlayer()->getFront();
            glm::vec3 cameraPosition = frameInfo.sceneManager.getPlayer()->getCameraPosition();

            for (auto it = frameInfo.gameObjects.begin(); it != frameInfo.gameObjects.end(); ) {
                auto& obj = it->second;
                if(obj.isEnemy == nullptr) {
                    ++it;
                    continue;
                }

                glm::mat2x3 boundingBox = obj.boundingBox;
                glm::vec3 min = boundingBox[0];
                glm::vec3 max = boundingBox[1];

                // Simple AABB ray intersection check
                // Note: This is a very basic form of intersection testing and might not be accurate for all cases.
                float tMin = (min.x - cameraPosition.x) / cameraForward.x;
                float tMax = (max.x - cameraPosition.x) / cameraForward.x;

                if (tMin > tMax) std::swap(tMin, tMax);

                float tyMin = (min.y - cameraPosition.y) / cameraForward.y;
                float tyMax = (max.y - cameraPosition.y) / cameraForward.y;

                if (tyMin > tyMax) std::swap(tyMin, tyMax);

                if ((tMin > tyMax) || (tyMin > tMax)) {
                    it++;
                    continue;
                }


                if (tyMin > tMin)
                    tMin = tyMin;

                if (tyMax < tMax)
                    tMax = tyMax;

                float tzMin = (min.z - cameraPosition.z) / cameraForward.z;
                float tzMax = (max.z - cameraPosition.z) / cameraForward.z;

                if (tzMin > tzMax) std::swap(tzMin, tzMax);

                if ((tMin > tzMax) || (tzMin > tMax)) {
                    it++;
                    continue;
                }
                if (tzMin > tMin)
                    tMin = tzMin;

                if (tzMax < tMax)
                    tMax = tzMax;

                // If we reach here, there was an intersection
                std::cout << "Hit detected with enemy object" << std::endl;
                it = frameInfo.gameObjects.erase(it);
            }
        }
    }

}