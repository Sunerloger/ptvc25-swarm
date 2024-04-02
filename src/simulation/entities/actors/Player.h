#pragma once

#include "camera/FPVCamera.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>

using namespace JPH;

class Player {

public:
	JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, Player)

	Player();
	virtual ~Player();



private:

	// before the physics update
	FPVCamera& mCamera;

	RefConst<Shape> mStandingShape;
	Ref<Character> mBody;

};

// TODO all camera transformations and rotations caused by the physics simulation and main pass through this class and update both the camera and the body
