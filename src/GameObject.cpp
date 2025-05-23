#include "GameObject.h"

#include "scene/SceneManager.h"

namespace vk {
	bool GameObject::markForDeletion() const {

		SceneManager& sceneManager = SceneManager::getInstance();

		return sceneManager.addToStaleQueue(id);
	}
}