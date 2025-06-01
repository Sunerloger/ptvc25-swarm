#include "VegetationObject.h"
#include "../rendering/materials/StandardMaterial.h"
#include <algorithm>

namespace procedural {

	VegetationObject::VegetationObject(
		vk::Device& device,
		const LSystemGeometry& geometry,
		const glm::vec3& position,
		const glm::vec3& scale,
		const glm::vec3& rotation) : position(position), scale(scale), rotation(rotation) {
		vegetationType = geometry.type;

		// Create model from L-System geometry
		model = createModelFromGeometry(device, geometry);
	}

	void VegetationObject::update(float deltaTime) {
		growthTime += deltaTime;
		float currentGrowth = std::min(growthTime / maxGrowth, 1.0f);

		// Apply growth scaling
		scale = scale * currentGrowth;
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

		geometry.type = VegetationType::Tree;

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

		geometry.type = VegetationType::Tree;

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

	void VegetationObject::addMainTrunk(LSystemGeometry& geometry, const TurtleParameters& params) {
		// Find if there are any vertices below ground level
		float minY = 0.0f;
		bool hasVerticesAboveGround = false;

		for (const auto& vertex : geometry.vertices) {
			minY = std::min(minY, vertex.position.y);
			if (vertex.position.y > 0.01f) {
				hasVerticesAboveGround = true;
			}
		}

		// If tree has vertices below ground or doesn't start properly at ground,
		// shift everything up so the base is at Y=0
		if (minY < -0.01f) {
			float offset = -minY;  // Shift everything up
			for (auto& vertex : geometry.vertices) {
				vertex.position.y += offset;
			}
		}

		glm::vec3 trunkStart(0.0f, 0.0f, 0.0f);	 // Ground level
		glm::vec3 trunkEnd(0.0f, 0.05f, 0.0f);	 // Very short trunk segment upward, L-system handles the rest

		float trunkRadius = params.initialRadius * 1.1f;  // Slightly thicker base
		generateTrunkCylinder(trunkStart, trunkEnd, trunkRadius, params.initialRadius, params.initialColor, geometry);
	}

	void VegetationObject::generateTrunkCylinder(const glm::vec3& start, const glm::vec3& end,
		float radiusStart, float radiusEnd, const glm::vec3& color, LSystemGeometry& geometry) {
		int segments = 8;

		float minRadius = 0.01f;
		float actualRadiusStart = std::max(radiusStart, minRadius);
		float actualRadiusEnd = std::max(radiusEnd, minRadius);

		if (glm::distance(start, end) < 0.01f) {
			return;	 // Too short to be visible
		}

		glm::vec3 direction = glm::normalize(end - start);
		glm::vec3 cylRight = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
		if (glm::length(cylRight) < 0.1f) {										   // if direction is (close to) Y-axis
			cylRight = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));  // use X-axis for cross product
		}
		glm::vec3 cylUp = glm::normalize(glm::cross(cylRight, direction));

		uint32_t baseVertexIndex = static_cast<uint32_t>(geometry.vertices.size());

		// Generate vertices for cylinder
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle vertex
			glm::vec3 offsetStart = (cylRight * cosAngle + cylUp * sinAngle) * actualRadiusStart;
			geometry.vertices.push_back({start + offsetStart,
				color,	// Use the passed color directly
				glm::normalize(offsetStart),
				glm::vec2(static_cast<float>(i) / segments, 0.0f)});

			// End circle vertex
			glm::vec3 offsetEnd = (cylRight * cosAngle + cylUp * sinAngle) * actualRadiusEnd;
			geometry.vertices.push_back({end + offsetEnd,
				color,	// Use the passed color directly
				glm::normalize(offsetEnd),
				glm::vec2(static_cast<float>(i) / segments, 1.0f)});
		}

		// Generate indices for cylinder sides
		for (int i = 0; i < segments; ++i) {
			uint32_t bottomLeft = baseVertexIndex + i * 2;
			uint32_t bottomRight = baseVertexIndex + (i + 1) * 2;
			uint32_t topLeft = bottomLeft + 1;
			uint32_t topRight = bottomRight + 1;

			// Triangle 1
			geometry.indices.push_back(bottomLeft);
			geometry.indices.push_back(topLeft);
			geometry.indices.push_back(bottomRight);

			// Triangle 2
			geometry.indices.push_back(bottomRight);
			geometry.indices.push_back(topLeft);
			geometry.indices.push_back(topRight);
		}
	}

	std::shared_ptr<vk::Model> VegetationObject::getModel() const {
		return model;
	}

}  // namespace procedural
