// TODO entities with behaviour

#pragma once

#include "../../../../GameObject.h"
#include "../../IPhysicsEntity.h"

// TODO abstract standard enemy?

namespace physics {
	class Enemy : public vk::GameObject, public IPhysicsEntity {

	public:

		Enemy();
		virtual ~Enemy();

		virtual void postSimulation();

		virtual BodyID getBodyID();

	private:
		// TODO set this in subclass depending on enemy
		float maxHealth = 100.0f;
		float currentHealth = 100.0f;
	};
}