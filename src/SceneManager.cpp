#include "SceneManager.h"

SceneManager::SceneManager() : scene(std::make_unique<Scene>()) {}

vk::id_t SceneManager::setPlayer(std::unique_ptr<physics::Player> newPlayer) {

	this->idToClass.erase(this->scene->player->getId());
	this->bodyIDToObjectId.erase(this->scene->player->getBodyID());

	scene->player = std::move(newPlayer);

	this->idToClass.emplace(this->scene->player->getId(), PLAYER);
	this->bodyIDToObjectId.emplace(this->scene->player->getBodyID(), this->scene->player->getId());

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	this->scene->player->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	scene->player->addPhysicsBody();

	this->physicsSceneIsChanged = true;

	return scene->player->getId();
}

vk::id_t SceneManager::setSun(std::unique_ptr<lighting::Sun> sun) {

	this->idToClass.erase(this->scene->sun->getId());

	scene->sun = std::move(sun);

	this->idToClass.emplace(this->scene->sun->getId(), SUN);

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	this->scene->sun->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	return scene->sun->getId();
}

vk::id_t SceneManager::addSpectralObject(std::unique_ptr<vk::GameObject> spectralObject) {
	vk::id_t id = spectralObject->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	spectralObject->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	std::pair result = this->scene->spectralObjects.emplace(id, std::move(spectralObject));

	if (result.second) {
		this->idToClass.emplace(id, SPECTRAL_OBJECT);
		return id;
	}
	else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addUIObject(std::unique_ptr<vk::UIComponent> uiObject) {
	vk::id_t id = uiObject->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	uiObject->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	std::pair result = this->scene->uiObjects.emplace(id, std::move(uiObject));

	if (result.second) {
		this->idToClass.emplace(id, UI_COMPONENT);
		return id;
	}
	else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addLight(std::unique_ptr<lighting::PointLight> light) {
	vk::id_t id = light->getId();

	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	light->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	std::pair result = this->scene->lights.emplace(id, std::move(light));

	if (result.second) {
		this->idToClass.emplace(id, LIGHT);
		return id;
	}
	else {
		return vk::INVALID_OBJECT_ID;
	}
}

vk::id_t SceneManager::addEnemy(std::unique_ptr<physics::Enemy> enemy) {
	vk::id_t id = enemy->getId();
	JPH::BodyID bodyID = enemy->getBodyID();
	
	std::weak_ptr<SceneManager> weakThis = shared_from_this();
	enemy->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

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
	managedPhysicsEntity->setRemovalCallback([weakThis](vk::id_t id) {
		if (auto manager = weakThis.lock()) {
			manager->removeGameObject(id);
		}});

	if (scene->passivePhysicsObjects.find(id) != scene->passivePhysicsObjects.end()) {
		return vk::INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->physicsObjects.emplace(id, std::move(managedPhysicsEntity));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->idToClass.emplace(id, ENEMY);
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return vk::INVALID_OBJECT_ID;
}

bool SceneManager::deleteGameObject(vk::id_t id) {

	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	}
	catch (std::out_of_range& e) {
		return false;
	}

	JPH::BodyID bodyID;

	switch (sceneClass) {

	case SPECTRAL_OBJECT:
		scene->spectralObjects.erase(id);
		this->idToClass.erase(id);
		return true;

	case UI_COMPONENT:
		scene->uiObjects.erase(id);
		this->idToClass.erase(id);
		return true;

	case LIGHT:
		scene->lights.erase(id);
		this->idToClass.erase(id);
		return true;

	case ENEMY:
		// one of the maps contains the enemy
		scene->enemies.erase(id);
		scene->passiveEnemies.erase(id);

		this->idToClass.erase(id);
		bodyID = scene->enemies.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);
		this->physicsSceneIsChanged = true;
		return true;

	case PHYSICS_OBJECT:
		// one of the maps contains the physics object
		scene->physicsObjects.erase(id);
		scene->passivePhysicsObjects.erase(id);

		this->idToClass.erase(id);
		bodyID = scene->physicsObjects.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);
		this->physicsSceneIsChanged = true;
		return true;

	case PLAYER:
		return false;

	case SUN:
		return false;

	default:
		return false;
	}
}

std::unique_ptr<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>> SceneManager::removeGameObject(vk::id_t id) {

	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	}
	catch (std::out_of_range& e) {
		return nullptr;
	}

	if (sceneClass == SPECTRAL_OBJECT) {
		auto it = scene->spectralObjects.find(id);
		std::shared_ptr<vk::GameObject> spectralObject = std::move(it->second);
		scene->spectralObjects.erase(id);

		this->idToClass.erase(id);

		spectralObject->setRemovalCallback(nullptr);

		// compiler automatically applies move semantics here
		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, spectralObject));
	}
	else if (sceneClass == UI_COMPONENT) {
		auto it = scene->uiObjects.find(id);
		std::shared_ptr<vk::GameObject> uiElement = std::move(it->second);
		scene->uiObjects.erase(id);

		this->idToClass.erase(id);

		uiElement->setRemovalCallback(nullptr);

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, uiElement));
	}
	else if (sceneClass == LIGHT) {
		auto it = scene->lights.find(id);
		std::shared_ptr<vk::GameObject> light = std::move(it->second);
		scene->lights.erase(id);

		this->idToClass.erase(id);

		light->setRemovalCallback(nullptr);

		return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, light));
	}
	else if (sceneClass == ENEMY) {
		this->idToClass.erase(id);
		JPH::BodyID bodyID = scene->enemies.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itEnemies = scene->enemies.find(id);
		if (itEnemies != scene->enemies.end()) {
			itEnemies->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> enemy = std::move(itEnemies->second);

			scene->enemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->setRemovalCallback(nullptr);

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, enemy));
		}
		else {
			auto itPassiveEnemies = scene->passiveEnemies.find(id);
			itPassiveEnemies->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> enemy = std::move(itPassiveEnemies->second);

			scene->passiveEnemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->setRemovalCallback(nullptr);

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, enemy));
		}
	}
	else if (sceneClass == PHYSICS_OBJECT) {
		this->idToClass.erase(id);
		JPH::BodyID bodyID = scene->physicsObjects.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itPhysicsObjects = scene->physicsObjects.find(id);
		if (itPhysicsObjects != scene->physicsObjects.end()) {
			itPhysicsObjects->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> physicsObject = std::move(itPhysicsObjects->second);

			scene->physicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->setRemovalCallback(nullptr);

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, physicsObject));
		}
		else {
			auto itPassivePhysicsObjects = scene->passivePhysicsObjects.find(id);
			itPassivePhysicsObjects->second->removePhysicsBody();
			std::shared_ptr<vk::GameObject> physicsObject = std::move(itPassivePhysicsObjects->second);

			scene->passivePhysicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->setRemovalCallback(nullptr);

			return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, physicsObject));
		}
	}
	else {
		return nullptr;
	}
}

