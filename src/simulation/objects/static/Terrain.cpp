#include "Terrain.h"
#include <iostream>
#include <numeric>

namespace physics {

	Terrain::Terrain(std::shared_ptr<PhysicsSystem> physics_system, glm::vec3 color, std::shared_ptr<vk::Model> model, glm::vec3 position, glm::vec3 scale)
		: ManagedPhysicsEntity(physics_system), model(model), useHeightfield(false) {
		std::cout << "Creating box-based terrain (flat collision)" << std::endl;
		this->color = color;
		this->scale = scale;

		// model is 2x2, but box takes in halfEdgeLength
		glm::vec3 physicsScale = scale * glm::vec3{1.0, 0.5, 1.0};

		// We can create a rigid body to serve as the floor, we make a large box
		// Create the settings for the collision volume (the shape).
		// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		BoxShapeSettings floor_shape_settings(GLMToRVec3(physicsScale));

		// Create the shape
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get();  // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(floor_shape, GLMToRVec3(position),
			Quat::sIdentity(), EMotionType::Static, physics::Layers::NON_MOVING));

		// create physics body
		this->bodyID = physics_system->GetBodyInterface().CreateBody(*body_settings)->GetID();
	}
	
	Terrain::Terrain(std::shared_ptr<PhysicsSystem> physics_system, glm::vec3 color, std::shared_ptr<vk::Model> model,
	                 glm::vec3 position, glm::vec3 scale, std::vector<float> heightfieldData)
		: ManagedPhysicsEntity(physics_system), model(model), useHeightfield(true) {
		std::cout << "Creating heightfield-based terrain with provided height data (3D collision)" << std::endl;
		this->color = color;
		this->scale = scale;
		
		// pointer swap
		heightfieldSamples = std::move(heightfieldData);
		
		// create a temporary array with the correct size
		float* heightData = this->heightfieldSamples.data();

		// calculate the number of samples in each dimension
		int numSamplesPerSide = static_cast<int>(sqrt(this->heightfieldSamples.size()));
		std::cout << "Creating heightfield with " << numSamplesPerSide << "x" << numSamplesPerSide << " samples" << std::endl;
		
		// Copy our height samples directly into the array
		for (int i = 0; i < numSamplesPerSide * numSamplesPerSide; i++) {
			if (i < heightfieldSamples.size()) {
				heightData[i] = heightfieldSamples[i];
			} else {
				heightData[i] = 0.0f;  // Default height if we run out of samples
			}
		}
		
		// Print some debug info
		std::cout << "Height samples min/max: ";
		if (!heightfieldSamples.empty()) {
			float min = heightfieldSamples[0];
			float max = heightfieldSamples[0];
			for (const auto& h : heightfieldSamples) {
				min = std::min(min, h);
				max = std::max(max, h);
			}
			std::cout << min << " / " << max << std::endl;
		} else {
			std::cout << "No samples!" << std::endl;
		}
		
		float width = scale.x * 2.0f;								// mesh spans [-scale.x, +scale.x]
		float depth = scale.z * 2.0f;								// mesh spans [-scale.z, +scale.z]
		float cellSizeX = width / (numSamplesPerSide - 1);			// divide by sample - 1 cells to get size of single cell
		float cellSizeZ = depth / (numSamplesPerSide - 1);
		RVec3 shapeOffset = RVec3(-scale.x, 0.0f, -scale.z);		// samples go from -scale.x -> +scale.x
		
		HeightFieldShapeSettings heightfield_settings(
			heightData,
			shapeOffset,
			Vec3(cellSizeX,	scale.y, cellSizeZ),
			numSamplesPerSide);
			
		// Create the shape
		ShapeSettings::ShapeResult heightfield_result = heightfield_settings.Create();
		
		// Create the body settings
		if (heightfield_result.HasError()) {
			std::cout << "Error creating heightfield shape: " << heightfield_result.GetError() << std::endl;
			// Fallback to a box shape
			BoxShapeSettings fallback_settings(GLMToRVec3(scale * glm::vec3{1.0, 0.5, 1.0}));
			ShapeSettings::ShapeResult fallback_result = fallback_settings.Create();
			body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(
				fallback_result.Get(),
				GLMToRVec3(position),
				Quat::sIdentity(),
				EMotionType::Static,
				physics::Layers::NON_MOVING));
		} else {
			std::cout << "Successfully created heightfield shape" << std::endl;
			// Use the heightfield shape
			body_settings = std::make_unique<BodyCreationSettings>(BodyCreationSettings(
				heightfield_result.Get(),
				GLMToRVec3(position),
				Quat::sIdentity(),
				EMotionType::Static,
				physics::Layers::NON_MOVING));
		}
		
		// Create physics body
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