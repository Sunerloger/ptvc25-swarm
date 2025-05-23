#include "CollisionHandler.h"

#include "../scene/SceneManager.h"

// STL includes
#include <iostream>

namespace physics {

	MyContactListener::MyContactListener() {}

	MyContactListener::~MyContactListener() {}

	JPH::ValidateResult MyContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) {
		JPH::BodyID bodyID1 = inBody1.GetID();
		JPH::BodyID bodyID2 = inBody2.GetID();

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id1 = sceneManager.getIdFromBodyID(bodyID1);
		vk::id_t id2 = sceneManager.getIdFromBodyID(bodyID2);

		// Debug: std::cout << "Contact validate callback [" << id1 << ", " << id2 << "]" << std::endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		// return JPH::ValidateResult::RejectContact;
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
		JPH::BodyID bodyID1 = inBody1.GetID();
		JPH::BodyID bodyID2 = inBody2.GetID();

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id1 = sceneManager.getIdFromBodyID(bodyID1);
		vk::id_t id2 = sceneManager.getIdFromBodyID(bodyID2);

		// Debug: std::cout << "A contact was added [" << id1 << ", " << id2 << "]" << std::endl;

		float impactSpeed = 0.0f;

		// if we have a contact point, we can get the relative velocity at the impact point
		if (inManifold.mRelativeContactPointsOn1.size() > 0) {
			const JPH::Vec3& contactPoint = inManifold.GetWorldSpaceContactPointOn1(0);

			JPH::Vec3 relativeVelocity = inBody2.GetPointVelocity(contactPoint) -
										 inBody1.GetPointVelocity(contactPoint);

			// project the relative velocity onto the contact normal to get impact speed
			impactSpeed = relativeVelocity.Dot(inManifold.mWorldSpaceNormal);
		}

		JPH::Vec3 normal = inManifold.mWorldSpaceNormal;
		// float panetrationDepth = inManifold.mPenetrationDepth;

		auto gameObj1 = sceneManager.getObject(id1);
		auto gameObj2 = sceneManager.getObject(id2);

		if (gameObj1 && gameObj2) {
			if (gameObj1->first == SceneClass::PLAYER && gameObj2->first == SceneClass::ENEMY) {
				handlePlayerEnemyCollision(gameObj1->second.lock(), gameObj2->second.lock(), impactSpeed, normal);
			} else if (gameObj1->first == SceneClass::ENEMY && gameObj2->first == SceneClass::PLAYER) {
				handlePlayerEnemyCollision(gameObj2->second.lock(), gameObj1->second.lock(), impactSpeed, -normal);
			}
		}
	}

	void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
		JPH::BodyID bodyID1 = inBody1.GetID();
		JPH::BodyID bodyID2 = inBody2.GetID();

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id1 = sceneManager.getIdFromBodyID(bodyID1);
		vk::id_t id2 = sceneManager.getIdFromBodyID(bodyID2);

		// Debug: std::cout << "A contact was persisted [" << id1 << ", " << id2 << "]" << std::endl;
	}

	void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
		JPH::BodyID bodyID1 = inSubShapePair.GetBody1ID();
		JPH::BodyID bodyID2 = inSubShapePair.GetBody2ID();

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id1 = sceneManager.getIdFromBodyID(bodyID1);
		vk::id_t id2 = sceneManager.getIdFromBodyID(bodyID2);

		// Debug: std::cout << "A contact was removed [" << id1 << ", " << id2 << "]" << std::endl;
	}

	void MyContactListener::handlePlayerEnemyCollision(std::shared_ptr<vk::GameObject> player, std::shared_ptr<vk::GameObject> enemy, float impactSpeed, const JPH::Vec3& normal) {
		std::shared_ptr<Player> playerObj = std::static_pointer_cast<physics::Player>(player);
		std::shared_ptr<Enemy> enemyObj = std::static_pointer_cast<physics::Enemy>(enemy);

		// the following lines are only for testing purposes
		float health = enemyObj->getCurrentHealth();
		bool isDead = enemyObj->takeDamage(health);

		if (isDead) {
			// Debug: std::cout << "Enemy [" << enemy->getId() << "] died" << std::endl;
		}

		// TODO deal damage to player, maybe enable head jumps (normal points from player to enemy, so if it is approximately down), maybe use impact speed
		// TODO maybe store time of last damage with player and add a invulnerability period after hit
	}

	MyBodyActivationListener::MyBodyActivationListener() {}

	MyBodyActivationListener::~MyBodyActivationListener() {}

	void MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id = sceneManager.getIdFromBodyID(inBodyID);

		// Debug: std::cout << "A body got activated [" << id << "]" << std::endl;
	}

	void MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {

		SceneManager& sceneManager = SceneManager::getInstance();

		vk::id_t id = sceneManager.getIdFromBodyID(inBodyID);

		// if object already destroyed in scene manager but for some reason appears here (e.g. while closing window)
		if (id == vk::INVALID_OBJECT_ID) {
			return;
		}

		// Debug: std::cout << "A body got deactivated [" << id << "]" << std::endl;
	}

}