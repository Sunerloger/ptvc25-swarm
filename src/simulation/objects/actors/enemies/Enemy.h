// TODO entities with behaviour

#pragma once

#include "../../../../GameObject.h"
#include "../../IPhysicsEntity.h"

// TODO abstract standard enemy?

namespace physics {
	class Enemy : public vk::GameObject, public IPhysicsEntity {

	public:

		virtual void postSimulation();
	};
}