#include "SwarmInputController.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace input {

    SwarmInputController::SwarmInputController(vk::Window& w)
        : window(w), lastX(w.getWidth() * 0.5), lastY(w.getHeight() * 0.5)
    {
        glfwSetCursorPos(window.getGLFWWindow(), lastX, lastY);
    }

    void SwarmInputController::setup(InputManager& inputManager) {

        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // use guards in case input action is not defined
        inputManager.registerKeyCallback(GLFW_KEY_SPACE, [this]() { if (onJump)  onJump();  }, this);
        inputManager.registerKeyCallback(GLFW_KEY_ESCAPE, [this]() { if (onPause) onPause(); }, this);
        inputManager.registerMouseButtonCallback(GLFW_MOUSE_BUTTON_LEFT, [this]() { if (onShoot) onShoot(); }, this);

        inputManager.registerPollingAction(
            [this, &inputManager](float dt) {
                // movement
                glm::vec3 dir{ 0.0f };
                if (inputManager.isKeyPressed(GLFW_KEY_W)) dir.z -= 1;
                if (inputManager.isKeyPressed(GLFW_KEY_S)) dir.z += 1;
                if (inputManager.isKeyPressed(GLFW_KEY_A)) dir.x -= 1;
                if (inputManager.isKeyPressed(GLFW_KEY_D)) dir.x += 1;
                if (auto len = glm::length(dir); len > 0.0f) dir /= len;
                if (onMove) onMove(dir);
            },
            this
        );
        inputManager.registerPollingAction(
            [this, &inputManager](float dt) {
                // looking
                auto [x, y] = inputManager.getCursorPos();
                float dx = float(x - lastX), dy = float(y - lastY);
                lastX = x; lastY = y;
                if ((dx != 0.0f || dy != 0.0f) && onLook) onLook(dx, dy);
            },
            this
        );
        // int placementTransform = placementController.updateModelMatrix(window.getGLFWWindow());
    }

    void SwarmInputController::deregister(InputManager& inputManager) {
        inputManager.deregisterOwner(this);
    }
}