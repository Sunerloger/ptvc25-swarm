#pragma once

#include "../../../../GameObject.h"
#include "../../IPhysicsEntity.h"
#include "../Player.h"

namespace physics {
	class Enemy : public vk::GameObject, public IPhysicsEntity {

	public:

		virtual ~Enemy() = default;

		// also sets model to position in simulation
		virtual void postSimulation() = 0;

		virtual float getMaxHealth() const = 0;
		virtual float getCurrentHealth() const = 0;

		// may destroy the object
		virtual void subtractHealth(float healthToSubtract) = 0;

		virtual void update(std::weak_ptr<physics::Player> player) = 0;

		virtual void setViewDirection(glm::vec3 direction) = 0;
		virtual void setViewTarget(glm::vec3 target) = 0;

		virtual void printPosition(int iterationStep) const = 0;
	};
}

// TODO some enemies could be able to jump