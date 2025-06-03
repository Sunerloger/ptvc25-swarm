#include "PhysicsPlayer.h"

#include "../../../scene/SceneManager.h"
#include "enemies/Enemy.h"
#include "../dynamic/Grenade.h"

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <iostream>

namespace physics {

	PhysicsPlayer::PhysicsPlayer(PlayerCreationSettings playerCreationSettings, JPH::PhysicsSystem& physics_system) : settings(playerCreationSettings.playerSettings), characterSettings(playerCreationSettings.characterSettings), physics_system(physics_system), camera(playerCreationSettings.cameraSettings) {
		this->character = std::unique_ptr<JPH::Character>(new JPH::Character(&this->characterSettings, playerCreationSettings.position, playerCreationSettings.rotation, playerCreationSettings.inUserData, &this->physics_system));
	}

	PhysicsPlayer::~PhysicsPlayer() {
		removePhysicsBody();
	}

	void PhysicsPlayer::addPhysicsBody() {
		character->AddToPhysicsSystem();
	}

	void PhysicsPlayer::removePhysicsBody() {
		character->RemoveFromPhysicsSystem();
	}

	void PhysicsPlayer::setInputDirection(const glm::vec3 dir) {
		this->currentMovementDirection = dir;
	}

	void PhysicsPlayer::handleMovement(float deltaTime) {
		// deltaTime can be used for e.g. setting a fixed time it should take for the character to reach max velocity

		if (currentMovementDirection == glm::vec3{0, 0, 0}) {
			return;
		}

		JPH::Vec3 playerMovementDirection = GLMToRVec3(currentMovementDirection);

		float yaw = glm::radians(camera.getYaw());
		JPH::RMat44 rotation_matrix = JPH::RMat44::sIdentity().sRotationY(yaw);

		JPH::Vec3 movementDirectionWorld = JPH::Vec3(rotation_matrix * JPH::Vec4(playerMovementDirection, 1));

		// Cancel movement in opposite direction of normal when touching something we can't walk up
		JPH::Character::EGroundState ground_state = this->character->GetGroundState();
		if (ground_state == JPH::Character::EGroundState::OnSteepGround || ground_state == JPH::Character::EGroundState::NotSupported) {
			JPH::Vec3 normal = character->GetGroundNormal();
			normal.SetY(0.0f);
			float normal_length_sq = normal.LengthSq();
			if (normal_length_sq > 0.0f) {
				float dot = normal.Dot(movementDirectionWorld);
				if (dot < 0.0f) {
					movementDirectionWorld -= (dot * normal) / normal_length_sq;
				}
			}
		}

		if (settings.controlMovementDuringJump || character->IsSupported()) {
			JPH::Vec3 current_velocity = character->GetLinearVelocity();
			JPH::Vec3 desired_velocity = settings.movementSpeed * movementDirectionWorld;
			desired_velocity.SetY(current_velocity.GetY());
			JPH::Vec3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

			character->SetLinearVelocity(new_velocity);
		}
		// TODO test different speeds for different directions, double jump, different movement speed in air
	}

	void PhysicsPlayer::handleJump() {
		if (settings.controlMovementDuringJump || character->IsSupported()) {
			JPH::Vec3 new_velocity = character->GetLinearVelocity();

			// Jump - OnGround also means you have friction
			JPH::Character::EGroundState ground_state = this->character->GetGroundState();
			if (ground_state == JPH::Character::EGroundState::OnGround) {
				new_velocity.SetY(std::sqrt(2 * settings.jumpHeight * characterSettings.mGravityFactor * 9.81f));
			}

			// Update the velocity
			character->SetLinearVelocity(new_velocity);
		}
	}

	void PhysicsPlayer::handleThrowGrenade(vk::Device& device, std::shared_ptr<vk::Model> grenadeModel) {
		SceneManager& sceneManager = SceneManager::getInstance();

		// Get player position and camera direction
		JPH::RVec3 playerPosition = character->GetPosition();
		glm::vec3 cameraPosition = camera.getPosition();
		glm::vec3 forward = camera.getFront();

		// Calculate throw position slightly in front of player to avoid self-collision
		JPH::RVec3 throwPosition = playerPosition + JPH::RVec3(forward.x * 1.5f, forward.y * 1.5f + 1.0f, forward.z * 1.5f);

		// Calculate throw velocity with forward momentum and upward arc
		float throwForce = 15.0f;	  // Base throwing speed
		float upwardVelocity = 8.0f;  // Upward component for grenade arc
		JPH::Vec3 throwVelocity = JPH::Vec3(forward.x * throwForce, forward.y * throwForce + upwardVelocity, forward.z * throwForce);

		// Create grenade settings
		physics::Grenade::GrenadeSettings grenadeSettings;
		grenadeSettings.explosionRadius = 8.0f;
		grenadeSettings.explosionDamage = 75.0f;
		grenadeSettings.fuseTime = 3.0f;
		grenadeSettings.mass = 0.5f;
		grenadeSettings.radius = 0.1f;
		grenadeSettings.enableDebugOutput = true;

		// Create grenade creation settings
		physics::Grenade::GrenadeCreationSettings grenadeCreationSettings;
		grenadeCreationSettings.position = throwPosition;
		grenadeCreationSettings.initialVelocity = throwVelocity;
		grenadeCreationSettings.grenadeSettings = grenadeSettings;
		grenadeCreationSettings.model = grenadeModel;  // Use the shared model instead of loading it

		// Create the grenade and add it to the scene
		auto grenade = std::make_unique<physics::Grenade>(grenadeCreationSettings, physics_system);

		// Add grenade to scene manager for rendering and physics updates
		sceneManager.addManagedPhysicsEntity(std::move(grenade));

		std::cout << "Grenade thrown!" << std::endl;
	}

