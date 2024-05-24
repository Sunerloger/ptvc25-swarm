#pragma once

#include <map>

#include "GameObject.h"
#include "simulation/objects/ManagedPhysicsEntity.h"
#include "simulation/objects/actors/Player.h"
#include "simulation/objects/actors/enemies/Enemy.h"
#include "lighting/PointLight.h"
#include "lighting/Sun.h"

using namespace vk;
using namespace physics;
using namespace lighting;

// provides scene information to the renderer and the physics engine
struct Scene {

	unique_ptr<Player> player = nullptr;

	// manage themselves - need to be treated differently
	std::unordered_map<id_t, std::unique_ptr<Enemy>> enemies = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::unordered_map<id_t, std::unique_ptr<ManagedPhysicsEntity>> physicsObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint - (also pointlights)
	std::unordered_map<id_t, std::unique_ptr<GameObject>> spectralObjects = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::unordered_map<id_t, std::unique_ptr<GameObject>> uiObjects = {};

	// not rendered and not in physics engine
	std::unordered_map<id_t, std::unique_ptr<PointLight>> lights = {};

	// not rendered and not in physics engine
	// TODO bring this back
	std::unique_ptr<Sun> sun = nullptr;

	std::unordered_map<id_t, unique_ptr<Enemy>> passiveEnemies{};
	std::unordered_map<id_t, unique_ptr<ManagedPhysicsEntity>> passivePhysicsObjects{};
};

// manages active scenes
class SceneManager {
public:

	SceneManager() = default;
	virtual ~SceneManager() = default;

	// always replaces old player!
	void setPlayer(std::unique_ptr<Player> player);

	// always replaces old sun!
	void setSun(std::unique_ptr<Sun> sun);

	// @return false if object could not be added because it already exists
	bool addSpectralObject(std::unique_ptr<GameObject> spectralObject);

	// @return false if object could not be added because it already exists
	bool addUIObject(std::unique_ptr<GameObject> uiObject);

	// @return false if enemy could not be added because it already exists
	bool addEnemy(std::unique_ptr<Enemy> enemy);

	// @return false if object could not be added because it already exists
	bool addManagedPhysicsEntity(std::unique_ptr<ManagedPhysicsEntity> managedPhysicsEntity);

	// @return false if light could not be added because it already exists
	bool addLight(std::unique_ptr<PointLight> light);

	// @return true if the object could be found and deleted
	bool deleteSpectralObject(id_t id);

	// @return true if the object could be found and deleted
	bool deleteUIObject(id_t id);

	// @return true if the object could be found and deleted
	bool deleteManagedPhysicsEntity(id_t id);

	// @return true if the enemy could be found and deleted
	bool deleteEnemy(id_t id);

	// @return true if the light could be found and deleted
	bool deleteLight(id_t id);

	// activates detached bodies (added to simulation again)
	bool activatePhysicsObject(id_t id);

	// removes bodies of scene from simulation but doesn't delete them (preserves state)
	bool detachPhysicsObject(id_t id);

	// TODO edits should happen via returned pointers/references and to physics objects only via locks

	// TODO replace this with injection of manager
	// const std::map<id_t, unique_ptr<GameObject>>& getRenderObjectsReadOnly() const;

	// don't change returned enemies (not thread safe)
	vector<Enemy*> getAllEnemies() const;

	Player* getPlayer();

	Sun* getSun();

	// returns the boolean and resets it to false
	bool isBroadPhaseOptimizationNeeded();

private:

	// for optimize broad phase -> optimize broad phase before simulation step if bodies in physics system changed
	bool physicsSceneIsChanged = false;

	PhysicsSystem* physics_system;

	unique_ptr<Scene> scene{};
};