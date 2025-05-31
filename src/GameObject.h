#pragma once

#include "vk/vk_model.h"

#include <functional>

namespace vk {

	using id_t = unsigned int;

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
		GameObject& operator=(GameObject&&) = delete;

		inline id_t getId() const {
			return id;
		}

		virtual glm::mat4 computeModelMatrix() const = 0;

		virtual glm::mat4 computeNormalMatrix() const = 0;

		virtual glm::vec3 getPosition() const = 0;

		// returns a nullptr if object has no model (e.g. light)
		virtual std::shared_ptr<Model> getModel() const = 0;

		virtual void toggleWireframeModeIfSupported() {};

		/**
		 * The object is added to a queue of objects to destroy in the scene manager - it is still alive for now, but gets removed in the cleanup phase.
		 * Doesn't destroy player or sun.
		 * @return true if a deletion assignment in the scene manager happened
		 * @return false otherwise (e.g. object not in scene manager)
		 */
		bool markForDeletion() const;

	   protected:
		GameObject() : id(nextID++) {}

		const id_t id;
	};
}