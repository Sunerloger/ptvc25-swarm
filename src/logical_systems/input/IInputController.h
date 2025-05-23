#pragma once

namespace input {

    class InputManager;

    struct IInputController {
        virtual ~IInputController() = default;
        virtual void setup() = 0;
        virtual void deregister() = 0;
        virtual bool isPaused() const = 0;
    };
}