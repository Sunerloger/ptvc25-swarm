#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>

#include "logical_systems/input/IInputController.h"
#include "logical_systems/input/InputManager.h"
#include "vk/vk_window.h"


namespace input {

    class SwarmInputController : public IInputController {
    public:

        enum ContextID {
            Gameplay = 0,
            MainMenu = 1
        };

        SwarmInputController(vk::Window& w, InputManager& im);

        void setup() override;
        void deregister() override;

        bool isPaused() const override;

        void setContext(ContextID ctx);
        int getContext() const;

        // define input actions
        std::function<void(const glm::vec3& dir)>                           onMove;
        std::function<void(float dx, float dy)>                             onLook;
        std::function<void()>                                               onJump;
        std::function<void()>                                               onShoot;

        std::function<void(float deltaTime, const glm::vec3& dir)>          onMoveUI;
        std::function<void(float deltaTime, const glm::vec3& rotDir)>       onRotateUI;
        std::function<void(float deltaTime, int scaleDir)>                  onScaleUI;

    private:
        vk::Window& window;

        input::InputManager& inputManager;

        double lastX, lastY;

        // fullscreen window restoration variables
        int prevX, prevY, prevW, prevH, prevRefresh;
    };
}