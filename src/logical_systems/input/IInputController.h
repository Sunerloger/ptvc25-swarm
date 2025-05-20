#pragma once

namespace input {

    class InputManager;

    struct IInputController {
        virtual ~IInputController() = default;
        virtual void setup(InputManager& im) = 0;
        virtual void deregister(InputManager& im) = 0;
    };
}