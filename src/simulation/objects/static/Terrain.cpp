#include "Terrain.h"

namespace physics {

	Terrain::Terrain(std::shared_ptr<PhysicsSystem> physics_system, glm::vec3 color, std::shared_ptr<vk::Model> model, glm::vec3 position, glm::vec3 scale) : ManagedPhysicsEntity(physics_system), model(model) {

		this->color = color;
		this->scale = scale;

		// model is 2x2, but box takes in halfEdgeLength
		glm::vec3 physicsScale = scale * glm::vec3{ 1.0, 0.5, 1.0 };

		// We can create a rigid body to serve as the floor, we make a large box
		// Create the settings for the collision volume (the shape).
		// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		BoxShapeSettings floor_shape_settings(GLMToRVec3(physicsScale));

		// Create the shape
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(floor_shape, GLMToRVec3(position),
			Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING));

		// create physics body
		this->bodyID = physics_system->GetBodyInterface().CreateBody(*body_settings)->GetID();
	}

	Terrain::~Terrain() {}

	void Terrain::addPhysicsBody() {

		// edits body if it was already added
		// bodyID is never a nullptr here
		physics_system->GetBodyInterface().AddBody(this->bodyID, EActivation::DontActivate);
	}

	glm::mat4 Terrain::computeModelMatrix() const {
		BodyInterface& body_interface = this->physics_system->GetBodyInterface();
		RMat44 physicsWorldTransform = body_interface.GetWorldTransform(this->bodyID);
		physicsWorldTransform = physicsWorldTransform.PreScaled(GLMToRVec3(this->scale));
		return RMat44ToGLM(physicsWorldTransform);
	}

	glm::mat4 Terrain::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(this->computeModelMatrix()));
	}

	glm::vec3 Terrain::getPosition() const {
		BodyInterface& body_interface = this->physics_system->GetBodyInterface();
		RVec3 physics_position = body_interface.GetPosition(this->bodyID);
		return RVec3ToGLM(physics_position);
	}

	std::shared_ptr<vk::Model> Terrain::getModel() const {
		return this->model;
	}


}