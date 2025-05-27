#pragma once

#include <memory>

#include "IGame.h"
#include "logical_systems/input/IInputController.h"
#include "logical_systems/input/InputManager.h"

class GameBase : public IGame {
   public:
	GameBase(input::IInputController& ctrl)
		: inputController(ctrl) {}

	void setupInput() override {
		bindInput();
	}

	void setInputController(input::IInputController& newCtrl) {
		inputController.deregister();
		inputController = newCtrl;
		setupInput();
	}

   protected:
	virtual void bindInput() = 0;

	input::IInputController& inputController;
};