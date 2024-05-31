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

		// @return true if enemy gets destroyed
		virtual bool subtractHealth(float healthToSubtract) = 0;

		virtual void update() = 0;

		virtual void printPosition(int iterationStep) const = 0;
	};
}

// TODO some enemies could be able to jump