#include "SceneManager.h"

SceneManager::SceneManager() : scene(std::make_unique<Scene>()) {}

void SceneManager::updateUITransforms(float deltaTime, int placementTransform) {
	if (placementTransform == -1) {
		return;
	}
	for (auto& uiObject : this->getUIObjects()) {
		std::shared_ptr<vk::UIComponent> uiComponent = uiObject.lock();
		if (!uiComponent || !uiComponent->isControllable())
			continue;
		uiComponent->updateTransform(deltaTime, placementTransform);
	}
}

vk::id_t SceneManager::setPlayer(std::unique_ptr<physics::Player> newPlayer) {
	if (this->scene->player) {
		this->idToClass.erase(this->scene->player->getId());
		this->bodyIDToObjectId.erase(this->scene->player->getBodyID());
	}

	scene->player = std::move(newPlayer);

	this->idToClass.emplace(this->scene->player->getId(), PLAYER);
	this->bodyIDToObjectId.emplace(this->scene->player->getBodyID(), this->scene->player->getId());

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	this->scene->player->setSceneManager(weakThis);

	scene->player->addPhysicsBody();

	this->physicsSceneIsChanged = true;

	return scene->player->getId();
}

vk::id_t SceneManager::setSun(std::unique_ptr<lighting::Sun> sun) {
	this->idToClass.erase(this->scene->sun->getId());

	scene->sun = std::move(sun);

	this->idToClass.emplace(this->scene->sun->getId(), SUN);

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	this->scene->sun->setSceneManager(weakThis);

	return scene->sun->getId();
}

