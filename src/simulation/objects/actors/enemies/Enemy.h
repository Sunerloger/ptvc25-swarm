// TODO entities with behaviour

#pragma once

#include "../../../../GameObject.h"
#include "../../IPhysicsEntity.h"

// TODO abstract standard enemy?

namespace physics {
	class Enemy : public GameObject, public IPhysicsEntity {

	public:

		virtual void postSimulation();
	};
}