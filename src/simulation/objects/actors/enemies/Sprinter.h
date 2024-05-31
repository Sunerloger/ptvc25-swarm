#pragma once

#include "Enemy.h"

namespace physics {

	struct SprinterSettings {
		// update per second
		float movementSpeed = 7.0f;
		float maxFloorSeparationDistance = 0.05f;
		float maxHealth = 100.0f;

		vk::Model* model;
	};

	struct SprinterCreationSettings {
		JPH::RVec3Arg position = JPH::RVec3::sZero();

		SprinterSettings* sprinterSettings;

		JPH::uint64 inUserData = 0;
	};

	class Sprinter : public Enemy {

	public:

		static JPH::CharacterSettings* characterSettings;
		
		Sprinter(SprinterCreationSettings sprinterCreationSettings);
		virtual ~Sprinter();

		JPH::BodyID getBodyID() override;

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		glm::mat4 computeModelMatrix() const override;
		glm::mat4 computeNormalMatrix() const override;
		glm::vec3 getPosition() const override;
		vk::Model* getModel() const override;

		// TODO needs to also set model to position in simulation
		void postSimulation() override;

		float getMaxHealth() const override;
		float getCurrentHealth() const override;

		// may destroy the object
		void subtractHealth(float healthToSubtract) override;

		void update(std::weak_ptr<physics::Player> player) override;

		void setViewDirection(glm::vec3 direction) override;
		void setViewTarget(glm::vec3 target) override;

		void printPosition(int iterationStep) const override;

	private:

		float currentHealth = 100.0f;

		std::unique_ptr<SprinterSettings> sprinterSettings;

		std::unique_ptr<JPH::Character> character;

	};
}