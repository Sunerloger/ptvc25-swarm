#include "FernParameterTuner.h"
#include <iostream>
#include <algorithm>

// Include INI reader
#include "INIReader.h"

namespace procedural {

    FernParameterTuner::FernParameterTuner() {
        // Initialize default parameters
        currentParameters.iterations = 3;
        currentParameters.axiom = "FFFFFF";
        
        // Default turtle parameters
        currentParameters.turtleParams.stepLength = 0.3f;
        currentParameters.turtleParams.angleIncrement = 30.0f;
        currentParameters.turtleParams.radiusDecay = 0.7f;
        currentParameters.turtleParams.lengthDecay = 0.87f;
        currentParameters.turtleParams.initialRadius = 0.12f;
        currentParameters.turtleParams.initialColor = glm::vec3(0.3f, 0.2f, 0.1f);
        currentParameters.turtleParams.leafColor = glm::vec3(0.15f, 0.8f, 0.2f);
        
        // Default vegetation settings
        currentParameters.vegetationSettings.fernDensity = 0.002f;
        currentParameters.vegetationSettings.maxBushSlope = 35.0f;
        currentParameters.vegetationSettings.fernScaleRange = glm::vec2(0.5f, 1.0f);
        currentParameters.vegetationSettings.placementSeed = 12345;
        
        // Default advanced settings
        currentParameters.enableTuning = true;
        currentParameters.regenerationMode = 0;
        currentParameters.checkFrequency = 1.0f;
    }

    void FernParameterTuner::initialize(const std::string& iniFilePath) {
        iniFilePath_ = iniFilePath;
        
        // Try to load initial parameters
        if (!loadParametersFromFile()) {
            std::cout << "FernParameterTuner: Could not load " << iniFilePath_ 
                      << ", using default parameters" << std::endl;
        } else {
            std::cout << "FernParameterTuner: Loaded parameters from " << iniFilePath_ << std::endl;
        }
    }

    void FernParameterTuner::update(float deltaTime) {
        if (!enabled_ || !currentParameters.enableTuning) {
            return;
        }
        
        timeSinceLastCheck_ += deltaTime;
        
        if (timeSinceLastCheck_ >= currentParameters.checkFrequency) {
            timeSinceLastCheck_ = 0.0f;
            
            if (hasFileChanged()) {
                std::cout << "FernParameterTuner: File changed, reloading parameters..." << std::endl;
                
                if (loadParametersFromFile()) {
                    std::cout << "FernParameterTuner: Parameters reloaded successfully" << std::endl;
                    
                    if (regenerationCallback_) {
                        regenerationCallback_(currentParameters);
                    }
                } else {
                    std::cout << "FernParameterTuner: Failed to reload parameters" << std::endl;
                }
            }
        }
    }

    void FernParameterTuner::setRegenerationCallback(const RegenerationCallback& callback) {
        regenerationCallback_ = callback;
    }

    void FernParameterTuner::forceReload() {
        std::cout << "FernParameterTuner: Force reloading parameters..." << std::endl;
        
        if (loadParametersFromFile()) {
            std::cout << "FernParameterTuner: Parameters force reloaded successfully" << std::endl;
            
            if (regenerationCallback_) {
                regenerationCallback_(currentParameters);
            }
        } else {
            std::cout << "FernParameterTuner: Failed to force reload parameters" << std::endl;
        }
    }