std::shared_ptr<physics::Player> SceneManager::getPlayer() {
	return scene->player;
}

std::shared_ptr<lighting::Sun> SceneManager::getSun() {
	return scene->sun;
}

std::vector<std::shared_ptr<physics::Enemy>> SceneManager::getActiveEnemies() const {

	std::vector<std::shared_ptr<physics::Enemy>> enemies{};

	for (auto& pair : scene->enemies) {
		enemies.push_back(pair.second);
	}

	return enemies;
}

std::vector<std::shared_ptr<lighting::PointLight>> SceneManager::getLights() {
	std::vector<std::shared_ptr<lighting::PointLight>> lights{};

	for (auto& pair : scene->lights) {
		lights.push_back(pair.second);
	}

	return lights;
}

std::vector<std::shared_ptr<vk::UIComponent>> SceneManager::getUIObjects() {
	std::vector<std::shared_ptr<vk::UIComponent>> uiObjects{};

	for (auto& pair : scene->uiObjects) {
		uiObjects.push_back(pair.second);
	}

	return uiObjects;
}

std::unique_ptr<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>> SceneManager::getObject(vk::id_t id) {
	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	}
	catch (std::out_of_range& e) {
		// no body with id found in manager
		return nullptr;
	}

	std::shared_ptr<vk::GameObject> gameObject;

	if (sceneClass == SPECTRAL_OBJECT) {
		auto it = scene->spectralObjects.find(id);
		gameObject = it->second;
	}
	else if (sceneClass == UI_COMPONENT) {
		auto it = scene->uiObjects.find(id);
		gameObject = it->second;
	}
	else if (sceneClass == LIGHT) {
		auto it = scene->lights.find(id);
		gameObject = it->second;
	}
	else if (sceneClass == ENEMY) {
		auto itEnemies = scene->enemies.find(id);
		if (itEnemies != scene->enemies.end()) {
			gameObject = itEnemies->second;
		}
		else {
			auto itPassiveEnemies = scene->passiveEnemies.find(id);
			gameObject = itPassiveEnemies->second;
		}
	}
	else if (sceneClass == PHYSICS_OBJECT) {
		auto itPhysicsObjects = scene->physicsObjects.find(id);
		if (itPhysicsObjects != scene->physicsObjects.end()) {
			gameObject = itPhysicsObjects->second;
		}
		else {
			auto itPassivePhysicsObjects = scene->passivePhysicsObjects.find(id);
			gameObject = itPassivePhysicsObjects->second;
		}
	}
	else if (sceneClass == SUN) {
		gameObject = scene->sun;
	}
	else if (sceneClass == PLAYER) {
		gameObject = scene->player;
	}

	// compiler automatically applies move semantics here
	return std::make_unique<std::pair<SceneClass, std::shared_ptr<vk::GameObject>>>(make_pair(sceneClass, gameObject));
}

bool SceneManager::activatePhysicsObject(vk::id_t id) {
	auto it1 = scene->passiveEnemies.find(id);

	if (it1 != scene->passiveEnemies.end()) {
		scene->enemies.emplace(it1->first, std::move(it1->second));
		scene->passiveEnemies.erase(it1);
		it1->second->addPhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	auto it2 = scene->passivePhysicsObjects.find(id);

	if (it2 != scene->passivePhysicsObjects.end()) {
		scene->physicsObjects.emplace(it2->first, std::move(it2->second));
		scene->passivePhysicsObjects.erase(it2);
		it2->second->addPhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	return false;
}

bool SceneManager::detachPhysicsObject(vk::id_t id) {
	auto it1 = scene->enemies.find(id);

	if (it1 != scene->enemies.end()) {
		scene->passiveEnemies.emplace(it1->first, std::move(it1->second));
		scene->enemies.erase(it1);
		it1->second->removePhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	auto it2 = scene->physicsObjects.find(id);

	if (it2 != scene->physicsObjects.end()) {
		scene->passivePhysicsObjects.emplace(it2->first, std::move(it2->second));
		scene->physicsObjects.erase(it2);
		it2->second->removePhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	return false;
}

bool SceneManager::isBroadPhaseOptimizationNeeded() {
	bool isNeeded = this->physicsSceneIsChanged;
	this->physicsSceneIsChanged = false;
	return isNeeded;
}

vk::id_t SceneManager::getIdFromBodyID(JPH::BodyID bodyID) {
	try {
		return this->bodyIDToObjectId.at(bodyID);
	}
	catch (std::out_of_range& e) {
		return vk::INVALID_OBJECT_ID;
	}
}