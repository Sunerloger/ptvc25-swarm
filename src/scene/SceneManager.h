#pragma once

#include <map>
#include <queue>
#include <memory>

#include "../GameObject.h"
#include "../simulation/objects/ManagedPhysicsEntity.h"
#include "../simulation/objects/actors/Player.h"
#include "../simulation/objects/actors/enemies/Enemy.h"
#include "../rendering/structures/WaterObject.h"
#include "../lighting/PointLight.h"
#include "../lighting/Sun.h"
#include "../ui/UIComponent.h"

enum SceneClass {
	INVALID,
	PLAYER,
	SUN,
	WATER,
	LIGHT,
	ENEMY,
	UI_COMPONENT,
	PHYSICS_OBJECT,
	SPECTRAL_OBJECT,
	TESSELLATION_OBJECT	 // New class for tessellation objects
};

// TODO simplify GameObject and SceneManager
// TODO GameObject stores SceneClass
// TODO make singleton
// TODO store assets + models + materials in sceneManager
// TODO sceneGraph

// provides scene information to the renderer and the physics engine
struct Scene {
	std::unique_ptr<Player> player;

	// not rendered and not in physics engine
	std::shared_ptr<lighting::Sun> sun;

	// rendered and not in physics engine
	std::unordered_map<vk::id_t, std::shared_ptr<vk::WaterObject>> waterObjects = {};

	// not rendered and not in physics engine
	std::unordered_map<vk::id_t, std::shared_ptr<lighting::PointLight>> lights = {};

	// not influenced by physics engine (= no collisions) and not translated according to viewpoint (= fixed on screen)
	std::unordered_map<vk::id_t, std::shared_ptr<vk::UIComponent>> uiObjects = {};

	// not influenced by physics engine (= no collisions), but translated according to viewpoint - (also pointlights)
	std::unordered_map<vk::id_t, std::shared_ptr<vk::GameObject>> spectralObjects = {};

	// non actor physics objects (e.g. terrain, drops, bullets, ...)
	std::unordered_map<vk::id_t, std::shared_ptr<physics::ManagedPhysicsEntity>> physicsObjects = {};

	// objects that use tessellation shaders
	std::unordered_map<vk::id_t, std::shared_ptr<physics::ManagedPhysicsEntity>> tessellationObjects = {};

	// manage themselves - need to be treated differently
	std::unordered_map<vk::id_t, std::shared_ptr<physics::Enemy>> enemies = {};

	std::unordered_map<vk::id_t, std::shared_ptr<physics::Enemy>> passiveEnemies = {};
	std::unordered_map<vk::id_t, std::shared_ptr<physics::ManagedPhysicsEntity>> passivePhysicsObjects = {};

	// objects scheduled for deletion from scene manager
	std::queue<vk::id_t> staleQueue = {};

	// TODO + store information if timer is running only when game is running in timers themselves
	// std::vector<Timer> timers;
};

// manages active scenes
class SceneManager {
   public:
	static SceneManager& getInstance();

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	void updateUIPosition(float deltaTime, glm::vec3 dir);
	void updateUIRotation(float deltaTime, glm::vec3 rotDir);
	void updateUIScale(float deltaTime, int scaleDir);

	// always replaces old player!
	std::unique_ptr<Player> setPlayer(std::unique_ptr<Player> player);

	// always replaces old sun!
	vk::id_t setSun(std::unique_ptr<lighting::Sun> sun);

	vk::id_t addWaterObject(std::unique_ptr<vk::WaterObject> waterObject);

	// @return false if object could not be added because it already exists
	vk::id_t addSpectralObject(std::unique_ptr<vk::GameObject> spectralObject);

	// @return false if object could not be added because it already exists
	vk::id_t addUIObject(std::unique_ptr<vk::UIComponent> uiObject);

	// @return false if enemy could not be added because it already exists
	vk::id_t addEnemy(std::unique_ptr<physics::Enemy> enemy);

	// @return false if object could not be added because it already exists
	vk::id_t addManagedPhysicsEntity(std::unique_ptr<physics::ManagedPhysicsEntity> managedPhysicsEntity);

	// @return false if object could not be added because it already exists
	vk::id_t addTessellationObject(std::unique_ptr<physics::ManagedPhysicsEntity> tessellationObject);

	// @return false if light could not be added because it already exists
	vk::id_t addLight(std::unique_ptr<lighting::PointLight> light);

	// @return true if the game object could be found and added to queue for deletion, does not delete player or sun
	bool addToStaleQueue(vk::id_t id);

	// @return true if the game object could be found and removed, does not remove player or sun
	std::unique_ptr<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>> removeGameObject(vk::id_t id);

	// delete objects in staleQueue
	void removeStaleObjects();

	// update step of all active enemies according to their behaviour in physics system
	void updateEnemyPhysics(float cPhysicsDeltaTime);

	// update step of all active enemies according to their behaviour in rendering system
	void updateEnemyVisuals(float deltaTime);

	// update step of all managed physics entities (e.g., grenades) according to their behaviour in physics system
	void updatePhysicsEntities(float cPhysicsDeltaTime);

	// activates detached bodies (added to simulation again)
	bool activatePhysicsObject(vk::id_t id);

	// removes bodies of scene from simulation but doesn't delete them (preserves state)
	bool detachPhysicsObject(vk::id_t id);

	// only change returned enemies with a lock (otherwise not thread safe)
	std::vector<std::weak_ptr<physics::Enemy>> getActiveEnemies() const;

	std::vector<std::weak_ptr<lighting::PointLight>> getLights();

	std::vector<std::weak_ptr<vk::UIComponent>> getUIObjects();

	// don't change physics related properties of returned objects without a lock (otherwise not thread safe)
	std::pair<SceneClass, vk::GameObject*> getObject(vk::id_t id);

	Player* getPlayer();

	std::shared_ptr<lighting::Sun> getSun();

	std::vector<std::weak_ptr<vk::WaterObject>> getWaterObjects();

	// returns the boolean and resets it to false
	bool isBroadPhaseOptimizationNeeded();

	vk::id_t getIdFromBodyID(JPH::BodyID bodyID);

	// Get standard render objects (non-tessellated)
	std::vector<std::weak_ptr<vk::GameObject>> getStandardRenderObjects();

	// Get tessellation render objects
	std::vector<std::weak_ptr<vk::GameObject>> getTessellationRenderObjects();

	void clearUIObjects();

	// Clear vegetation objects from the scene
	void clearVegetationObjects();

	void toggleUIVisibility() {
		isUIVisible = !isUIVisible;
	}

	void toggleDebugMenu() {
		isDebugMenuVisible = !isDebugMenuVisible;
	}

   private:
	SceneManager();
	~SceneManager() = default;

	// for optimize broad phase -> optimize broad phase before simulation step if bodies in physics system changed
	bool physicsSceneIsChanged = false;

	std::unique_ptr<Scene> scene;

	// enables simple self-removal from manager when game objects should despawn according to their own logic
	std::unordered_map<vk::id_t, SceneClass> idToClass = {};

	// enables to recognize objects on collision
	std::unordered_map<JPH::BodyID, vk::id_t> bodyIDToObjectId = {};

	bool isUIVisible = true;
	bool isDebugMenuVisible = false;
};