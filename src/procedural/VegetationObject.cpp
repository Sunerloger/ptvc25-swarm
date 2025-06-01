#include "VegetationObject.h"
#include "../rendering/materials/StandardMaterial.h"
#include <algorithm>

namespace procedural {

	VegetationObject::VegetationObject(
		vk::Device& device,
		const LSystemGeometry& geometry,
		const glm::vec3& position,
		const glm::vec3& scale) : position(position), scale(scale) {
		// Create model from L-System geometry
		model = createModelFromGeometry(device, geometry);
	}

	std::unique_ptr<VegetationObject> VegetationObject::createTree(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createTree(seed);

		std::string lsystemString = lsystem.generate(3);

		// Get the default turtle parameters
		TurtleParameters params = lsystem.getTurtleParameters();

		// Generate geometry with customized parameters, starting from origin
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, params, glm::vec3(0.0f, 0.0f, 0.0f), seed);

		// Create the vegetation object
		auto treeObject = std::make_unique<VegetationObject>(device, geometry, position, scale);

		return treeObject;
	}

	std::unique_ptr<VegetationObject> VegetationObject::createTree(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed,
		int iterations,
		const std::string& axiom,
		const TurtleParameters& turtleParams) {
		LSystem lsystem = LSystem::createTree(seed);

		if (!axiom.empty()) {
			lsystem.setAxiom(axiom);
		}

		std::string lsystemString = lsystem.generate(iterations);

		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, turtleParams, glm::vec3(0.0f, 0.0f, 0.0f), seed);

		// Create the vegetation object
		auto treeObject = std::make_unique<VegetationObject>(device, geometry, position, scale);

		return treeObject;	// Renamed from fernObject
	}

	std::shared_ptr<vk::Model> VegetationObject::createModelFromGeometry(
		vk::Device& device,
		const LSystemGeometry& geometry) {
		vk::Model::Builder builder{};

		// Add vertices
		for (const auto& vertex : geometry.vertices) {
			vk::Model::Vertex modelVertex{};
			modelVertex.position = vertex.position;
			modelVertex.color = vertex.color;
			modelVertex.normal = vertex.normal;
			modelVertex.uv = vertex.uv;
			builder.vertices.push_back(modelVertex);
		}

		// Add indices
		for (const auto& index : geometry.indices) {
			builder.indices.push_back(index);
		}

		// Create the model
		auto model = std::make_shared<vk::Model>(device, builder);

		return model;
	}

	glm::mat4 VegetationObject::computeModelMatrix() const {
		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 R = glm::mat4(1.0f);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
		return T * R * S;
	}

	glm::mat4 VegetationObject::computeNormalMatrix() const {
		return glm::transpose(glm::inverse(computeModelMatrix()));
	}

	glm::vec3 VegetationObject::getPosition() const {
		return position;
	}

	std::shared_ptr<vk::Model> VegetationObject::getModel() const {
		return model;
	}

}  // namespace procedural
