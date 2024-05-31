#pragma once

#include "Enemy.h"

namespace physics {

	struct SprinterSettings {
		// update per second
		float movementSpeed = 5.0f;
		float maxFloorSeparationDistance = 0.05f;
		float maxHealth = 100.0f;

		std::shared_ptr<vk::Model> model;
	};

	struct SprinterCreationSettings {
		JPH::RVec3Arg position = JPH::RVec3::sZero();

		std::unique_ptr<SprinterSettings> sprinterSettings;

		JPH::uint64 inUserData = 0;

		float yaw = 0;
	};

	class Sprinter : public Enemy {

	public:

		static std::unique_ptr<JPH::CharacterSettings> characterSettings;
		
		Sprinter(std::unique_ptr<SprinterCreationSettings> sprinterCreationSettings, std::shared_ptr<JPH::PhysicsSystem> physics_system);
		virtual ~Sprinter();

		JPH::BodyID getBodyID() override;

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		std::shared_ptr<vk::Model> getModel() const override;

		// TODO needs to also set model to position in simulation
		void postSimulation() override;

		float getMaxHealth() const override;
		float getCurrentHealth() const override;

		// may destroy the object
		void subtractHealth(float healthToSubtract) override;

		void update() override;

		void setViewDirection(glm::vec3 direction) override;
		void setViewTarget(glm::vec3 target) override;

		void printPosition(int iterationStep) const override;

	private:

		float currentHealth;
		float yaw;

		std::unique_ptr<SprinterSettings> sprinterSettings;

		std::unique_ptr<JPH::Character> character;

		std::shared_ptr<JPH::PhysicsSystem> physics_system;
	};
}