#include "VegetationIntegrator.h"
#include "SimpleGameObject.h"
#include <iostream>
#include <cmath>

namespace procedural {

	VegetationIntegrator::VegetationIntegrator(vk::Device& device)
		: device(device) {
	}

	void VegetationIntegrator::generateVegetationOnTerrain(
		const VegetationSettings& settings,
		const std::vector<float>& heightfieldData,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition) {
		// Clear existing vegetation
		clearVegetation();

		std::mt19937 rng(settings.placementSeed);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		// Calculate terrain area
		float terrainWidth = settings.terrainMax.x - settings.terrainMin.x;
		float terrainDepth = settings.terrainMax.y - settings.terrainMin.y;
		float terrainArea = terrainWidth * terrainDepth;

		int numTrees = static_cast<int>(terrainArea * settings.treeDensity);

		std::cout << "Generating vegetation: " << numTrees << " trees" << std::endl;

		// Create shared resources to reuse materials
		auto resources = std::make_shared<VegetationSharedResources>(device);

		// Generate trees
		for (int i = 0; i < numTrees; ++i) {
			glm::vec2 pos2D(
				settings.terrainMin.x + dist(rng) * terrainWidth,
				settings.terrainMin.y + dist(rng) * terrainDepth);

			float height = sampleHeightAt(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);
			float slope = calculateSlope(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);

			if (isSuitableForVegetation(pos2D, height, slope, settings)) {
				// Ensure tree is properly grounded - place it slightly below terrain surface
				glm::vec3 position(pos2D.x, height - 0.1f, pos2D.y);
				float scale = getRandomScale(settings.treeScaleRange, rng);

				// Generate unique seed for each tree for variety
				int treeSeed = rng();

				auto tree = VegetationObject::createTree(device, position, glm::vec3(scale), treeSeed);

				// Apply shared material
				tree->getModel()->setMaterial(resources->getMaterial());

				vegetation.push_back(std::move(tree));
			}
		}

		std::cout << "Generated " << vegetation.size() << " vegetation objects" << std::endl;
	}

	void VegetationIntegrator::generateVegetationWithCustomParams(
		const VegetationSettings& settings,
		const std::vector<float>& heightfieldData,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition,
		int lsystemIterations,
		const std::string& axiom,
		const TurtleParameters& turtleParams) {
		clearVegetation();

		std::mt19937 rng(settings.placementSeed);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		// Calculate terrain area
		float terrainWidth = settings.terrainMax.x - settings.terrainMin.x;
		float terrainDepth = settings.terrainMax.y - settings.terrainMin.y;
		float terrainArea = terrainWidth * terrainDepth;

		// Calculate number of trees to place
		int numTrees = static_cast<int>(terrainArea * settings.treeDensity);

		std::cout << "Generating vegetation with custom parameters: " << numTrees << " trees" << std::endl;
		std::cout << "  Iterations: " << lsystemIterations << ", Axiom: " << axiom << std::endl;

		// Create shared resources to reuse materials
		auto resources = std::make_shared<VegetationSharedResources>(device);

		// Generate trees with custom parameters
		for (int i = 0; i < numTrees; ++i) {
			glm::vec2 pos2D(
				settings.terrainMin.x + dist(rng) * terrainWidth,
				settings.terrainMin.y + dist(rng) * terrainDepth);

			float height = sampleHeightAt(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);
			float slope = calculateSlope(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);

			if (isSuitableForVegetation(pos2D, height, slope, settings)) {
				// Ensure tree is properly grounded
				glm::vec3 position(pos2D.x, height - 0.1f, pos2D.y);
				float scale = getRandomScale(settings.treeScaleRange, rng);

				// Generate unique seed for each tree for variety
				int treeSeed = rng();

				// Create tree with custom parameters
				auto tree = VegetationObject::createTree(device, position, glm::vec3(scale),
					treeSeed, lsystemIterations, axiom, turtleParams);

				// Apply shared material
				tree->getModel()->setMaterial(resources->getMaterial());

				vegetation.push_back(std::move(tree));
			}
		}

		std::cout << "Generated " << vegetation.size() << " vegetation objects with custom parameters" << std::endl;
	}

	void VegetationIntegrator::generateEnhancedVegetationOnTerrain(
		const VegetationSettings& settings,
		const std::vector<float>& heightfieldData,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition) {
		// Clear existing vegetation
		clearVegetation();

		std::mt19937 rng(settings.placementSeed);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		// Calculate terrain area
		float terrainWidth = settings.terrainMax.x - settings.terrainMin.x;
		float terrainDepth = settings.terrainMax.y - settings.terrainMin.y;
		float terrainArea = terrainWidth * terrainDepth;

		int numTrees = static_cast<int>(terrainArea * settings.treeDensity);

		std::cout << "Generating enhanced vegetation: " << numTrees << " trees with separate bark/leaf materials" << std::endl;

		// Create shared resources to reuse materials
		auto resources = std::make_shared<VegetationSharedResources>(device);

		// Generate enhanced trees with separate bark and leaf materials
		for (int i = 0; i < numTrees; ++i) {
			glm::vec2 pos2D(
				settings.terrainMin.x + dist(rng) * terrainWidth,
				settings.terrainMin.y + dist(rng) * terrainDepth);

			float height = sampleHeightAt(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);
			float slope = calculateSlope(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);

			if (isSuitableForVegetation(pos2D, height, slope, settings)) {
				// Ensure tree is properly grounded - place it slightly below terrain surface
				glm::vec3 position(pos2D.x, height - 0.1f, pos2D.y);
				float scale = getRandomScale(settings.treeScaleRange, rng);

				// Generate unique seed for each tree for variety
				int treeSeed = rng();

				auto tree = VegetationObject::createEnhancedTree(device, resources->getTreeMaterial(),
					position, glm::vec3(scale), treeSeed);

				enhancedVegetation.push_back(std::move(tree));
			}
		}

		std::cout << "Generated " << enhancedVegetation.size() << " enhanced vegetation objects" << std::endl;
	}

