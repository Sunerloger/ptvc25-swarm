#include "Grenade.h"
#include "../../../scene/SceneManager.h"
#include "../actors/enemies/Enemy.h"
#include "../../PhysicsConversions.h"
#include "../../CollisionSettings.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>

#include <iostream>

namespace physics {

	Grenade::Grenade(const GrenadeCreationSettings& creationSettings, JPH::PhysicsSystem& physics_system)
		: ManagedPhysicsEntity(physics_system), settings(creationSettings.grenadeSettings), model(creationSettings.model) {
		creationTime = std::chrono::steady_clock::now();

		createPhysicsBody(creationSettings.position, creationSettings.initialVelocity);

		if (settings.enableDebugOutput) {
			std::cout << "Grenade created at position ("
					  << creationSettings.position.GetX() << ", "
					  << creationSettings.position.GetY() << ", "
					  << creationSettings.position.GetZ() << ")" << std::endl;
		}
	}

	void Grenade::createPhysicsBody(const JPH::RVec3& position, const JPH::Vec3& initialVelocity) {
		// Create sphere shape for grenade
		JPH::Ref<JPH::SphereShape> sphere_shape = new JPH::SphereShape(settings.radius);

		// Create body creation settings
		JPH::BodyCreationSettings body_settings(
			sphere_shape,
			position,
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Dynamic,
			physics::Layers::MOVING);

		body_settings.mMassPropertiesOverride.mMass = settings.mass;
		body_settings.mLinearVelocity = initialVelocity;
		body_settings.mFriction = 0.8f;		  // Some friction for bouncing
		body_settings.mRestitution = 0.3f;	  // Some bounciness
		body_settings.mLinearDamping = 0.1f;  // Air resistance
		body_settings.mAngularDamping = 0.1f;

		// Create the body
		JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
		bodyID = body_interface.CreateAndAddBody(body_settings, JPH::EActivation::Activate);
	}

	void Grenade::addPhysicsBody() {
		// Body is already added in constructor via CreateAndAddBody
		// This method is called by SceneManager, but we don't need to do anything here
		// since the body was already added to avoid timing issues
	}

	void Grenade::updatePhysics(float deltaTime) {
		if (markedForDeletion) {
			return;
		}

		if (exploded) {
			// Check if enough time has passed since explosion to safely delete
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - explosionTime);
			if (elapsed.count() >= (DELETION_DELAY * 1000.0f)) {
				markForDeletion();
				markedForDeletion = true;
			}
			return;
		}

		// Check if grenade should explode based on fuse timer
		if (shouldExplode()) {
			explode();
		}
	}

	bool Grenade::shouldExplode() const {
		if (exploded) {
			return false;
		}

		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - creationTime);
		return elapsed.count() >= (settings.fuseTime * 1000.0f);
	}

	void Grenade::explode() {
		if (exploded) {
			return;
		}

		exploded = true;
		explosionTime = std::chrono::steady_clock::now();

		JPH::RVec3 explosionCenter = physics_system.GetBodyInterface().GetPosition(bodyID);

		if (settings.enableDebugOutput) {
			std::cout << "Grenade exploded at position ("
					  << explosionCenter.GetX() << ", "
					  << explosionCenter.GetY() << ", "
					  << explosionCenter.GetZ() << ")" << std::endl;
		}

		// Get all enemies in range and damage them
		SceneManager& sceneManager = SceneManager::getInstance();
		auto enemies = sceneManager.getActiveEnemies();

		int enemiesHit = 0;
		for (auto& weakEnemy : enemies) {
			if (auto enemy = weakEnemy.lock()) {
				glm::vec3 enemyPos = enemy->getPosition();
				glm::vec3 grenadePos = RVec3ToGLM(explosionCenter);

				float distance = glm::length(enemyPos - grenadePos);

				if (distance <= settings.explosionRadius) {
					// Calculate damage falloff based on distance
					float damageMultiplier = 1.0f - (distance / settings.explosionRadius);
					damageMultiplier = glm::max(0.1f, damageMultiplier);  // Minimum 10% damage

					float actualDamage = settings.explosionDamage * damageMultiplier;

					// Calculate knockback direction
					glm::vec3 knockbackDir = glm::normalize(enemyPos - grenadePos);
					if (glm::length(knockbackDir) < 0.01f) {
						knockbackDir = glm::vec3(0, 1, 0);	// Default upward if too close
					}

					float knockbackStrength = 20.0f * damageMultiplier;

					bool isDead = enemy->takeDamage(actualDamage, knockbackDir, knockbackStrength);
					enemiesHit++;

					if (settings.enableDebugOutput) {
						std::cout << "Enemy hit by grenade explosion. Distance: " << distance
								  << ", Damage: " << actualDamage << ", Dead: " << (isDead ? "Yes" : "No") << std::endl;
					}
				}
			}
		}

		if (settings.enableDebugOutput) {
			std::cout << "Grenade explosion hit " << enemiesHit << " enemies within radius " << settings.explosionRadius << std::endl;
		}

		// Don't mark for deletion immediately - wait for the delay in updatePhysics
	}

	glm::mat4 Grenade::computeModelMatrix() const {
		if (bodyID.IsInvalid()) {
			return glm::mat4(1.0f);
		}

		JPH::RVec3 position = physics_system.GetBodyInterface().GetPosition(bodyID);
		JPH::Quat rotation = physics_system.GetBodyInterface().GetRotation(bodyID);

		glm::vec3 pos = RVec3ToGLM(position);
		glm::quat rot = QuatToGLM(rotation);

		glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
		glm::mat4 R = glm::toMat4(rot);
		glm::mat4 R_correction = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(settings.radius * 2.0f * 10.0f));

		return T * R * R_correction * S;
	}

	glm::mat4 Grenade::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(computeModelMatrix()));
	}

	glm::vec3 Grenade::getPosition() const {
		if (bodyID.IsInvalid()) {
			return glm::vec3(0.0f);
		}
		return RVec3ToGLM(physics_system.GetBodyInterface().GetPosition(bodyID));
	}

	std::shared_ptr<vk::Model> Grenade::getModel() const {
		return model;
	}
}