vk::id_t SceneManager::addWaterObject(std::unique_ptr<vk::GameObject> waterObject) {
	vk::id_t id = waterObject->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	waterObject->setSceneManager(weakThis);

	std::pair result = this->scene->waterObjects.emplace(id, std::move(waterObject));

	if (result.second) {
		this->idToClass.emplace(id, WATER);
		return id;
	} else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addSpectralObject(std::unique_ptr<vk::GameObject> spectralObject) {
	vk::id_t id = spectralObject->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	spectralObject->setSceneManager(weakThis);

	std::pair result = this->scene->spectralObjects.emplace(id, std::move(spectralObject));

	if (result.second) {
		this->idToClass.emplace(id, SPECTRAL_OBJECT);
		return id;
	} else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addUIObject(std::unique_ptr<vk::UIComponent> uiObject) {
	vk::id_t id = uiObject->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	uiObject->setSceneManager(weakThis);

	std::pair result = this->scene->uiObjects.emplace(id, std::move(uiObject));

	if (result.second) {
		this->idToClass.emplace(id, UI_COMPONENT);
		return id;
	} else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addLight(std::unique_ptr<lighting::PointLight> light) {
	vk::id_t id = light->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	light->setSceneManager(weakThis);

	std::pair result = this->scene->lights.emplace(id, std::move(light));

	if (result.second) {
		this->idToClass.emplace(id, LIGHT);
		return id;
	} else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addEnemy(std::unique_ptr<physics::Enemy> enemy) {
	vk::id_t id = enemy->getId();
	JPH::BodyID bodyID = enemy->getBodyID();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	enemy->setSceneManager(weakThis);

	if (scene->passiveEnemies.find(id) != scene->passiveEnemies.end()) {
		return vk::INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->enemies.emplace(id, std::move(enemy));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->idToClass.emplace(id, ENEMY);
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return vk::INVALID_OBJECT_ID;
}

vk::id_t SceneManager::addManagedPhysicsEntity(std::unique_ptr<physics::ManagedPhysicsEntity> managedPhysicsEntity) {
	vk::id_t id = managedPhysicsEntity->getId();
	JPH::BodyID bodyID = managedPhysicsEntity->getBodyID();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	managedPhysicsEntity->setSceneManager(weakThis);

	if (scene->passivePhysicsObjects.find(id) != scene->passivePhysicsObjects.end()) {
		return vk::INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->physicsObjects.emplace(id, std::move(managedPhysicsEntity));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->idToClass.emplace(id, PHYSICS_OBJECT);
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return vk::INVALID_OBJECT_ID;
}

vk::id_t SceneManager::addTessellationObject(std::unique_ptr<physics::ManagedPhysicsEntity> tessellationObject) {
	vk::id_t id = tessellationObject->getId();
	JPH::BodyID bodyID = tessellationObject->getBodyID();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	tessellationObject->setSceneManager(weakThis);

	// Check if the object already exists
	if (scene->passivePhysicsObjects.find(id) != scene->passivePhysicsObjects.end()) {
		return vk::INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->tessellationObjects.emplace(id, std::move(tessellationObject));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->idToClass.emplace(id, TESSELLATION_OBJECT);
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return vk::INVALID_OBJECT_ID;
}

bool SceneManager::addToStaleQueue(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	} catch (std::out_of_range& e) {
		return false;
	}

	switch (sceneClass) {
		case PLAYER:
			return false;

		case SUN:
			return false;

		default:
			scene->staleQueue.push(id);
			return true;
	}
}

void SceneManager::removeStaleObjects() {
	vk::id_t id;
	SceneClass sceneClass;

	while (!scene->staleQueue.empty()) {
		id = scene->staleQueue.front();
		scene->staleQueue.pop();

		try {
			sceneClass = this->idToClass.at(id);
		} catch (std::out_of_range& e) {
			continue;
		}

		JPH::BodyID bodyID;

		switch (sceneClass) {
			case SPECTRAL_OBJECT:
				scene->spectralObjects.erase(id);
				this->idToClass.erase(id);
				continue;

			case UI_COMPONENT:
				scene->uiObjects.erase(id);
				this->idToClass.erase(id);
				continue;

			case LIGHT:
				scene->lights.erase(id);
				this->idToClass.erase(id);
				continue;

			case ENEMY:

				// one of the maps contains the enemy
				if (scene->enemies.count(id)) {
					bodyID = scene->enemies.at(id)->getBodyID();
					scene->enemies.erase(id);
				} else {
					bodyID = scene->passiveEnemies.at(id)->getBodyID();
					scene->passiveEnemies.erase(id);
				}

				this->idToClass.erase(id);
				this->bodyIDToObjectId.erase(bodyID);
				this->physicsSceneIsChanged = true;
				continue;

			case PHYSICS_OBJECT:

				// one of the maps contains the physics object
				if (scene->physicsObjects.count(id)) {
					bodyID = scene->physicsObjects.at(id)->getBodyID();
					scene->physicsObjects.erase(id);
				} else {
					bodyID = scene->passivePhysicsObjects.at(id)->getBodyID();
					scene->passivePhysicsObjects.erase(id);
				}

				this->idToClass.erase(id);
				this->bodyIDToObjectId.erase(bodyID);
				this->physicsSceneIsChanged = true;
				continue;

			case TESSELLATION_OBJECT:
				// Handle tessellation objects
				bodyID = scene->tessellationObjects.at(id)->getBodyID();
				scene->tessellationObjects.erase(id);

				this->idToClass.erase(id);
				this->bodyIDToObjectId.erase(bodyID);
				this->physicsSceneIsChanged = true;
				continue;

			default:
				continue;
		}
	}
}

void SceneManager::updateEnemies(float cPhysicsDeltaTime) {
	for (auto& pair : this->scene->enemies) {
		std::shared_ptr<physics::Enemy> enemy = pair.second;
		enemy->update(cPhysicsDeltaTime);
	}
}

std::unique_ptr<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>> SceneManager::removeGameObject(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	} catch (std::out_of_range& e) {
		return nullptr;
	}

	if (sceneClass == SPECTRAL_OBJECT) {
		this->idToClass.erase(id);

		auto itSpectralObjects = scene->spectralObjects.find(id);
		std::shared_ptr<vk::GameObject> spectralObject = std::move(itSpectralObjects->second);

		scene->spectralObjects.erase(id);

		spectralObject->deleteSceneManager();

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, spectralObject));
	} else if (sceneClass == UI_COMPONENT) {
		this->idToClass.erase(id);

		auto itUIObjects = scene->uiObjects.find(id);
		std::shared_ptr<vk::GameObject> uiObject = std::move(itUIObjects->second);

		scene->uiObjects.erase(id);

		uiObject->deleteSceneManager();

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, uiObject));
	} else if (sceneClass == LIGHT) {
		this->idToClass.erase(id);

		auto itLights = scene->lights.find(id);
		std::shared_ptr<vk::GameObject> light = std::move(itLights->second);

		scene->lights.erase(id);

		light->deleteSceneManager();

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, light));
	} else if (sceneClass == ENEMY) {
		this->idToClass.erase(id);
		JPH::BodyID bodyID = scene->enemies.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itEnemies = scene->enemies.find(id);
		if (itEnemies != scene->enemies.end()) {
			itEnemies->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> enemy = std::move(itEnemies->second);

			scene->enemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->deleteSceneManager();

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, enemy));
		} else {
			auto itPassiveEnemies = scene->passiveEnemies.find(id);
			itPassiveEnemies->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> enemy = std::move(itPassiveEnemies->second);

			scene->passiveEnemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->deleteSceneManager();

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, enemy));
		}
	} else if (sceneClass == PHYSICS_OBJECT) {
		this->idToClass.erase(id);
		JPH::BodyID bodyID = scene->physicsObjects.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itPhysicsObjects = scene->physicsObjects.find(id);
		if (itPhysicsObjects != scene->physicsObjects.end()) {
			itPhysicsObjects->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> physicsObject = std::move(itPhysicsObjects->second);

			scene->physicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->deleteSceneManager();

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, physicsObject));
		} else {
			auto itPassivePhysicsObjects = scene->passivePhysicsObjects.find(id);
			itPassivePhysicsObjects->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> physicsObject = std::move(itPassivePhysicsObjects->second);

			scene->passivePhysicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->deleteSceneManager();

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, physicsObject));
		}
	} else if (sceneClass == TESSELLATION_OBJECT) {
		this->idToClass.erase(id);
		JPH::BodyID bodyID = scene->tessellationObjects.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itTessellationObjects = scene->tessellationObjects.find(id);
		itTessellationObjects->second->removePhysicsBody();
		std::shared_ptr<vk::GameObject> tessellationObject = std::move(itTessellationObjects->second);

		scene->tessellationObjects.erase(id);
		this->physicsSceneIsChanged = true;

		tessellationObject->deleteSceneManager();

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, tessellationObject));
	} else {
		return nullptr;
	}
}

