#pragma once

#include "GameObject.h"
#include "simulation/objects/PhysicsEntity.h"
#include "simulation/objects/actors/Player.h"

using namespace vk;
using namespace physics;

// provides scene information to the renderer and the physics engine
struct Scene {

	// has to be unique
	string name;

	// physics engine doesn't influence position (e.g. terrain)
	std::vector<std::shared_ptr<PhysicsEntity>> staticObjects = {};

	// physics engine has an influence on position (with behaviour e.g. enemies, whithout behaviour e.g. drops)
	std::vector<std::shared_ptr<PhysicsEntity>> dynamicObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint
	std::vector<std::shared_ptr<GameObject>> spectralObjects = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::vector<std::shared_ptr<GameObject>> uiObjects = {};
};

// TODO ensure that game object is only part of one scene
// TODO maybe use PhysicsScene (see LoadSaveBinaryTest.cpp in Jolt)