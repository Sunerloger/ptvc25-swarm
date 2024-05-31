#pragma once

#include "vk_model.h"

#include <functional>

namespace vk {

	using id_t = unsigned int;
	using DestroyCallback = std::function<bool(id_t)>;
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

		id_t getId() const { return id; }

		virtual glm::mat4 computeModelMatrix() const = 0;

		virtual glm::mat4 computeNormalMatrix() const = 0;

		virtual glm::vec3 getPosition() const = 0;

		// returns a nullptr if object has no model (e.g. light)
		virtual Model* getModel() const = 0;

		void setDestroyCallback(DestroyCallback callback) {
			destroyCallback = callback;
		}

		/**
		* The object is destroyed in the scene manager - it could still be alive if shared pointers still point to it in other places.
		* Be sure that the shared pointer that calls this method falls out of scope or is resetted,
		* otherwise e.g. physics related objects are not removed from the simulation.
		* Doesn't destroy player or sun.
		* @return true if a deletion in the scene manager happened
		* @return false otherwise (e.g. object not in scene manager)
		*/
		bool destroy() {
			if (destroyCallback) {
				return destroyCallback(id);
			}
			return false;
		}

		glm::vec3 color{};

	protected:

		GameObject() : id(nextID++) {}

		const id_t id;

		DestroyCallback destroyCallback;
	};
}