#pragma once

#include <memory>

namespace vk {
	using id_t = unsigned int;
}

namespace physics {
	class Player;
}

// interface enables game objects to interact with the scene manager
class ISceneManagerInteraction {

public:

	virtual std::shared_ptr<physics::Player> getPlayer() = 0;

	// @return true if the game object could be found and added to queue of objects to delete, does not delete player or sun
	virtual bool addToStaleQueue(vk::id_t id) = 0;
};