	void VegetationIntegrator::addVegetationToScene(SceneManager& sceneManager) {
		for (auto& vegObject : vegetation) {
			std::unique_ptr<vk::GameObject> gameObject = std::move(vegObject);
			sceneManager.addSpectralObject(std::move(gameObject));
		}
		vegetation.clear();
	}

	void VegetationIntegrator::addEnhancedVegetationToScene(SceneManager& sceneManager) {
		for (auto& vegObject : enhancedVegetation) {
			// For enhanced trees, we need to add both bark and leaf models as separate game objects
			if (vegObject->hasMultipleMaterials()) {
				// Add bark model as a separate game object
				if (vegObject->getBarkModel()) {
					auto barkGameObject = std::make_unique<SimpleGameObject>(
						vegObject->getBarkModel(),
						vegObject->getPosition(),
						vegObject->getScale());
					sceneManager.addSpectralObject(std::move(barkGameObject));
				}

				// Add leaf model as a separate game object
				if (vegObject->getLeafModel()) {
					auto leafGameObject = std::make_unique<SimpleGameObject>(
						vegObject->getLeafModel(),
						vegObject->getPosition(),
						vegObject->getScale());
					sceneManager.addSpectralObject(std::move(leafGameObject));
				}
			} else {
				// Legacy single-material tree
				std::unique_ptr<vk::GameObject> gameObject = std::move(vegObject);
				sceneManager.addSpectralObject(std::move(gameObject));
			}
		}
		enhancedVegetation.clear();
	}

	void VegetationIntegrator::clearVegetation() {
		vegetation.clear();
		enhancedVegetation.clear();
	}

	VegetationIntegrator::VegetationStats VegetationIntegrator::getVegetationStats() const {
		VegetationStats stats;

		for (const auto& vegObject : vegetation) {
			stats.treeCount++;
		}

		return stats;
	}

	float VegetationIntegrator::sampleHeightAt(
		const glm::vec2& worldPos,
		const std::vector<float>& heightfieldData,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition) const {
		// Convert world position to heightfield coordinates
		float localX = (worldPos.x - terrainPosition.x) / terrainScale.x;
		float localZ = (worldPos.y - terrainPosition.z) / terrainScale.z;

		// Normalize to [0, 1] range
		float normalizedX = (localX + 1.0f) * 0.5f;
		float normalizedZ = (localZ + 1.0f) * 0.5f;

		// Convert to grid coordinates
		float gridX = normalizedX * (gridSize - 1);
		float gridZ = normalizedZ * (gridSize - 1);

		// Clamp to valid range
		gridX = std::max(0.0f, std::min(gridX, static_cast<float>(gridSize - 1)));
		gridZ = std::max(0.0f, std::min(gridZ, static_cast<float>(gridSize - 1)));

		// Bilinear interpolation
		int x0 = static_cast<int>(gridX);
		int z0 = static_cast<int>(gridZ);
		int x1 = std::min(x0 + 1, gridSize - 1);
		int z1 = std::min(z0 + 1, gridSize - 1);

		float fx = gridX - x0;
		float fz = gridZ - z0;

		float h00 = heightfieldData[z0 * gridSize + x0];
		float h10 = heightfieldData[z0 * gridSize + x1];
		float h01 = heightfieldData[z1 * gridSize + x0];
		float h11 = heightfieldData[z1 * gridSize + x1];

		float h0 = h00 * (1.0f - fx) + h10 * fx;
		float h1 = h01 * (1.0f - fx) + h11 * fx;
		float height = h0 * (1.0f - fz) + h1 * fz;

		// Apply terrain scale and position
		return terrainPosition.y + height * terrainScale.y;
	}

	float VegetationIntegrator::calculateSlope(
		const glm::vec2& worldPos,
		const std::vector<float>& heightfieldData,
		int gridSize,
		const glm::vec3& terrainScale,
		const glm::vec3& terrainPosition) const {
		const float epsilon = 0.1f;

		float heightCenter = sampleHeightAt(worldPos, heightfieldData, gridSize, terrainScale, terrainPosition);
		float heightRight = sampleHeightAt(worldPos + glm::vec2(epsilon, 0.0f), heightfieldData, gridSize, terrainScale, terrainPosition);
		float heightUp = sampleHeightAt(worldPos + glm::vec2(0.0f, epsilon), heightfieldData, gridSize, terrainScale, terrainPosition);

		// Calculate gradients
		float dx = (heightRight - heightCenter) / epsilon;
		float dz = (heightUp - heightCenter) / epsilon;

		// Calculate slope angle in degrees
		float slopeRadians = std::atan(std::sqrt(dx * dx + dz * dz));
		return slopeRadians * 180.0f / glm::pi<float>();
	}

	bool VegetationIntegrator::isSuitableForVegetation(
		// VegetationType type, // Removed type parameter
		const glm::vec2& worldPos,
		float height,
		float slope,
		const VegetationSettings& settings) const {
		return slope <= settings.maxTreeSlope;
	}

	float VegetationIntegrator::getRandomScale(const glm::vec2& scaleRange, std::mt19937& rng) const {
		std::uniform_real_distribution<float> dist(scaleRange.x, scaleRange.y);
		return dist(rng);
	}

}  // namespace procedural
