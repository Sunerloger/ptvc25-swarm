#include "Terrain.h"

namespace physics {

	static const float cCollisionTolerance = 0.05f;

	Terrain::Terrain(PhysicsSystem& physics_system) : ManagedPhysicsEntity(physics_system) {

		// We can create a rigid body to serve as the floor, we make a large box
		// Create the settings for the collision volume (the shape).
		// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));

		// Create the shape
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(floor_shape, RVec3(0.0_r, -2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING));
	}

	Terrain::~Terrain() {}

	void Terrain::addPhysicsBody() {
		physics_system.GetBodyInterface().CreateAndAddBody(*body_settings, EActivation::DontActivate);
	}
}