#include "SceneManager.h"

id_t SceneManager::setPlayer(std::unique_ptr<Player> newPlayer) {

	// TODO remove old id from maps

	scene->player = std::move(newPlayer);

	scene->player->addPhysicsBody();

	this->physicsSceneIsChanged = true;

	return scene->player->getId();
}

id_t SceneManager::setSun(std::unique_ptr<Sun> sun) {
	// TODO remove old id from maps
	scene->sun = std::move(sun);
	return scene->sun->getId();
}

id_t SceneManager::addSpectralObject(std::unique_ptr<GameObject> spectralObject) {
	id_t id = spectralObject->getId();
	std::pair result = this->scene->spectralObjects.emplace(id, std::move(spectralObject));

	if (result.second) {
		return id;
	}
	else {
		return INVALID_OBJECT_ID;
	}
}

id_t SceneManager::addUIObject(std::unique_ptr<UIComponent> uiObject) {
	id_t id = uiObject->getId();
	std::pair result = this->scene->uiObjects.emplace(id, std::move(uiObject));

	if (result.second) {
		return id;
	}
	else {
		return INVALID_OBJECT_ID;
	}
}

id_t SceneManager::addLight(std::unique_ptr<lighting::PointLight> light) {
	id_t id = light->getId();
	std::pair result = this->scene->lights.emplace(id, std::move(light));

	if (result.second) {
		return id;
	}
	else {
		return INVALID_OBJECT_ID;
	}
}

id_t SceneManager::addEnemy(std::unique_ptr<Enemy> enemy) {
	id_t id = enemy->getId();
	BodyID bodyID = enemy->getBodyID();

	if (scene->passiveEnemies.find(id) != scene->passiveEnemies.end()) {
		return INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->enemies.emplace(id, std::move(enemy));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return INVALID_OBJECT_ID;
}

id_t SceneManager::addManagedPhysicsEntity(std::unique_ptr<ManagedPhysicsEntity> managedPhysicsEntity) {
	id_t id = managedPhysicsEntity->getId();
	BodyID bodyID = managedPhysicsEntity->getBodyID();

	if (scene->passivePhysicsObjects.find(id) != scene->passivePhysicsObjects.end()) {
		return INVALID_OBJECT_ID;
	}

	std::pair result = this->scene->physicsObjects.emplace(id, std::move(managedPhysicsEntity));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->bodyIDToObjectId.emplace(bodyID, id);
		this->physicsSceneIsChanged = true;
		return id;
	}

	return INVALID_OBJECT_ID;
}

bool SceneManager::deleteGameObject(id_t id) {

	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	}
	catch (out_of_range& e) {
		return false;
	}

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
		BodyID bodyID = scene->enemies.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);
		this->physicsSceneIsChanged = true;
		return true;

	case PHYSICS_OBJECT:
		// one of the maps contains the physics object
		scene->physicsObjects.erase(id);
		scene->passivePhysicsObjects.erase(id);

		this->idToClass.erase(id);
		BodyID bodyID = scene->physicsObjects.at(id)->getBodyID();
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

unique_ptr<pair<SceneClass, shared_ptr<GameObject>>> SceneManager::removeGameObject(id_t id) {

	SceneClass sceneClass;

	try {
		sceneClass = this->idToClass.at(id);
	}
	catch (out_of_range& e) {
		return nullptr;
	}

	if (sceneClass == SPECTRAL_OBJECT) {
		auto it = scene->spectralObjects.find(id);
		shared_ptr<GameObject> spectralObject = std::move(it->second);
		scene->spectralObjects.erase(id);

		this->idToClass.erase(id);

		spectralObject->sceneManager.reset();

		// compiler automatically applies move semantics here
		return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, spectralObject));
	}
	else if (sceneClass == UI_COMPONENT) {
		auto it = scene->uiObjects.find(id);
		shared_ptr<GameObject> uiElement = std::move(it->second);
		scene->uiObjects.erase(id);

		this->idToClass.erase(id);

		uiElement->sceneManager.reset();

		return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, uiElement));
	}
	else if (sceneClass == LIGHT) {
		auto it = scene->lights.find(id);
		shared_ptr<GameObject> light = std::move(it->second);
		scene->lights.erase(id);

		this->idToClass.erase(id);

		light->sceneManager.reset();

		return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, light));
	}
	else if (sceneClass == ENEMY) {
		this->idToClass.erase(id);
		BodyID bodyID = scene->enemies.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itEnemies = scene->enemies.find(id);
		if (itEnemies != scene->enemies.end()) {
			itEnemies->second->removePhysicsBody();
			shared_ptr<GameObject> enemy = std::move(itEnemies->second);

			scene->enemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->sceneManager.reset();

			return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, enemy));
		}
		else {
			auto itPassiveEnemies = scene->passiveEnemies.find(id);
			itPassiveEnemies->second->removePhysicsBody();
			shared_ptr<GameObject> enemy = std::move(itPassiveEnemies->second);

			scene->passiveEnemies.erase(id);
			this->physicsSceneIsChanged = true;

			enemy->sceneManager.reset();

			return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, enemy));
		}
	}
	else if (sceneClass == PHYSICS_OBJECT) {
		this->idToClass.erase(id);
		BodyID bodyID = scene->physicsObjects.at(id)->getBodyID();
		this->bodyIDToObjectId.erase(bodyID);

		auto itPhysicsObjects = scene->physicsObjects.find(id);
		if (itPhysicsObjects != scene->physicsObjects.end()) {
			itPhysicsObjects->second->removePhysicsBody();
			shared_ptr<GameObject> physicsObject = std::move(itPhysicsObjects->second);

			scene->physicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->sceneManager.reset();

			return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, physicsObject));
		}
		else {
			auto itPassivePhysicsObjects = scene->passivePhysicsObjects.find(id);
			itPassivePhysicsObjects->second->removePhysicsBody();
			shared_ptr<GameObject> physicsObject = std::move(itPassivePhysicsObjects->second);

			scene->passivePhysicsObjects.erase(id);
			this->physicsSceneIsChanged = true;

			physicsObject->sceneManager.reset();

			return make_unique<pair<SceneClass, shared_ptr<GameObject>>>(make_pair(sceneClass, physicsObject));
		}
	}
	else {
		return nullptr;
	}	
}

shared_ptr<Player> SceneManager::getPlayer() {
	return scene->player;
}

shared_ptr<Sun> SceneManager::getSun() {
	return scene->sun;
}

vector<shared_ptr<Enemy>> SceneManager::getActiveEnemies() const {

	vector<shared_ptr<Enemy>> enemies{};

	for (auto& pair : scene->enemies) {
		enemies.push_back(pair.second);
	}

	return enemies;
}

vector<std::shared_ptr<PointLight>> SceneManager::getLights() {
	vector<shared_ptr<PointLight>> lights{};

	for (auto& pair : scene->lights) {
		lights.push_back(pair.second);
	}

	return lights;
}

vector<std::shared_ptr<UIComponent>> SceneManager::getUIObjects() {
	vector<shared_ptr<UIComponent>> uiObjects{};

	for (auto& pair : scene->uiObjects) {
		uiObjects.push_back(pair.second);
	}

	return uiObjects;
}

bool SceneManager::activatePhysicsObject(id_t id) {
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

bool SceneManager::detachPhysicsObject(id_t id) {
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

id_t SceneManager::getIdFromBodyID(BodyID bodyID) {
	try {
		return this->bodyIDToObjectId.at(bodyID);
	}
	catch (out_of_range& e) {
		return INVALID_OBJECT_ID;
	}
}