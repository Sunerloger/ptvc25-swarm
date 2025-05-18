#pragma once

#include <string>


class IGame {

public:
	virtual void init() = 0;
	
	virtual void prePhysicsUpdate() = 0;
	virtual void postPhysicsUpdate() = 0;

	virtual void gameActiveUpdate(float deltaTime) = 0;
	virtual void gamePauseUpdate(float deltaTime) = 0;

	virtual std::string getName() const = 0;
};
