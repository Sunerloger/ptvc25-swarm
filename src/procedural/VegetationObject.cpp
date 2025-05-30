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

	std::unique_ptr<VegetationObject> VegetationObject::createTree(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createSimpleTree(seed);
		std::string lsystemString = lsystem.generate(3);  // 3 iterations for trees
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, lsystem.getTurtleParameters(), glm::vec3(0.0f), seed);
		geometry.type = VegetationType::Tree;
		return std::make_unique<VegetationObject>(device, geometry, position, scale);
	}

	std::unique_ptr<VegetationObject> VegetationObject::createBush(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createBush(seed);
		std::string lsystemString = lsystem.generate(2);  // 2 iterations for bushes
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, lsystem.getTurtleParameters(), glm::vec3(0.0f), seed);
		geometry.type = VegetationType::Bush;
		return std::make_unique<VegetationObject>(device, geometry, position, scale);
	}

	std::unique_ptr<VegetationObject> VegetationObject::createGrass(
		vk::Device& device,
		const glm::vec3& position,
		const glm::vec3& scale,
		int seed) {
		LSystem lsystem = LSystem::createGrass(seed);
		std::string lsystemString = lsystem.generate(1);  // 1 iteration for grass
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, lsystem.getTurtleParameters(), glm::vec3(0.0f), seed);
		geometry.type = VegetationType::Grass;
		return std::make_unique<VegetationObject>(device, geometry, position, scale);
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

		// Generate geometry with customized parameters, starting at ground level
		LSystemGeometry geometry = lsystem.interpretToGeometry(lsystemString, params, glm::vec3(0.0f, 0.0f, 0.0f), seed);
		geometry.type = VegetationType::Fern;

		// Create the vegetation object
		auto fernObject = std::make_unique<VegetationObject>(device, geometry, position, scale);

		// Create a proper material for the fern with backface culling disabled
		std::vector<unsigned char> fernGreen = {34, 139, 34, 255};	// Forest green
		auto material = std::make_shared<vk::StandardMaterial>(device, fernGreen, 1, 1, 4);

		// Disable backface culling by configuring the material
		material->getPipelineConfig().rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

		// Assign the material to the model
		fernObject->getModel()->setMaterial(material);

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

		// Create a simple vegetation material based on vegetation type
		std::shared_ptr<vk::Material> material;

		switch (geometry.type) {
			case VegetationType::Tree:
				// Create a green material for trees with a simple 1x1 green texture
				{
					std::vector<unsigned char> greenPixel = {34, 139, 34, 255};	 // Forest green
					material = std::make_shared<vk::StandardMaterial>(device, greenPixel, 1, 1, 4);
				}
				break;
			case VegetationType::Bush:
				// Create a darker green material for bushes
				{
					std::vector<unsigned char> darkGreenPixel = {0, 100, 0, 255};  // Dark green
					material = std::make_shared<vk::StandardMaterial>(device, darkGreenPixel, 1, 1, 4);
				}
				break;
			case VegetationType::Grass:
				// Create a bright green material for grass
				{
					std::vector<unsigned char> brightGreenPixel = {124, 252, 0, 255};  // Lawn green
					material = std::make_shared<vk::StandardMaterial>(device, brightGreenPixel, 1, 1, 4);
				}
				break;
			case VegetationType::Fern:
				// Create a medium green material for ferns
				{
					std::vector<unsigned char> fernGreenPixel = {46, 125, 50, 255};	 // Fern green
					material = std::make_shared<vk::StandardMaterial>(device, fernGreenPixel, 1, 1, 4);
				}
				break;
			default:
				// Fallback to a default green
				{
					std::vector<unsigned char> defaultGreenPixel = {0, 128, 0, 255};  // Default green
					material = std::make_shared<vk::StandardMaterial>(device, defaultGreenPixel, 1, 1, 4);
				}
				break;
		}

		model->setMaterial(material);
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

	std::shared_ptr<vk::Model> VegetationObject::getModel() const {
		return model;
	}

}  // namespace procedural
