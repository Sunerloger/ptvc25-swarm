#include "GameObject.h"
#include "simulation/objects/PhysicsEntity.h"

// provides scene information to the renderer and the physics engine
struct Scene {

	Player& player;

	// physics engine doesn't influence position (e.g. terrain)
	std::vector<PhysicsEntity> staticObjects = {};

	// physics engine has an influence on position (with behaviour e.g. enemies, whithout behaviour e.g. drops)
	std::vector<PhysicsEntity> dynamicObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint
	std::vector<GameObject> spectralObjects = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::vector<GameObject> uiObjects = {};
};