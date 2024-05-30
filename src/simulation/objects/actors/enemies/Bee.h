#pragma once

#include "Enemy.h"

namespace physics {
	class Bee : public Enemy {
		
		Bee(float maxHealth, vk::Model* model);
		virtual ~Bee();

		JPH::BodyID getBodyID() override;

		void addPhysicsBody() override;
		void removePhysicsBody() override;

		glm::mat4 computeModelMatrix() const override;

		virtual glm::mat4 computeNormalMatrix() const override;

		virtual glm::vec3 getPosition() const override;

		virtual vk::Model* getModel() const override;
	};
}