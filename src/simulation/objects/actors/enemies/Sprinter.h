#pragma once

#include "Enemy.h"

namespace physics {

	struct SprinterSettings {
		// update per second
		float movementSpeed = 6.0f;

		// in [0, 1], 0 = doesn't rotate, 1 = front is always facing player
		float rotationSpeed = 0.01f;

		// how much can the player not be directly in front of the enemy for it to still charge (in radians)
		float movementAngle = 0.37f;

		float maxFloorSeparationDistance = 0.05f;
		float maxHealth = 100.0f;

		std::shared_ptr<vk::Model> model;
	};

	struct SprinterCreationSettings {
		JPH::RVec3Arg position = JPH::RVec3::sZero();

		std::unique_ptr<JPH::CharacterSettings> characterSettings;
		std::unique_ptr<SprinterSettings> sprinterSettings;

		JPH::uint64 inUserData = 0;
	};

	class Sprinter : public Enemy {

	public:
		
		Sprinter(std::unique_ptr<SprinterCreationSettings> sprinterCreationSettings, std::shared_ptr<JPH::PhysicsSystem> physics_system);
		virtual ~Sprinter();

		JPH::BodyID getBodyID() override;

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<vk::Model> getModel() const override;

		void postSimulation() override;

		float getMaxHealth() const override;
		float getCurrentHealth() const override;

		// @return true if enemy gets destroyed
		bool subtractHealth(float healthToSubtract) override;

		void update() override;

		void printPosition(int iterationStep) const override;

	private:

		float currentHealth;

		std::unique_ptr<JPH::CharacterSettings> characterSettings;
		std::unique_ptr<SprinterSettings> sprinterSettings;

		std::unique_ptr<JPH::Character> character;

		std::shared_ptr<JPH::PhysicsSystem> physics_system;

		float calculateTargetAngle();

		JPH::RVec3 getDirectionToCharacter();
	};
}