bool SceneManager::activatePhysicsObject(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	} catch (std::out_of_range& e) {
		return false;
	}

	if (sceneClass == ENEMY) {
		auto itPassiveEnemies = scene->passiveEnemies.find(id);
		if (itPassiveEnemies != scene->passiveEnemies.end()) {
			std::shared_ptr<physics::Enemy> enemy = std::move(itPassiveEnemies->second);
			scene->passiveEnemies.erase(id);

			enemy->addPhysicsBody();
			scene->enemies.emplace(id, std::move(enemy));
			this->physicsSceneIsChanged = true;
			return true;
		}
	} else if (sceneClass == PHYSICS_OBJECT) {
		auto itPassivePhysicsObjects = scene->passivePhysicsObjects.find(id);
		if (itPassivePhysicsObjects != scene->passivePhysicsObjects.end()) {
			std::shared_ptr<physics::ManagedPhysicsEntity> physicsObject = std::move(itPassivePhysicsObjects->second);
			scene->passivePhysicsObjects.erase(id);

			physicsObject->addPhysicsBody();
			scene->physicsObjects.emplace(id, std::move(physicsObject));
			this->physicsSceneIsChanged = true;
			return true;
		}
	}
	// Tessellation objects can't be passive, so no need to handle them here

	return false;
}

bool SceneManager::detachPhysicsObject(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	} catch (std::out_of_range& e) {
		return false;
	}

	if (sceneClass == ENEMY) {
		auto itEnemies = scene->enemies.find(id);
		if (itEnemies != scene->enemies.end()) {
			std::shared_ptr<physics::Enemy> enemy = std::move(itEnemies->second);
			scene->enemies.erase(id);

			enemy->removePhysicsBody();
			scene->passiveEnemies.emplace(id, std::move(enemy));
			this->physicsSceneIsChanged = true;
			return true;
		}
	} else if (sceneClass == PHYSICS_OBJECT) {
		auto itPhysicsObjects = scene->physicsObjects.find(id);
		if (itPhysicsObjects != scene->physicsObjects.end()) {
			std::shared_ptr<physics::ManagedPhysicsEntity> physicsObject = std::move(itPhysicsObjects->second);
			scene->physicsObjects.erase(id);

			physicsObject->removePhysicsBody();
			scene->passivePhysicsObjects.emplace(id, std::move(physicsObject));
			this->physicsSceneIsChanged = true;
			return true;
		}
	}
	// Tessellation objects can't be passive, so no need to handle them here

	return false;
}

std::vector<std::weak_ptr<physics::Enemy>> SceneManager::getActiveEnemies() const {
	std::vector<std::weak_ptr<physics::Enemy>> enemies = {};

	for (auto& it : this->scene->enemies) {
		std::weak_ptr<physics::Enemy> enemy = it.second;
		enemies.push_back(enemy);
	}

	return enemies;
}

