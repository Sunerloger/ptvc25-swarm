#pragma once

#include "GameBase.h"

#include "vk/vk_model.h"

#include "simulation/objects/actors/Player.h"
#include "simulation/objects/actors/PhysicsPlayer.h"
#include "simulation/objects/actors/DebugPlayer.h"
#include "simulation/objects/static/Terrain.h"
#include "simulation/objects/actors/enemies/Sprinter.h"
#include "simulation/PhysicsSimulation.h"

#include "asset_utils/AssetManager.h"
#include "AudioSystem.h"
#include "SwarmInputController.h"

#include "ui/Font.h"
#include "ui/TextComponent.h"
#include "ui/UIComponent.h"

#include "rendering/materials/CubemapMaterial.h"
#include "rendering/materials/TessellationMaterial.h"
#include "rendering/materials/WaterMaterial.h"

#include "rendering/structures/Skybox.h"
#include "rendering/structures/WaterObject.h"

#include "logical_systems/Settings.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace vk;

class Swarm : public GameBase {
   public:
	Swarm(physics::PhysicsSimulation& physicsSimulation, AssetManager& assetManager, Window& window, Device& device, input::SwarmInputController& inputController,
		RenderSystemSettings& renderSystemSettings, bool debugMode = false);
	~Swarm() override = default;

	Swarm(const Swarm&) = delete;
	Swarm& operator=(const Swarm&) = delete;

	void init() override;

	void prePhysicsUpdate() override;
	void postPhysicsUpdate() override;

	void gameActiveUpdate(float deltaTime) override;
	void gamePauseUpdate(float deltaTime) override;

	void postRenderingUpdate(EngineStats engineStats, float deltaTime) override;

	static inline std::string Name = "Swarm";
	std::string getName() const override {
		return Name;
	}

	bool isPaused() const override {
		return inputController.isPaused();
	}

	void onPlayerDeath();

   private:
	void bindInput() override;
	void toggleDebug();
	void toggleCulling();

	id_t gameTimeTextID;
	id_t gameHealthTextID;
	id_t renderedObjectsTextID;
	int oldSecond = 0;
	int lastSpawnSecond = 0;

	RenderSystemSettings& renderSystemSettings;

	physics::PhysicsSimulation& physicsSimulation;
	AssetManager& assetManager;

	Window& window;
	Device& device;

	bool debugMode;

	bool isDebugActive = false;

	physics::PhysicsPlayer::PlayerCreationSettings originalPlayerSettings;

	std::vector<float> heightfieldData; // heightfield for regeneration
	int terrainSamplesPerSide = 100;
	glm::vec3 terrainScale{100.0f, 15.0f, 100.0f};
	glm::vec3 terrainPosition{0.0f, -2.0f, 0.0f};

	float sunRotationAngle = 0.0f;
	glm::vec3 baseSunDirection = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
	float sunDistance = 100.0f;

	std::shared_ptr<Model> enemyModel;
	std::shared_ptr<Model> grenadeModel;
};