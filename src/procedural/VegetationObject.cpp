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

	VegetationObject::VegetationObject(
		vk::Device& device,
		const TreeGeometry& treeGeometry,
		const TreeMaterial& treeMaterial,
		const glm::vec3& position,
		const glm::vec3& scale) : multipleMaterials(true), position(position), scale(scale) {
		// Create separate models for bark and leaf geometry
		auto models = createModelsFromTreeGeometry(device, treeGeometry);
		barkModel = models.first;
		leafModel = models.second;

		// Apply materials to the models
		if (barkModel) {
			barkModel->setMaterial(treeMaterial.getBarkMaterial());
		}
		if (leafModel) {
			leafModel->setMaterial(treeMaterial.getLeafMaterial());
		}

		// Set main model to bark for compatibility
		model = barkModel;
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

		return treeObject;
	}

	std::unique_ptr<VegetationObject> VegetationObject::createEnhancedTree(
		vk::Device& device,
		const TreeMaterial& treeMaterial,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createTree(seed);

		std::string lsystemString = lsystem.generate(3);

		// Get the default turtle parameters
		TurtleParameters params = lsystem.getTurtleParameters();

		// Generate tree geometry with separate bark and leaf materials
		TreeGeometry treeGeometry = lsystem.interpretToTreeGeometry(lsystemString, params, glm::vec3(0.0f, 0.0f, 0.0f), seed);

		// Create the enhanced vegetation object
		auto treeObject = std::make_unique<VegetationObject>(device, treeGeometry, treeMaterial, position, scale);

		return treeObject;
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

		if (!builder.vertices.empty()) {
			auto mn = builder.vertices[0].position;
			auto mx = mn;
			for (size_t i = 1; i < builder.vertices.size(); ++i) {
				mn = glm::min(mn, builder.vertices[i].position);
				mx = glm::max(mx, builder.vertices[i].position);
			}
			builder.boundsMin = mn;
			builder.boundsMax = mx;
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

	std::shared_ptr<vk::Model> VegetationObject::getBarkModel() const {
		return barkModel;
	}

	std::shared_ptr<vk::Model> VegetationObject::getLeafModel() const {
		return leafModel;
	}

	bool VegetationObject::hasMultipleMaterials() const {
		return multipleMaterials;
	}

	glm::vec3 VegetationObject::getScale() const {
		return scale;
	}

	std::pair<std::shared_ptr<vk::Model>, std::shared_ptr<vk::Model>>
	VegetationObject::createModelsFromTreeGeometry(
		vk::Device& device,
		const TreeGeometry& treeGeometry) {
		// Create bark model
		std::shared_ptr<vk::Model> barkModel;
		if (!treeGeometry.bark.vertices.empty()) {
			vk::Model::Builder barkBuilder{};

			// Add bark vertices
			for (const auto& vertex : treeGeometry.bark.vertices) {
				vk::Model::Vertex modelVertex{};
				modelVertex.position = vertex.position;
				modelVertex.color = vertex.color;
				modelVertex.normal = vertex.normal;
				modelVertex.uv = vertex.uv;
				barkBuilder.vertices.push_back(modelVertex);
			}

			if (!barkBuilder.vertices.empty()) {
				auto mn = barkBuilder.vertices[0].position;
				auto mx = mn;
				for (size_t i = 1; i < barkBuilder.vertices.size(); ++i) {
					mn = glm::min(mn, barkBuilder.vertices[i].position);
					mx = glm::max(mx, barkBuilder.vertices[i].position);
				}
				barkBuilder.boundsMin = mn;
				barkBuilder.boundsMax = mx;
			}

			// Add bark indices
			for (const auto& index : treeGeometry.bark.indices) {
				barkBuilder.indices.push_back(index);
			}

			barkModel = std::make_shared<vk::Model>(device, barkBuilder);
		}

		// Create leaf model
		std::shared_ptr<vk::Model> leafModel;
		if (!treeGeometry.leaves.vertices.empty()) {
			vk::Model::Builder leafBuilder{};

			// Add leaf vertices
			for (const auto& vertex : treeGeometry.leaves.vertices) {
				vk::Model::Vertex modelVertex{};
				modelVertex.position = vertex.position;
				modelVertex.color = vertex.color;
				modelVertex.normal = vertex.normal;
				modelVertex.uv = vertex.uv;
				leafBuilder.vertices.push_back(modelVertex);
			}

			if (!leafBuilder.vertices.empty()) {
				auto mn = leafBuilder.vertices[0].position;
				auto mx = mn;
				for (size_t i = 1; i < leafBuilder.vertices.size(); ++i) {
					mn = glm::min(mn, leafBuilder.vertices[i].position);
					mx = glm::max(mx, leafBuilder.vertices[i].position);
				}
				leafBuilder.boundsMin = mn;
				leafBuilder.boundsMax = mx;
			}

			// Add leaf indices
			for (const auto& index : treeGeometry.leaves.indices) {
				leafBuilder.indices.push_back(index);
			}

			leafModel = std::make_shared<vk::Model>(device, leafBuilder);
		}

		return std::make_pair(barkModel, leafModel);
	}

}  // namespace procedural
