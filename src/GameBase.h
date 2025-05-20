#pragma once

#include <memory>

#include "IGame.h"
#include "logical_systems/input/IInputController.h"
#include "logical_systems/input/InputManager.h"


class GameBase : public IGame {
public:
    GameBase(std::shared_ptr<input::IInputController> ctrl)
        : inputController(std::move(ctrl)), paused(false) {}

    void setupInput(input::InputManager& im) override {
        inputController->setup(im);
        bindInput(im);
    }
    void togglePause() override { paused = !paused; }
    bool isPaused()   const override { return paused; }

    void setInputController(std::shared_ptr<input::IInputController> newCtrl,
        input::InputManager& im)
    {
        inputController->deregister(im);
        inputController = std::move(newCtrl);
        setupInput(im);
    }

protected:
    virtual void bindInput(input::InputManager& im) = 0;
    std::shared_ptr<input::IInputController> inputController;

private:
    bool paused;
};