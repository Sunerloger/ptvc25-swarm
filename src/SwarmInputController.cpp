#include "SwarmInputController.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace input {

    SwarmInputController::SwarmInputController(vk::Window& w, InputManager& im)
        : window(w), inputManager(im), lastX(w.getWidth() * 0.5), lastY(w.getHeight() * 0.5)
    {
        glfwSetCursorPos(window.getGLFWWindow(), lastX, lastY);
    }

    void SwarmInputController::setup() {

        setContext(ContextID::Gameplay);

        inputManager.registerKeyCallback(
            GLFW_KEY_ESCAPE,
            [this]() {
                setContext(ContextID::MainMenu);
            },
            this,
            ContextID::Gameplay
        );

        inputManager.registerKeyCallback(
            GLFW_KEY_ESCAPE,
            [this]() {
                setContext(ContextID::Gameplay);
            },
            this,
            ContextID::MainMenu
        );

        // use guards in case input action is not defined
        inputManager.registerKeyCallback(GLFW_KEY_SPACE, [this]() { if (onJump)  onJump();  }, this, ContextID::Gameplay);
        inputManager.registerMouseButtonCallback(GLFW_MOUSE_BUTTON_LEFT, [this]() { if (onShoot) onShoot(); }, this, ContextID::Gameplay);

        inputManager.registerPollingAction(
            [this](float dt) {
                // movement
                glm::vec3 dir{ 0.0f };
                if (this->inputManager.isKeyPressed(GLFW_KEY_W)) dir.z -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_S)) dir.z += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_A)) dir.x -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_D)) dir.x += 1;
                if (auto len = glm::length(dir); len > 0.0f) dir /= len;
                if (onMove) onMove(dir);
            },
            this,
            ContextID::Gameplay
        );
        inputManager.registerPollingAction(
            [this](float dt) {
                // looking
                auto [x, y] = this->inputManager.getCursorPos();
                float dx = float(x - lastX), dy = float(y - lastY);
                lastX = x; lastY = y;
                if ((dx || dy) && onLook) onLook(dx, dy);
            },
            this,
            ContextID::Gameplay
        );

        inputManager.registerPollingAction(
            [this](float dt) {
                // UI movement
                glm::vec3 dir{ 0.0f };
                if (this->inputManager.isKeyPressed(GLFW_KEY_LEFT)) dir.x -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_RIGHT)) dir.x += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_UP)) dir.y += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_DOWN)) dir.y -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_COMMA)) dir.z += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_PERIOD)) dir.z -= 1;
                if (auto len = glm::length(dir); len > 0.0f) dir /= len;
                if (dir != glm::vec3(0.0f) && onMoveUI) onMoveUI(dt, dir);
            },
            this,
            ContextID::MainMenu
        );
        inputManager.registerPollingAction(
            [this](float dt) {
                // UI rotation
                glm::vec3 rotDir{ 0.0f };
                if (this->inputManager.isKeyPressed(GLFW_KEY_Z)) rotDir.x -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_X)) rotDir.x += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_C)) rotDir.y -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_V)) rotDir.y += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_B)) rotDir.z -= 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_N)) rotDir.z += 1;
                if (rotDir != glm::vec3(0.0f) && onRotateUI) onRotateUI(dt, rotDir);
            },
            this,
            ContextID::MainMenu
        );
        inputManager.registerPollingAction(
            [this](float dt) {
                // UI scale
                int scaleDir = 0;
                if (this->inputManager.isKeyPressed(GLFW_KEY_EQUAL)) scaleDir += 1;
                if (this->inputManager.isKeyPressed(GLFW_KEY_MINUS)) scaleDir -= 1;
                if (scaleDir != 0 && onScaleUI) onScaleUI(dt, scaleDir);
            },
            this,
            ContextID::MainMenu
        );
    }

    void SwarmInputController::deregister() {
        inputManager.deregisterOwner(this);
    }

    void SwarmInputController::setContext(ContextID ctx) {

        inputManager.setActiveContext(ctx);

        // lock mouse for gameplay, free otherwise
        if (ctx == ContextID::Gameplay) {
            glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // reposition to center so mouse delta starts fresh
            lastX = window.getWidth() * 0.5;
            lastY = window.getHeight() * 0.5;
            glfwSetCursorPos(window.getGLFWWindow(), lastX, lastY);
        }
        else {
            glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    int SwarmInputController::getContext() const {
        return inputManager.getActiveContext();
    }

    bool SwarmInputController::isPaused() const {
        return inputManager.getActiveContext() != ContextID::Gameplay;
    }
}