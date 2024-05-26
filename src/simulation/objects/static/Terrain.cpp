#include "Terrain.h"

namespace physics {

	Terrain::Terrain(PhysicsSystem& physics_system, glm::vec3 color, Model* model, glm::vec3 position) : ManagedPhysicsEntity(physics_system), model(model), position(position) {

		this->color = color;
		this->scale = glm::vec3{ 100.0, 1.0, 100.0 };

		// We can create a rigid body to serve as the floor, we make a large box
		// Create the settings for the collision volume (the shape).
		// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		BoxShapeSettings floor_shape_settings(RVec3(this->scale[0], this->scale[1], this->scale[2]));

		// Create the shape
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(floor_shape, RVec3(this->position[0], this->position[1], this->position[2]), 
			Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING));

		// create physics body
		this->bodyID = physics_system.GetBodyInterface().CreateBody(*body_settings)->GetID();
	}

	Terrain::~Terrain() {}

	void Terrain::addPhysicsBody() {

		// edits body if it was already added
		// bodyID is never a nullptr here
		physics_system.GetBodyInterface().AddBody(this->bodyID, EActivation::DontActivate);
	}


}