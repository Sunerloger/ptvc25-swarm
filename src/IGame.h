#pragma once


class IGame {

public:
	virtual void init() = 0;
	virtual void prePhysicsUpdate() = 0;
	virtual void postPhysicsUpdate() = 0;
	virtual void menuUpdate() = 0;

	virtual const string getName() const = 0;
};
