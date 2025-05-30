#include "VegetationIntegrator.h"
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

		// Calculate number of ferns to place - we only need ferns
		int numFerns = static_cast<int>(terrainArea * settings.fernDensity);

		std::cout << "Generating vegetation: " << numFerns << " ferns" << std::endl;

		// Create shared resources to reuse materials
		auto resources = std::make_shared<VegetationSharedResources>(device);

		// Generate ferns
		for (int i = 0; i < numFerns; ++i) {
			glm::vec2 pos2D(
				settings.terrainMin.x + dist(rng) * terrainWidth,
				settings.terrainMin.y + dist(rng) * terrainDepth);

			float height = sampleHeightAt(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);
			float slope = calculateSlope(pos2D, heightfieldData, gridSize, terrainScale, terrainPosition);

			if (isSuitableForVegetation(VegetationType::Fern, pos2D, height, slope, settings)) {
				glm::vec3 position(pos2D.x, height, pos2D.y);
				float scale = getRandomScale(settings.fernScaleRange, rng);

				// Generate unique seed for each fern for variety
				int fernSeed = rng();

				auto fern = VegetationObject::createFern(device, position, glm::vec3(scale), fernSeed);

				// Apply shared material
				fern->getModel()->setMaterial(resources->getMaterial(VegetationType::Fern));

				vegetation.push_back(std::move(fern));
			}
		}

		std::cout << "Generated " << vegetation.size() << " vegetation objects" << std::endl;
	}

	void VegetationIntegrator::addVegetationToScene(SceneManager& sceneManager) {
		for (auto& vegObject : vegetation) {
			// Cast to base class and move the unique_ptr
			std::unique_ptr<vk::GameObject> gameObject = std::move(vegObject);
			sceneManager.addSpectralObject(std::move(gameObject));
		}
		vegetation.clear();	 // Clear since we moved all objects
	}

	void VegetationIntegrator::clearVegetation() {
		vegetation.clear();
	}

	VegetationIntegrator::VegetationStats VegetationIntegrator::getVegetationStats() const {
		VegetationStats stats;

		for (const auto& vegObject : vegetation) {
			if (vegObject->getVegetationType() == VegetationType::Fern) {
				stats.fernCount++;
			}
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
		const float epsilon = 0.1f;	 // Small offset for finite difference

		float heightCenter = sampleHeightAt(worldPos, heightfieldData, gridSize, terrainScale, terrainPosition);
		float heightRight = sampleHeightAt(worldPos + glm::vec2(epsilon, 0.0f), heightfieldData, gridSize, terrainScale, terrainPosition);
		float heightUp = sampleHeightAt(worldPos + glm::vec2(0.0f, epsilon), heightfieldData, gridSize, terrainScale, terrainPosition);

		// Calculate gradients
		float dx = (heightRight - heightCenter) / epsilon;
		float dz = (heightUp - heightCenter) / epsilon;

		// Calculate slope angle in degrees
		float slopeRadians = std::atan(std::sqrt(dx * dx + dz * dz));
		return slopeRadians * 180.0f / M_PI;
	}

	bool VegetationIntegrator::isSuitableForVegetation(
		VegetationType type,
		const glm::vec2& worldPos,
		float height,
		float slope,
		const VegetationSettings& settings) const {
		// Only need to check for ferns
		return slope <= settings.maxBushSlope;
	}

	float VegetationIntegrator::getRandomScale(const glm::vec2& scaleRange, std::mt19937& rng) const {
		std::uniform_real_distribution<float> dist(scaleRange.x, scaleRange.y);
		return dist(rng);
	}

}  // namespace procedural
