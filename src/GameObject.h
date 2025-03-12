#pragma once

#include "vk_model.h"
#include "scene/ISceneManagerInteraction.h"

#include <functional>

namespace vk {

	constexpr id_t INVALID_OBJECT_ID = 0;

	class GameObject {

	private:

		static inline id_t nextID = 1;

	public:

		// only destroy object through scene manager if registered in scene manager or through method destroy
		virtual ~GameObject() = default;

		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;

		inline id_t getId() const { return id; }

		virtual glm::mat4 computeModelMatrix() const = 0;

		virtual glm::mat4 computeNormalMatrix() const = 0;

		virtual glm::vec3 getPosition() const = 0;

		// returns a nullptr if object has no model (e.g. light)
		virtual std::shared_ptr<Model> getModel() const = 0;

		inline void setSceneManager(std::weak_ptr<ISceneManagerInteraction> sceneManager) {
			sceneManagerInteraction = sceneManager;
		}

		inline void deleteSceneManager() {
			sceneManagerInteraction.reset();
		}

		/**
		* The object is added to a queue of objects to destroy in the scene manager - it is still alive for now, but gets removed in the cleanup phase.
		* Doesn't destroy player or sun.
		* @return true if a deletion assignment in the scene manager happened
		* @return false otherwise (e.g. object not in scene manager)
		*/
		inline bool markForDeletion() {
			if (auto sceneManager = sceneManagerInteraction.lock()) {
				return sceneManager->addToStaleQueue(id);
			}
			return false;
		}

		glm::vec3 color{};

	protected:

		GameObject() : id(nextID++) {}

		const id_t id;

		std::weak_ptr<ISceneManagerInteraction> sceneManagerInteraction;
	};
}