#pragma once

#include <map>

#include "GameObject.h"
#include "simulation/objects/ManagedPhysicsEntity.h"
#include "simulation/objects/actors/Player.h"
#include "simulation/objects/actors/enemies/Enemy.h"

using namespace vk;
using namespace physics;

// provides scene information to the renderer and the physics engine
struct Scene {

	// manage themselves - need to be treated differently
	std::unordered_map<BodyID, std::unique_ptr<Enemy>> enemies = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::unordered_map<BodyID, std::unique_ptr<ManagedPhysicsEntity>> physicsObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint - (also pointlights)
	std::unordered_map<id_t, std::unique_ptr<GameObject>> spectralObjects = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::unordered_map<id_t, std::unique_ptr<GameObject>> uiObjects = {};
};

enum GameObjectType {
	// TODO 4, models should be managed by each object but only loaded once (not per object), TODO lights are spectral objects
};

// TODO inject sceneManager reference into every place it is needed

// manages active scenes
class SceneManager {
public:

	SceneManager();
	virtual ~SceneManager() = default;

	// @return false if object could not be added because it already exists
	bool addSpectralObject(std::unique_ptr<GameObject> spectralObject);

	// @return false if object could not be added because it already exists
	bool addUIObject(std::unique_ptr<GameObject> uiObject);

	// @return false if object could not be added because it already exists
	bool addEnemy(std::unique_ptr<Enemy> enemy);

	// @return false if object could not be added because it already exists
	bool addManagedPhysicsEntity(std::unique_ptr<ManagedPhysicsEntity> managedPhysicsEntity);

	// deletes body (except for physics related entities)
	// @return true if the object could be found and deleted
	bool deleteSpectralObject(id_t id);

	bool deleteUIObject(id_t id);

	bool deleteManagedPhysicsEntity(BodyID id);

	bool deleteEnemy(BodyID id);

	bool activateManagedPhysicsEntity(BodyID id);

	bool activateEnemy(BodyID id);

	// removes bodies of scene from simulation but doesn't delete them (preserves state)
	void detachObject(BodyID id);

	bool editNonPhysicsObject();

	// scenes that are active or objects that are part of active scenes mustn't be changed!
	bool editPassivePhysicsObject(string sceneName);

	const std::map<id_t, unique_ptr<GameObject>>& getRenderObjectsReadOnly() const;

	Player* getPlayer();

	void setPlayer(std::unique_ptr<Player> player);

	void setPhysicsSystem(PhysicsSystem* physics_system);

private:

	PhysicsSystem* physics_system;

	unique_ptr<Player> player = nullptr;

	unique_ptr<Scene> scene{};

	std::map<BodyID, unique_ptr<GameObject>> passivePhysicsObjects{};
};