std::vector<std::weak_ptr<lighting::PointLight>> SceneManager::getLights() {
	std::vector<std::weak_ptr<lighting::PointLight>> lights = {};

	for (auto& it : this->scene->lights) {
		std::weak_ptr<lighting::PointLight> light = it.second;
		lights.push_back(light);
	}

	return lights;
}

std::vector<std::weak_ptr<vk::UIComponent>> SceneManager::getUIObjects() {
	std::vector<std::weak_ptr<vk::UIComponent>> uiObjects = {};

	for (auto& it : this->scene->uiObjects) {
		std::weak_ptr<vk::UIComponent> uiObject = it.second;
		uiObjects.push_back(uiObject);
	}

	return uiObjects;
}
// Get water objects for rendering
std::vector<std::weak_ptr<vk::GameObject>> SceneManager::getWaterObjects() {
    std::vector<std::weak_ptr<vk::GameObject>> waterObjs;
    for (auto& it : this->scene->waterObjects) {
        waterObjs.push_back(it.second);
    }
    return waterObjs;
}

std::unique_ptr<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>> SceneManager::getObject(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	} catch (std::out_of_range& e) {
		return nullptr;
	}

	if (sceneClass == PLAYER) {
		std::weak_ptr<vk::GameObject> player = this->scene->player;
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, player));
	} else if (sceneClass == SUN) {
		std::weak_ptr<vk::GameObject> sun = this->scene->sun;
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, sun));
	} else if (sceneClass == LIGHT) {
		std::weak_ptr<vk::GameObject> light = this->scene->lights.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, light));
	} else if (sceneClass == ENEMY) {
		std::weak_ptr<vk::GameObject> enemy = this->scene->enemies.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, enemy));
	} else if (sceneClass == UI_COMPONENT) {
		std::weak_ptr<vk::GameObject> uiObject = this->scene->uiObjects.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, uiObject));
	} else if (sceneClass == PHYSICS_OBJECT) {
		std::weak_ptr<vk::GameObject> physicsObject = this->scene->physicsObjects.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, physicsObject));
	} else if (sceneClass == SPECTRAL_OBJECT) {
		std::weak_ptr<vk::GameObject> spectralObject = this->scene->spectralObjects.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, spectralObject));
	} else if (sceneClass == TESSELLATION_OBJECT) {
		std::weak_ptr<vk::GameObject> tessellationObject = this->scene->tessellationObjects.at(id);
		return std::make_unique<std::pair<SceneClass, std::weak_ptr<vk::GameObject>>>(make_pair(sceneClass, tessellationObject));
	} else {
		return nullptr;
	}
}

std::shared_ptr<physics::Player> SceneManager::getPlayer() {
	return this->scene->player;
}

std::shared_ptr<lighting::Sun> SceneManager::getSun() {
	return this->scene->sun;
}

bool SceneManager::isBroadPhaseOptimizationNeeded() {
	bool isNeeded = this->physicsSceneIsChanged;
	this->physicsSceneIsChanged = false;
	return isNeeded;
}

vk::id_t SceneManager::getIdFromBodyID(JPH::BodyID bodyID) {
	if (this->bodyIDToObjectId.count(bodyID)) {
		return this->bodyIDToObjectId.at(bodyID);
	}
	return vk::INVALID_OBJECT_ID;
}

std::vector<std::weak_ptr<vk::GameObject>> SceneManager::getStandardRenderObjects() {
	std::vector<std::weak_ptr<vk::GameObject>> renderObjects = {};

	for (auto& it : this->scene->spectralObjects) {
		std::weak_ptr<vk::GameObject> object = it.second;
		renderObjects.push_back(object);
	}

	for (auto& it : this->scene->physicsObjects) {
		std::weak_ptr<vk::GameObject> object = it.second;
		renderObjects.push_back(object);
	}

	for (auto& it : this->scene->enemies) {
		std::weak_ptr<vk::GameObject> object = it.second;
		renderObjects.push_back(object);
	}

	return renderObjects;
}

std::vector<std::weak_ptr<vk::GameObject>> SceneManager::getTessellationRenderObjects() {
	std::vector<std::weak_ptr<vk::GameObject>> tessellationObjects = {};

	for (auto& it : this->scene->tessellationObjects) {
		std::weak_ptr<vk::GameObject> object = it.second;
		tessellationObjects.push_back(object);
	}

	return tessellationObjects;
}