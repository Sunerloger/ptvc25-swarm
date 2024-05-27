#pragma once

#include <map>

#include "GameObject.h"
#include "simulation/objects/ManagedPhysicsEntity.h"
#include "simulation/objects/actors/Player.h"
#include "simulation/objects/actors/enemies/Enemy.h"
#include "lighting/PointLight.h"
#include "lighting/Sun.h"
#include "ui/UIComponent.h"

using namespace vk;
using namespace physics;
using namespace lighting;

enum SceneClass {
	PLAYER,
	SUN,
	LIGHT,
	ENEMY,
	UI_COMPONENT,
	PHYSICS_OBJECT,
	SPECTRAL_OBJECT
};

// provides scene information to the renderer and the physics engine
struct Scene {

	shared_ptr<Player> player;

	// not rendered and not in physics engine
	std::shared_ptr<Sun> sun;

	// not rendered and not in physics engine
	std::unordered_map<id_t, std::shared_ptr<PointLight>> lights = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::unordered_map<id_t, std::shared_ptr<UIComponent>> uiObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint - (also pointlights)
	std::unordered_map<id_t, std::shared_ptr<GameObject>> spectralObjects = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::unordered_map<id_t, std::shared_ptr<ManagedPhysicsEntity>> physicsObjects = {};

	// manage themselves - need to be treated differently
	std::unordered_map<id_t, std::shared_ptr<Enemy>> enemies = {};

	std::unordered_map<id_t, shared_ptr<Enemy>> passiveEnemies{};
	std::unordered_map<id_t, shared_ptr<ManagedPhysicsEntity>> passivePhysicsObjects{};
};

// manages active scenes
class SceneManager : public enable_shared_from_this<SceneManager> {
public:

	SceneManager() = default;
	virtual ~SceneManager() = default;

	// always replaces old player!
	id_t setPlayer(std::unique_ptr<Player> player);

	// always replaces old sun!
	id_t setSun(std::unique_ptr<Sun> sun);


	// @return false if object could not be added because it already exists
	id_t addSpectralObject(std::unique_ptr<GameObject> spectralObject);

	// @return false if object could not be added because it already exists
	id_t addUIObject(std::unique_ptr<UIComponent> uiObject);

	// @return false if enemy could not be added because it already exists
	id_t addEnemy(std::unique_ptr<Enemy> enemy);

	// @return false if object could not be added because it already exists
	id_t addManagedPhysicsEntity(std::unique_ptr<ManagedPhysicsEntity> managedPhysicsEntity);

	// @return false if light could not be added because it already exists
	id_t addLight(std::unique_ptr<lighting::PointLight> light);


	// @return true if the game object could be found and deleted, does not delete player or sun
	bool deleteGameObject(id_t id);

	// @return true if the game object could be found and removed, does not remove player or sun
	unique_ptr<pair<SceneClass, shared_ptr<GameObject>>> removeGameObject(id_t id);


	// activates detached bodies (added to simulation again)
	bool activatePhysicsObject(id_t id);

	// removes bodies of scene from simulation but doesn't delete them (preserves state)
	bool detachPhysicsObject(id_t id);

	// TODO edits should happen via returned pointers/references and to physics objects only via locks

	// don't change returned enemies (not thread safe)
	vector<std::shared_ptr<Enemy>> getActiveEnemies() const;

	vector<std::shared_ptr<PointLight>> getLights();

	vector<std::shared_ptr<UIComponent>> getUIObjects();

	std::shared_ptr<Player> getPlayer();

	std::shared_ptr<Sun> getSun();

	// returns the boolean and resets it to false
	bool isBroadPhaseOptimizationNeeded();

	id_t getIdFromBodyID(BodyID bodyID);

private:

	// for optimize broad phase -> optimize broad phase before simulation step if bodies in physics system changed
	bool physicsSceneIsChanged = false;

	// weak to prevent ownership loop
	std::weak_ptr<PhysicsSystem> physics_system;

	unique_ptr<Scene> scene{};

	// enables simple self-removal from manager when game objects should despawn according to their own logic
	std::unordered_map<id_t, SceneClass> idToClass = {};

	// enables to recognize objects on collision
	std::unordered_map<BodyID, id_t> bodyIDToObjectId = {};
};