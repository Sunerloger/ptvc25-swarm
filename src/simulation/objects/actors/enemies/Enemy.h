#pragma once

#include "../../../../GameObject.h"
#include "../../IPhysicsEntity.h"
#include "../Player.h"

namespace physics {
	class Enemy : public vk::GameObject, public IPhysicsEntity {

	public:

		virtual ~Enemy() = default;

		// may also need to map model to physics object
		virtual void postSimulation() = 0;

		virtual float getMaxHealth() const = 0;
		virtual float getCurrentHealth() const = 0;

		virtual float getBaseDamage() const = 0;

		// @return true if enemy gets destroyed
		virtual bool takeDamage(float healthToSubtract, glm::vec3 direction = glm::vec3(0.0f), float knockbackStrength = 0.0f) = 0;

		virtual void updatePhysics(float cPhysicsDeltaTime) = 0;
		virtual void updateVisuals(float deltaTime) = 0;

		virtual void printInfo(int iterationStep) const = 0;
	};
}

// TODO some enemies could be able to jump