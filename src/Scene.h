#pragma once

#include "GameObject.h"
#include "simulation/objects/ManagedPhysicsEntity.h"
#include "simulation/objects/actors/Player.h"
#include "simulation/objects/actors/enemies/Enemy.h"

using namespace vk;
using namespace physics;

// provides scene information to the renderer and the physics engine
struct Scene {

	// has to be unique
	string name;

	// manage themselves - need to be treated differently
	std::vector<std::shared_ptr<Enemy>> enemies = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::vector<std::shared_ptr<ManagedPhysicsEntity>> physicsObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint
	std::vector<std::shared_ptr<GameObject>> spectralObjects = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::vector<std::shared_ptr<GameObject>> uiObjects = {};
};

// TODO ensure that game object is only part of one scene
// TODO physicsObjects managed by a PhysicsScene (see LoadSaveBinaryTest.cpp in Jolt)
// TODO maybe use std::unordered_map<id_t, GameObject>