#pragma once

#include "LSystem.h"
#include "VegetationObject.h"
#include "VegetationSharedResources.h"
#include "../vk/vk_device.h"
#include "../scene/SceneManager.h"
#include <vector>
#include <memory>
#include <random>

namespace procedural {

	class VegetationIntegrator {
	   public:
		struct VegetationSettings {
			float treeDensity = 0.05f;

			float maxTreeSlope = 35.0f;

			glm::vec2 treeScaleRange = glm::vec2(0.5f, 1.0f);

			glm::vec2 terrainMin = glm::vec2(-100.0f, -100.0f);
			glm::vec2 terrainMax = glm::vec2(100.0f, 100.0f);

			int placementSeed = 12345;
		};

		VegetationIntegrator(vk::Device& device);
		~VegetationIntegrator() = default;

		// Generate and place vegetation on terrain
		void generateVegetationOnTerrain(
			const VegetationSettings& settings,
			const std::vector<float>& heightfieldData,
			int gridSize,
			const glm::vec3& terrainScale,
			const glm::vec3& terrainPosition);

		// Generate vegetation with custom L-System parameters
		void generateVegetationWithCustomParams(
			const VegetationSettings& settings,
			const std::vector<float>& heightfieldData,
			int gridSize,
			const glm::vec3& terrainScale,
			const glm::vec3& terrainPosition,
			int lsystemIterations,
			const std::string& axiom,
			const TurtleParameters& turtleParams);

		// Add individual vegetation objects to the scene
		void addVegetationToScene(SceneManager& sceneManager);

		// Clear all generated vegetation
		void clearVegetation();

		// Get generated vegetation count by type
		struct VegetationStats {
			int treeCount = 0;
		};

		VegetationStats getVegetationStats() const;

	   private:
		vk::Device& device;
		std::vector<std::unique_ptr<VegetationObject>> vegetation;

		// Height sampling from heightfield data
		float sampleHeightAt(
			const glm::vec2& worldPos,
			const std::vector<float>& heightfieldData,
			int gridSize,
			const glm::vec3& terrainScale,
			const glm::vec3& terrainPosition) const;

		// Calculate terrain slope at a position
		float calculateSlope(
			const glm::vec2& worldPos,
			const std::vector<float>& heightfieldData,
			int gridSize,
			const glm::vec3& terrainScale,
			const glm::vec3& terrainPosition) const;

		// Check if a position is suitable for a specific vegetation type
		bool isSuitableForVegetation(
			VegetationType type,
			const glm::vec2& worldPos,
			float height,
			float slope,
			const VegetationSettings& settings) const;

		// Generate random scale within range
		float getRandomScale(const glm::vec2& scaleRange, std::mt19937& rng) const;
	};

}  // namespace procedural
