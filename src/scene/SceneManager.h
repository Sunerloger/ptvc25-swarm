#pragma once

#include <map>

#include "../GameObject.h"
#include "../simulation/objects/ManagedPhysicsEntity.h"
#include "../simulation/objects/actors/Player.h"
#include "../simulation/objects/actors/enemies/Enemy.h"
#include "../lighting/PointLight.h"
#include "../lighting/Sun.h"
#include "../ui/UIComponent.h"
#include "ISceneManagerInteraction.h"

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

	std::shared_ptr<physics::Player> player;

	// not rendered and not in physics engine
	std::shared_ptr<lighting::Sun> sun;

	// not rendered and not in physics engine
	std::unordered_map<vk::id_t, std::shared_ptr<lighting::PointLight>> lights = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::unordered_map<vk::id_t, std::shared_ptr<vk::UIComponent>> uiObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint - (also pointlights)
	std::unordered_map<vk::id_t, std::shared_ptr<vk::GameObject>> spectralObjects = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::unordered_map<vk::id_t, std::shared_ptr<physics::ManagedPhysicsEntity>> physicsObjects = {};

	// manage themselves - need to be treated differently
	std::unordered_map<vk::id_t, std::shared_ptr<physics::Enemy>> enemies = {};

	std::unordered_map<vk::id_t, std::shared_ptr<physics::Enemy>> passiveEnemies = {};
	std::unordered_map<vk::id_t, std::shared_ptr<physics::ManagedPhysicsEntity>> passivePhysicsObjects = {};
};

// manages active scenes
class SceneManager : public std::enable_shared_from_this<SceneManager>, public ISceneManagerInteraction {
public:

	SceneManager();
	virtual ~SceneManager() = default;

	// always replaces old player!
	vk::id_t setPlayer(std::unique_ptr<physics::Player> player);

	// always replaces old sun!
	vk::id_t setSun(std::unique_ptr<lighting::Sun> sun);


	// @return false if object could not be added because it already exists
	vk::id_t addSpectralObject(std::unique_ptr<vk::GameObject> spectralObject);

	// @return false if object could not be added because it already exists
	vk::id_t addUIObject(std::unique_ptr<vk::UIComponent> uiObject);

	// @return false if enemy could not be added because it already exists
	vk::id_t addEnemy(std::unique_ptr<physics::Enemy> enemy);

	// @return false if object could not be added because it already exists
	vk::id_t addManagedPhysicsEntity(std::unique_ptr<physics::ManagedPhysicsEntity> managedPhysicsEntity);

	// @return false if light could not be added because it already exists
	vk::id_t addLight(std::unique_ptr<lighting::PointLight> light);


	// @return true if the game object could be found and deleted, does not delete player or sun
	bool deleteGameObject(vk::id_t id) override;

	// @return true if the game object could be found and removed, does not remove player or sun
	std::unique_ptr<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>> removeGameObject(vk::id_t id);


	// activates detached bodies (added to simulation again)
	bool activatePhysicsObject(vk::id_t id);

	// removes bodies of scene from simulation but doesn't delete them (preserves state)
	bool detachPhysicsObject(vk::id_t id);

	// TODO edits should happen via returned pointers/references and to physics objects only via locks

	// only change returned enemies with a lock (otherwise not thread safe)
	std::vector<std::weak_ptr<physics::Enemy>> getActiveEnemies() const;

	std::vector<std::weak_ptr<lighting::PointLight>> getLights();

	std::vector<std::weak_ptr<vk::UIComponent>> getUIObjects();

	// don't change physics related properties of returned objects without a lock (otherwise not thread safe)
	std::unique_ptr<std::pair<SceneClass,std::weak_ptr<vk::GameObject>>> getObject(vk::id_t id);

	std::shared_ptr<physics::Player> getPlayer() override;

	std::shared_ptr<lighting::Sun> getSun();

	// returns the boolean and resets it to false
	bool isBroadPhaseOptimizationNeeded();

	vk::id_t getIdFromBodyID(JPH::BodyID bodyID);

	std::vector<std::weak_ptr<vk::GameObject>> getRenderObjects();

private:

	// for optimize broad phase -> optimize broad phase before simulation step if bodies in physics system changed
	bool physicsSceneIsChanged = false;

	std::unique_ptr<Scene> scene;

	// enables simple self-removal from manager when game objects should despawn according to their own logic
	std::unordered_map<vk::id_t, SceneClass> idToClass = {};

	// enables to recognize objects on collision
	std::unordered_map<JPH::BodyID, vk::id_t> bodyIDToObjectId = {};
};