	void PhysicsPlayer::handleShoot() {
		SceneManager& sceneManager = SceneManager::getInstance();

		JPH::RVec3 origin = GLMToRVec3(camera.getPosition());

		glm::vec3 forward = camera.getFront();
		JPH::Vec3 direction = GLMToRVec3(forward) * settings.shootRange;

		JPH::RRayCast ray{origin, direction};

		JPH::IgnoreSingleBodyFilter filter(character->GetBodyID());

		JPH::RayCastResult result;
		bool hit = physics_system.GetNarrowPhaseQuery().CastRay(
			ray,
			result,
			JPH::BroadPhaseLayerFilter(),
			JPH::ObjectLayerFilter(),
			filter);

		if (hit) {
			JPH::BodyID hitBodyID = result.mBodyID;

			vk::id_t hitObjectID = sceneManager.getIdFromBodyID(hitBodyID);
			auto sceneObject = sceneManager.getObject(hitObjectID);
			if (sceneObject.first == SceneClass::ENEMY) {
				std::cout << "Hit enemy with ID: " << hitBodyID.GetIndexAndSequenceNumber() << std::endl;
				auto enemy = static_cast<Enemy*>(sceneObject.second);
				if (enemy) {
					bool isDead = enemy->takeDamage(settings.shootDamage, camera.getFront(), settings.knockbackSpeed);
					std::cout << "Enemy took damage. New health: " << enemy->getCurrentHealth() << "/" << enemy->getMaxHealth() << std::endl;
					if (isDead) {
						std::cout << "Enemy died" << std::endl;
					}
				} else {
					std::cout << "Error: Enemy didn't take any damage because cast to class failed!" << std::endl;
				}
			} else {
				std::cout << "Hit non-enemy with ID: " << hitBodyID.GetIndexAndSequenceNumber() << std::endl;
			}

			JPH::Vec3 pt = ray.GetPointOnRay(result.mFraction);
			std::cout
				<< "Hit at ("
				<< pt.GetX() << ", "
				<< pt.GetY() << ", "
				<< pt.GetZ() << ")\n";
		} else {
			std::cout << "No hit\n";
		}
	}

	void PhysicsPlayer::handleRotation(float deltaYaw, float deltaPitch) {
		camera.addRotation(deltaYaw, deltaPitch);
	}

	float PhysicsPlayer::getMaxHealth() const {
		return settings.maxHealth;
	}

	float PhysicsPlayer::getCurrentHealth() const {
		return currentHealth;
	}

	// @returns true if dead
	void PhysicsPlayer::takeDamage(float damage) {
		currentHealth -= damage;

		if (currentHealth <= 0)
			settings.deathCallback();
	}

	bool PhysicsPlayer::isDead() const {
		return currentHealth <= 0;
	}

	void PhysicsPlayer::postSimulation() {
		character->PostSimulation(settings.maxFloorSeparationDistance);
		camera.setPhysicsPosition(character->GetPosition());

		float playerY = character->GetPosition().GetY();
		float deathHeight = -10.0f;

		if (playerY < deathHeight && !isDead()) {
			settings.deathCallback();
		}
	}

	const glm::vec3 PhysicsPlayer::getCameraPosition() const {
		return camera.getPosition();
	}

	void PhysicsPlayer::printInfo(int iterationStep) const {
		// Output current position and velocity of the player

		JPH::RVec3 position = character->GetPosition();
		JPH::Vec3 velocity = character->GetLinearVelocity();
		std::cout << "PhysicsPlayer [" << id << "] : Step " << iterationStep << " : Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << "), health = " << currentHealth << "/" << settings.maxHealth << std::endl;
	}

	JPH::BodyID PhysicsPlayer::getBodyID() const {
		return this->character->GetBodyID();
	}

	glm::mat4 PhysicsPlayer::computeModelMatrix() const {
		return glm::mat4(1.0);
	}

	glm::mat4 PhysicsPlayer::computeNormalMatrix() const {
		return glm::mat4(1.0);
	}

	glm::vec3 PhysicsPlayer::getPosition() const {
		return RVec3ToGLM(this->character->GetPosition());
	}

	PhysicsPlayer::PlayerCreationSettings PhysicsPlayer::getCreationSettings() const {
		PlayerCreationSettings creationSettings;
		creationSettings.position = character->GetPosition();
		creationSettings.playerSettings = settings;
		creationSettings.cameraSettings = camera.getSettings();
		creationSettings.characterSettings = characterSettings;

		return creationSettings;
	}
}