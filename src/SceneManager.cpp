#include "SceneManager.h"

void SceneManager::setPlayer(std::unique_ptr<Player> newPlayer) {

	if (scene->player != nullptr) {
		scene->player->removePhysicsBody();
	}

	scene->player = std::move(newPlayer);

	scene->player->addPhysicsBody();

	this->physicsSceneIsChanged = true;
}

void SceneManager::setSun(std::unique_ptr<Sun> sun) {
	scene->sun = std::move(sun);
}

bool SceneManager::addSpectralObject(std::unique_ptr<GameObject> spectralObject) {
	id_t id = spectralObject->getId();
	std::pair result = this->scene->spectralObjects.emplace(id, std::move(spectralObject));
	return result.second;
}

bool SceneManager::addUIObject(std::unique_ptr<GameObject> uiObject) {
	id_t id = uiObject->getId();
	std::pair result = this->scene->uiObjects.emplace(id, std::move(uiObject));
	return result.second;
}

bool SceneManager::addLight(std::unique_ptr<PointLight> light) {
	id_t id = light->getId();
	std::pair result = this->scene->lights.emplace(id, std::move(light));
	return result.second;
}

bool SceneManager::addEnemy(std::unique_ptr<Enemy> enemy) {
	id_t id = enemy->getId();

	if (scene->passiveEnemies.find(id) != scene->passiveEnemies.end()) {
		return false;
	}

	std::pair result = this->scene->enemies.emplace(id, std::move(enemy));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	return false;
}

bool SceneManager::addManagedPhysicsEntity(std::unique_ptr<ManagedPhysicsEntity> managedPhysicsEntity) {
	id_t id = managedPhysicsEntity->getId();

	if (scene->passivePhysicsObjects.find(id) != scene->passivePhysicsObjects.end()) {
		return false;
	}

	std::pair result = this->scene->physicsObjects.emplace(id, std::move(managedPhysicsEntity));

	if (result.second) {
		result.first->second->addPhysicsBody();
		this->physicsSceneIsChanged = true;
		return true;
	}

	return false;
}

bool SceneManager::deleteSpectralObject(id_t id) {
	return scene->spectralObjects.erase(id) == 1;
}

bool SceneManager::deleteUIObject(id_t id) {
	return scene->uiObjects.erase(id) == 1;
}

bool SceneManager::deleteLight(id_t id) {
	return scene->lights.erase(id) == 1;
}

bool SceneManager::deleteManagedPhysicsEntity(id_t id) {
	if (scene->physicsObjects.erase(id) == 1 || scene->passivePhysicsObjects.erase(id) == 1) {
		this->physicsSceneIsChanged = true;
		return true;
	}
	return false;
}

bool SceneManager::deleteEnemy(id_t id) {
	if (scene->enemies.erase(id) == 1 || scene->passiveEnemies.erase(id) == 1) {
		this->physicsSceneIsChanged = true;
		return true;
	}
	return false;
}

Player* SceneManager::getPlayer() {
	return scene->player.get();
}

Sun* SceneManager::getSun() {
	return scene->sun.get();
}

vector<Enemy*> SceneManager::getAllEnemies() const {

	vector<Enemy*> enemies{};

	for (auto& pair : scene->enemies) {
		enemies.push_back(pair.second.get());
	}

	return enemies;
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