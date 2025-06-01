#pragma once

#include <string>
#include <filesystem>
#include <chrono>
#include <functional>

#include "VegetationIntegrator.h"
#include "LSystem.h"
#include "../asset_utils/AssetLoader.h"

namespace fs = std::filesystem;

namespace procedural {

    /**
     * Real-time parameter tuning system for fern generation
     * Monitors INI file changes and triggers fern regeneration
     */
    class FernParameterTuner {
    public:
        struct FernTuningParameters {
            // L-System generation
            int iterations = 3;
            std::string axiom = "FFFFFF";
            
            // Turtle parameters
            TurtleParameters turtleParams;
            
            // Vegetation placement
            VegetationIntegrator::VegetationSettings vegetationSettings;
            
            // Advanced
            bool enableTuning = true;
            int regenerationMode = 0;  // 0=all, 1=visible only
            float checkFrequency = 1.0f;
        };

        using RegenerationCallback = std::function<void(const FernTuningParameters&)>;

        FernParameterTuner();
        ~FernParameterTuner() = default;

        // Initialize the tuning system
        void initialize(const std::string& iniFilePath);
        
        // Update the tuning system (call from main loop)
        void update(float deltaTime);
        
        // Set callback for when parameters change
        void setRegenerationCallback(const RegenerationCallback& callback);
        
        // Get current parameters
        const FernTuningParameters& getParameters() const { return currentParameters; }
        
        // Force reload parameters from file
        void forceReload();
        
        // Enable/disable the tuning system
        void setEnabled(bool enabled) { enabled_ = enabled; }
        bool isEnabled() const { return enabled_; }

    private:
        std::string iniFilePath_;
        fs::file_time_type lastWriteTime_;
        FernTuningParameters currentParameters;
        RegenerationCallback regenerationCallback_;
        
        float timeSinceLastCheck_ = 0.0f;
        bool enabled_ = true;
        bool fileExists_ = false;
        
        // Load parameters from INI file
        bool loadParametersFromFile();
        
        // Check if file has been modified
        bool hasFileChanged();
        
        // Parse color from INI (R,G,B components)
        glm::vec3 parseColor(float r, float g, float b);
        
        // Validate parameters
        void validateParameters();
    };

} // namespace procedural
