#pragma once

#include "Enemy.h"

namespace physics {

	class Sprinter : public Enemy {

	public:

		struct SprinterSettings {
			// m/s
			float maxMovementSpeed = 7.0f;

			// m/s^2
			float accelerationToMaxSpeed = 1.0f;

			// seconds to turn around 180 degrees 
			float rotationTime = 3.0f;

			// how much can the player not be directly in front of the enemy for it to still charge (in radians)
			float movementAngle = 0.5f;

			float maxFloorSeparationDistance = 0.05f;
			float maxHealth = 100.0f;

			std::shared_ptr<vk::Model> model;
		};

		struct SprinterCreationSettings {
			JPH::RVec3 position = JPH::RVec3::sZero();

			JPH::CharacterSettings characterSettings;
			SprinterSettings sprinterSettings;

			JPH::uint64 inUserData = 0;
		};
		
		Sprinter(SprinterCreationSettings sprinterCreationSettings, JPH::PhysicsSystem& physics_system);
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
		bool takeDamage(float healthToSubtract, glm::vec3 direction = glm::vec3(0.0f), float knockbackStrength = 0.0f) override;

		void update(float cPhysicsDeltaTime) override;

		void printInfo(int iterationStep) const override;

	private:

		float currentHealth;

		JPH::CharacterSettings characterSettings;
		SprinterSettings sprinterSettings;

		std::unique_ptr<JPH::Character> character;

		JPH::PhysicsSystem& physics_system;

		float calculateTargetAngle();

		JPH::RVec3 getDirectionToCharacter();
	};
}