    bool FernParameterTuner::loadParametersFromFile() {
        try {
            std::string resolvedPath = vk::AssetLoader::getInstance().resolvePath(iniFilePath_);
            
            if (!fs::exists(resolvedPath)) {
                fileExists_ = false;
                return false;
            }
            
            fileExists_ = true;
            lastWriteTime_ = fs::last_write_time(resolvedPath);
            
            INIReader reader(resolvedPath);
            
            if (reader.ParseError() < 0) {
                std::cerr << "FernParameterTuner: Error parsing INI file: " << resolvedPath << std::endl;
                return false;
            }
            
            // Load L-System generation parameters
            currentParameters.iterations = reader.GetInteger("LSystemGeneration", "iterations", 3);
            currentParameters.axiom = reader.Get("LSystemGeneration", "axiom", "FFFFFF");
            
            // Load turtle parameters
            currentParameters.turtleParams.stepLength = 
                static_cast<float>(reader.GetReal("TurtleParameters", "stepLength", 0.3));
            currentParameters.turtleParams.angleIncrement = 
                static_cast<float>(reader.GetReal("TurtleParameters", "angleIncrement", 30.0));
            currentParameters.turtleParams.radiusDecay = 
                static_cast<float>(reader.GetReal("TurtleParameters", "radiusDecay", 0.7));
            currentParameters.turtleParams.lengthDecay = 
                static_cast<float>(reader.GetReal("TurtleParameters", "lengthDecay", 0.87));
            currentParameters.turtleParams.initialRadius = 
                static_cast<float>(reader.GetReal("TurtleParameters", "initialRadius", 0.12));
            
            // Load colors
            float initialR = static_cast<float>(reader.GetReal("Colors", "initialColorR", 0.3));
            float initialG = static_cast<float>(reader.GetReal("Colors", "initialColorG", 0.2));
            float initialB = static_cast<float>(reader.GetReal("Colors", "initialColorB", 0.1));
            currentParameters.turtleParams.initialColor = parseColor(initialR, initialG, initialB);
            
            float leafR = static_cast<float>(reader.GetReal("Colors", "leafColorR", 0.15));
            float leafG = static_cast<float>(reader.GetReal("Colors", "leafColorG", 0.8));
            float leafB = static_cast<float>(reader.GetReal("Colors", "leafColorB", 0.2));
            currentParameters.turtleParams.leafColor = parseColor(leafR, leafG, leafB);
            
            // Load vegetation placement parameters
            currentParameters.vegetationSettings.fernDensity = 
                static_cast<float>(reader.GetReal("VegetationPlacement", "fernDensity", 0.002));
            currentParameters.vegetationSettings.maxBushSlope = 
                static_cast<float>(reader.GetReal("VegetationPlacement", "maxBushSlope", 35.0));
            
            float scaleMin = static_cast<float>(reader.GetReal("VegetationPlacement", "fernScaleMin", 0.5));
            float scaleMax = static_cast<float>(reader.GetReal("VegetationPlacement", "fernScaleMax", 1.0));
            currentParameters.vegetationSettings.fernScaleRange = glm::vec2(scaleMin, scaleMax);
            
            currentParameters.vegetationSettings.placementSeed = 
                reader.GetInteger("VegetationPlacement", "placementSeed", 12345);
            
            // Load advanced parameters
            currentParameters.enableTuning = 
                reader.GetBoolean("AdvancedParameters", "enableTuning", true);
            currentParameters.regenerationMode = 
                reader.GetInteger("AdvancedParameters", "regenerationMode", 0);
            currentParameters.checkFrequency = 
                static_cast<float>(reader.GetReal("AdvancedParameters", "checkFrequency", 1.0));
            
            validateParameters();
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "FernParameterTuner: Exception loading parameters: " << e.what() << std::endl;
            return false;
        }
    }

    bool FernParameterTuner::hasFileChanged() {
        if (!fileExists_) {
            return false;
        }
        
        try {
            std::string resolvedPath = vk::AssetLoader::getInstance().resolvePath(iniFilePath_);
            
            if (!fs::exists(resolvedPath)) {
                fileExists_ = false;
                return false;
            }
            
            auto currentWriteTime = fs::last_write_time(resolvedPath);
            
            if (currentWriteTime != lastWriteTime_) {
                lastWriteTime_ = currentWriteTime;
                return true;
            }
            
            return false;
            
        } catch (const std::exception& e) {
            std::cerr << "FernParameterTuner: Exception checking file modification: " << e.what() << std::endl;
            return false;
        }
    }

    glm::vec3 FernParameterTuner::parseColor(float r, float g, float b) {
        return glm::vec3(
            std::clamp(r, 0.0f, 1.0f),
            std::clamp(g, 0.0f, 1.0f),
            std::clamp(b, 0.0f, 1.0f)
        );
    }

    void FernParameterTuner::validateParameters() {
        // Clamp iterations to reasonable range
        currentParameters.iterations = std::clamp(currentParameters.iterations, 1, 6);
        
        // Clamp turtle parameters to reasonable ranges
        currentParameters.turtleParams.stepLength = 
            std::clamp(currentParameters.turtleParams.stepLength, 0.01f, 5.0f);
        currentParameters.turtleParams.angleIncrement = 
            std::clamp(currentParameters.turtleParams.angleIncrement, 1.0f, 90.0f);
        currentParameters.turtleParams.radiusDecay = 
            std::clamp(currentParameters.turtleParams.radiusDecay, 0.1f, 1.0f);
        currentParameters.turtleParams.lengthDecay = 
            std::clamp(currentParameters.turtleParams.lengthDecay, 0.1f, 1.0f);
        currentParameters.turtleParams.initialRadius = 
            std::clamp(currentParameters.turtleParams.initialRadius, 0.001f, 1.0f);
        
        // Clamp vegetation settings
        currentParameters.vegetationSettings.fernDensity = 
            std::clamp(currentParameters.vegetationSettings.fernDensity, 0.0001f, 0.1f);
        currentParameters.vegetationSettings.maxBushSlope = 
            std::clamp(currentParameters.vegetationSettings.maxBushSlope, 0.0f, 90.0f);
        
        // Ensure scale range is valid
        if (currentParameters.vegetationSettings.fernScaleRange.x > 
            currentParameters.vegetationSettings.fernScaleRange.y) {
            std::swap(currentParameters.vegetationSettings.fernScaleRange.x,
                     currentParameters.vegetationSettings.fernScaleRange.y);
        }
        currentParameters.vegetationSettings.fernScaleRange.x = 
            std::clamp(currentParameters.vegetationSettings.fernScaleRange.x, 0.1f, 10.0f);
        currentParameters.vegetationSettings.fernScaleRange.y = 
            std::clamp(currentParameters.vegetationSettings.fernScaleRange.y, 0.1f, 10.0f);
        
        // Clamp advanced parameters
        currentParameters.checkFrequency = 
            std::clamp(currentParameters.checkFrequency, 0.1f, 10.0f);
        currentParameters.regenerationMode = 
            std::clamp(currentParameters.regenerationMode, 0, 1);
    }

} // namespace procedural
