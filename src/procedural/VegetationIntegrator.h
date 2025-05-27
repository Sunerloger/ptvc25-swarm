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

    // Integrates L-System vegetation with the game's terrain and scene management
    class VegetationIntegrator {
    public:
        struct VegetationSettings {
            // Density controls (plants per unit area)
            float fernDensity = 0.05f;      // Ferns per square unit
            
            // Slope constraints (in degrees)
            float maxBushSlope = 35.0f;     // Maximum slope angle for ferns
            
            // Scale variation
            glm::vec2 fernScaleRange = glm::vec2(0.5f, 1.0f);
            
            // Terrain bounds
            glm::vec2 terrainMin = glm::vec2(-100.0f, -100.0f);
            glm::vec2 terrainMax = glm::vec2(100.0f, 100.0f);
            
            // Random seed for vegetation placement
            int placementSeed = 12345;
        };

        VegetationIntegrator(vk::Device& device);
        ~VegetationIntegrator() = default;

        // Generate and place vegetation on terrain using the VegetationManager
        void generateVegetationOnTerrain(
            const VegetationSettings& settings,
            const std::vector<float>& heightfieldData,
            int gridSize,
            const glm::vec3& terrainScale,
            const glm::vec3& terrainPosition
        );

        // Add individual vegetation objects to the scene
        void addVegetationToScene(SceneManager& sceneManager);

        // Clear all generated vegetation
        void clearVegetation();

        // Get generated vegetation count by type
        struct VegetationStats {
            int fernCount = 0;
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
            const glm::vec3& terrainPosition
        ) const;
        
        // Calculate terrain slope at a position
        float calculateSlope(
            const glm::vec2& worldPos,
            const std::vector<float>& heightfieldData,
            int gridSize,
            const glm::vec3& terrainScale,
            const glm::vec3& terrainPosition
        ) const;
        
        // Check if a position is suitable for a specific vegetation type
        bool isSuitableForVegetation(
            VegetationType type,
            const glm::vec2& worldPos,
            float height,
            float slope,
            const VegetationSettings& settings
        ) const;
        
        // Generate random scale within range
        float getRandomScale(const glm::vec2& scaleRange, std::mt19937& rng) const;
    };

} // namespace procedural
