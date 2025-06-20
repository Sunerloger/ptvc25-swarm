#pragma once

#include <string>

#include "logical_systems/Settings.h"

class IGame {

public:
	virtual ~IGame() = default;

	virtual void init() = 0;
	
	virtual void prePhysicsUpdate() = 0;
	virtual void postPhysicsUpdate() = 0;

	virtual void gameActiveUpdate(float deltaTime) = 0;
	virtual void gamePauseUpdate(float deltaTime) = 0;

	virtual void postRenderingUpdate(EngineStats engineStats, float deltaTime) = 0;

	virtual void setupInput() = 0;

	virtual bool isPaused() const = 0;

	virtual std::string getName() const = 0;
};
