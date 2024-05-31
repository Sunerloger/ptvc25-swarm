#include "Sprinter.h"

namespace physics {

	// TODO
	std::unique_ptr<JPH::CharacterSettings> Sprinter::characterSettings = std::move(std::make_unique<JPH::CharacterSettings>());

	Sprinter::Sprinter(std::unique_ptr<SprinterCreationSettings> sprinterCreationSettings, std::shared_ptr<JPH::PhysicsSystem> physics_system) {
		current
	}

	Sprinter::~Sprinter() {
		
	}

	glm::vec3 Sprinter::getPosition() const {
		return RVec3ToGLM(this->character->GetPosition());
	}
}