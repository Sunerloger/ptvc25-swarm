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
		// Simple growth animation
		growthTime += deltaTime;
		float currentGrowth = std::min(growthTime / maxGrowth, 1.0f);

		// Apply growth scaling
		scale = scale * currentGrowth;
	}

	std::unique_ptr<VegetationObject> VegetationObject::createFern(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createFern(seed);

		// Create larger, more detailed fern with 3-4 iterations for good complexity
		std::string lsystemString = lsystem.generate(3);

		// Get the default turtle parameters
		TurtleParameters params = lsystem.getTurtleParameters();

		// Customize the parameters for larger, more realistic ferns
		params.stepLength = 0.3f;							// Much larger segments for bigger ferns
		params.angleIncrement = 30.0f;						// Good angle for natural frond spread
		params.radiusDecay = 0.7f;							// Gradual taper to maintain main stem
		params.lengthDecay = 0.87f;							// Slow decay for larger overall structure
		params.initialRadius = 0.12f;						// Much thicker main stem
		params.initialColor = glm::vec3(0.3f, 0.2f, 0.1f);	// Brown stem color
		params.leafColor = glm::vec3(0.15f, 0.8f, 0.2f);	// Vibrant green for fronds

		// Generate geometry with customized parameters, starting from origin
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, params, glm::vec3(0.0f, 0.0f, 0.0f), seed);

		// Add explicit main trunk from ground to tree base to ensure connection
		addMainTrunk(geometry, params);

		geometry.type = VegetationType::Fern;

		// Create the vegetation object
		auto fernObject = std::make_unique<VegetationObject>(device, geometry, position, scale);

		// Note: Material will be applied by the calling code using VegetationSharedResources
		// This avoids creating individual materials for each fern, reducing descriptor pool usage

		return fernObject;
	}

	std::unique_ptr<VegetationObject> VegetationObject::createFern(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed,
		int iterations,
		const std::string& axiom,
		const TurtleParameters& turtleParams) {
		LSystem lsystem = LSystem::createFern(seed);

		// Override axiom if provided
		if (!axiom.empty()) {
			lsystem.setAxiom(axiom);
		}

		// Generate L-system string with custom iterations
		std::string lsystemString = lsystem.generate(iterations);

		// Generate the tree structure starting from origin
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, turtleParams, glm::vec3(0.0f, 0.0f, 0.0f), seed);

		// Add explicit main trunk from ground to tree base to ensure connection
		addMainTrunk(geometry, turtleParams);

		geometry.type = VegetationType::Fern;

		// Create the vegetation object
		auto fernObject = std::make_unique<VegetationObject>(device, geometry, position, scale);

		// Note: Material will be applied by the calling code using VegetationSharedResources
		// This avoids creating individual materials for each fern, reducing descriptor pool usage

		return fernObject;
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

		// Note: Material will be applied by the calling code using VegetationSharedResources
		// This avoids creating individual materials for each vegetation object

		return model;
	}

	// GameObject interface implementations
	glm::mat4 VegetationObject::computeModelMatrix() const {
		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 R = glm::mat4(1.0f);	// Could add rotation here if needed
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
		// Ensure the tree is properly grounded by creating a trunk from Y=0 upward
		// The L-System generation should start from ground level
		
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
		
		// Always add a small root/trunk segment at ground level for visual connection
		// This ensures the tree appears properly planted
		glm::vec3 trunkStart(0.0f, 0.0f, 0.0f);       // Ground level
		glm::vec3 trunkEnd(0.0f, 0.15f, 0.0f);        // Short trunk segment upward
		
		float trunkRadius = params.initialRadius * 1.1f;  // Slightly thicker base
		generateTrunkCylinder(trunkStart, trunkEnd, trunkRadius, params.initialRadius, params.initialColor, geometry);
	}

	void VegetationObject::generateTrunkCylinder(const glm::vec3& start, const glm::vec3& end,
		float radiusStart, float radiusEnd, const glm::vec3& color, LSystemGeometry& geometry) {
		int segments = 8;

		glm::vec3 direction = glm::normalize(end - start);
		glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(1, 0, 0)));
		if (glm::length(right) < 0.1f) {
			right = glm::normalize(glm::cross(direction, glm::vec3(0, 0, 1)));
		}
		glm::vec3 up = glm::normalize(glm::cross(right, direction));

		uint32_t startVertexIndex = geometry.vertices.size();

		// Generate vertices for cylinder
		for (int i = 0; i <= segments; ++i) {
			float angle = 2.0f * M_PI * i / segments;
			float cosAngle = std::cos(angle);
			float sinAngle = std::sin(angle);

			// Start circle
			glm::vec3 offsetStart = (right * cosAngle + up * sinAngle) * radiusStart;
			glm::vec3 posStart = start + offsetStart;
			glm::vec3 normalStart = glm::normalize(offsetStart);

			// End circle
			glm::vec3 offsetEnd = (right * cosAngle + up * sinAngle) * radiusEnd;
			glm::vec3 posEnd = end + offsetEnd;
			glm::vec3 normalEnd = glm::normalize(offsetEnd);

			// Add vertices
			geometry.vertices.push_back({posStart, color, normalStart, glm::vec2(float(i) / segments, 0.0f)});
			geometry.vertices.push_back({posEnd, color, normalEnd, glm::vec2(float(i) / segments, 1.0f)});
		}

		// Generate indices for cylinder sides
		for (int i = 0; i < segments; ++i) {
			uint32_t curr = startVertexIndex + i * 2;
			uint32_t next = startVertexIndex + ((i + 1) % (segments + 1)) * 2;

			// First triangle
			geometry.indices.push_back(curr);
			geometry.indices.push_back(curr + 1);
			geometry.indices.push_back(next);

			// Second triangle
			geometry.indices.push_back(next);
			geometry.indices.push_back(curr + 1);
			geometry.indices.push_back(next + 1);
		}
	}

	std::shared_ptr<vk::Model> VegetationObject::getModel() const {
		return model;
	}

}  // namespace procedural
