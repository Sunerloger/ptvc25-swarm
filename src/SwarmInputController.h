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
        SwarmInputController(vk::Window& w);

        void setup(InputManager& im) override;
        void deregister(InputManager& im) override;

        // define input actions
        std::function<void(const glm::vec3& dir)>           onMove;
        std::function<void(float dx, float dy)>             onLook;
        std::function<void()>                               onJump;
        std::function<void()>                               onShoot;
        std::function<void()>                               onPause;
        // TODO

    private:
        vk::Window& window;

        double lastX, lastY;
    };
}