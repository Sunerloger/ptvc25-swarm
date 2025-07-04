#include "Swarm.h"

#include "scene/SceneManager.h"
#include "procedural/VegetationIntegrator.h"
#include <glm/gtc/matrix_transform.hpp>
#include "procedural/VegetationSharedResources.h"

#include <fmt/format.h>
#include <random>

Swarm::Swarm(physics::PhysicsSimulation& physicsSimulation, AssetManager& assetManager, Window& window, Device& device, input::SwarmInputController& inputController,
	RenderSystemSettings& renderSystemSettings, bool debugMode)
	: GameBase(inputController), physicsSimulation(physicsSimulation), assetManager(assetManager), window(window), device(device), 
	renderSystemSettings(renderSystemSettings), debugMode(debugMode) {
	enemyModel = Model::createModelFromFile(device, "models:enemy.glb");
	grenadeModel = Model::createModelFromFile(device, "models:grenade.glb");
}

void Swarm::bindInput() {
	SceneManager& sceneManager = SceneManager::getInstance();

	input::SwarmInputController& swarmInput = static_cast<input::SwarmInputController&>(inputController);

	swarmInput.setup(debugMode);
	swarmInput.onMove = [this, &sceneManager](const glm::vec3& dir) {
		Player* player = sceneManager.getPlayer();
		if (player && player->isPhysicsPlayer() && player->getBodyID() != JPH::BodyID(JPH::BodyID::cInvalidBodyID)) {
			static_cast<physics::PhysicsPlayer*>(player)->setInputDirection(dir);
		}
	};
	swarmInput.onLook = [this, &sceneManager](float dx, float dy) { sceneManager.getPlayer()->handleRotation(-dx, -dy); };
	swarmInput.onJump = [this, &sceneManager]() {
		Player* player = sceneManager.getPlayer();
		if (player && player->isPhysicsPlayer() && player->getBodyID() != JPH::BodyID(JPH::BodyID::cInvalidBodyID)) {
			static_cast<physics::PhysicsPlayer*>(player)->handleJump();
		}
	};
	swarmInput.onShoot = [this, &sceneManager]() {
		Player* player = sceneManager.getPlayer();
		if (player && player->isPhysicsPlayer() && player->getBodyID() != JPH::BodyID(JPH::BodyID::cInvalidBodyID)) {
			static_cast<physics::PhysicsPlayer*>(player)->handleShoot();
		}
	};

	swarmInput.onThrowGrenade = [this, &sceneManager]() {
		Player* player = sceneManager.getPlayer();
		if (player && player->isPhysicsPlayer() && player->getBodyID() != JPH::BodyID(JPH::BodyID::cInvalidBodyID)) {
			static_cast<physics::PhysicsPlayer*>(player)->handleThrowGrenade(device, grenadeModel);
		}
	};

	swarmInput.onMoveUI = [this, &sceneManager](float dt, const glm::vec3 dir) { sceneManager.updateUIPosition(dt, dir); };
	swarmInput.onRotateUI = [this, &sceneManager](float dt, const glm::vec3 rotDir) { sceneManager.updateUIRotation(dt, rotDir); };
	swarmInput.onScaleUI = [this, &sceneManager](float dt, float scaleDir) { sceneManager.updateUIScale(dt, scaleDir); };

	swarmInput.onMoveDebug = [this, &sceneManager](float dt, const glm::vec3& dir) {
		Player* player = sceneManager.getPlayer();
		if (player) {
			static_cast<DebugPlayer*>(player)->updatePosition(dt, dir);
		}
	};
	swarmInput.onLookDebug = [this, &sceneManager](float dx, float dy) {
		Player* player = sceneManager.getPlayer();
		if (player) {
			player->handleRotation(-dx, -dy);
		}
	};
	swarmInput.onChangeSpeedDebug = [this, &sceneManager](float scrollOffset) {
		Player* player = sceneManager.getPlayer();
		if (player) {
			static_cast<DebugPlayer*>(player)->handleSpeedChange(scrollOffset);
		}
	};

	swarmInput.onToggleHudDebug = [this, &sceneManager]() {
		sceneManager.toggleUIVisibility();
	};

	swarmInput.onToggleWireframeMode = [this, &sceneManager]() {
		this->isWireframeMode = !this->isWireframeMode;
		sceneManager.toggleWireframeOnTerrainObjects(this->isWireframeMode);
		sceneManager.toggleWireframeOnWaterObjects(this->isWireframeMode);
	};

	swarmInput.onToggleCulling = [this, &sceneManager]() {
		toggleCulling();
	};

	if (debugMode) {
		swarmInput.onToggleDebug = [this, &sceneManager]() { toggleDebug(); };
	} else {
		swarmInput.onToggleDebug = []() { /* Do nothing */ };
	}
}

void Swarm::toggleDebug() {
	printf("toggleDebug: %s -> %s\n",
		isDebugActive ? "Debug" : "Gameplay",
		isDebugActive ? "Gameplay" : "Debug");

	SceneManager& sceneManager = SceneManager::getInstance();

	if (isDebugActive) {
		// switching back from debug mode to physics
		Player* currentPlayer = sceneManager.getPlayer();
		if (!currentPlayer) {
			printf("Error: Current player is null!\n");
			return;
		}

		glm::vec3 currentPos = currentPlayer->getPosition();
		CharacterCameraSettings currentCameraSettings = currentPlayer->getCameraSettings();

		float currentYaw = currentCameraSettings.yaw;
		float currentPitch = currentCameraSettings.pitch;

		originalPlayerSettings.position = JPH::RVec3(currentPos.x, currentPos.y, currentPos.z);

		originalPlayerSettings.cameraSettings.position = currentPos;
		originalPlayerSettings.cameraSettings.yaw = currentYaw;
		originalPlayerSettings.cameraSettings.pitch = currentPitch;

		sceneManager.setPlayer(std::make_unique<physics::PhysicsPlayer>(originalPlayerSettings, physicsSimulation.getPhysicsSystem()));
	} else {
		// switching from physics mode to debug mode
		Player* playerRef = sceneManager.getPlayer();
		if (!playerRef) {
			printf("Error: Current player is null!\n");
			return;
		}

		CharacterCameraSettings cameraSettings = playerRef->getCameraSettings();
		float currentYaw = cameraSettings.yaw;
		float currentPitch = cameraSettings.pitch;

		if (playerRef->isPhysicsPlayer()) {
			physics::PhysicsPlayer* physPlayer = static_cast<physics::PhysicsPlayer*>(playerRef);
			auto settings = physPlayer->getCreationSettings();
			originalPlayerSettings.position = settings.position;
			originalPlayerSettings.playerSettings = settings.playerSettings;
			originalPlayerSettings.cameraSettings = settings.cameraSettings;
			originalPlayerSettings.characterSettings = settings.characterSettings;
			originalPlayerSettings.inUserData = settings.inUserData;
		}

		auto movementSpeed = playerRef->getMovementSpeed();

		auto debugPlayer = std::make_unique<DebugPlayer>(cameraSettings, movementSpeed);

		sceneManager.setPlayer(std::move(debugPlayer));

		printf("Switched to debug mode: yaw=%f, pitch=%f\n", currentYaw, currentPitch);
	}
	sceneManager.toggleDebugMenu();
	isDebugActive = !isDebugActive;
}

void Swarm::onPlayerDeath() {

	audio::AudioSystem& audioSystem = audio::AudioSystem::getInstance();
	audio::SoundSettings soundSettings{};
	soundSettings.volume = 5.0;
	audioSystem.playSound("death", soundSettings);

	input::SwarmInputController& swarmInput = static_cast<input::SwarmInputController&>(inputController);
	swarmInput.setContext(input::SwarmInputController::ContextID::Death);
	SceneManager& sceneManager = SceneManager::getInstance();

	std::string time = "";

	auto objPair = sceneManager.getObject(gameTimeTextID);
	if (objPair.first != SceneClass::INVALID) {
		if (auto ui = objPair.second) {
			if (auto text = static_cast<TextComponent*>(ui)) {
				time = text->getText();
			}
		}
	}

	// Clear existing UI objects
	sceneManager.clearUIObjects();

	// Create background
	UIComponentCreationSettings hudSettings{};
	hudSettings.window = window.getGLFWWindow();
	hudSettings.model = Model::createModelFromFile(device, "models:quad.glb", true);
	hudSettings.name = "you_died_quad";
	hudSettings.controllable = false;
	hudSettings.anchorRight = false;
	hudSettings.anchorBottom = false;
	hudSettings.centerHorizontal = true;
	hudSettings.centerVertical = true;
	hudSettings.isDebugMenuComponent = false;
	sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

	// Create "You died" text
	Font font;
	TextComponent* deathText = new TextComponent(
		device,
		font,
		"You died",
		"you_died_text",
		/* controllable: */ false,
		/* centerHorizontal: */ true,
		/* horizontalOffset: */ 0.0f,
		/* centerVertical:   */ true,
		/* verticalOffset: */ 0.0f,
		/* anchorRight: */ false,
		/* anchorBottom: */ false,
		/* isDebugMenuComponent: */ false,
		window.getGLFWWindow());
	sceneManager.addUIObject(std::unique_ptr<UIComponent>(deathText));

	// Create "Time: <time>" text
	TextComponent* deathTime = new TextComponent(
		device,
		font,
		time,
		"you_died_time",
		/* controllable: */ false,
		/* centerHorizontal: */ true,
		/* horizontalOffset: */ 0.0f,
		/* centerVertical:   */ true,
		/* verticalOffset: */ -300.0f,
		/* anchorRight: */ false,
		/* anchorBottom: */ false,
		/* isDebugMenuComponent: */ false,
		window.getGLFWWindow());
	sceneManager.addUIObject(std::unique_ptr<UIComponent>(deathTime));
}

void Swarm::init() {

	audio::AudioSystem& audioSystem = audio::AudioSystem::getInstance();

	audioSystem.loadSound("gun", "audio:gun_shot.mp3");
	audioSystem.loadSound("ambience", "audio:forest_background.mp3");
	audioSystem.loadSound("death", "audio:death.mp3");
	audioSystem.loadSound("hurt", "audio:hurt.mp3");
	audioSystem.loadSound("growl", "audio:growl.mp3");
	audioSystem.loadSound("explosion", "audio:explosion.mp3");
	audioSystem.loadSound("grenade_pin", "audio:grenade_pin.mp3");
	audio::SoundSettings soundSettings{};
	soundSettings.looping = true;
	soundSettings.volume = 0.1f;
	audioSystem.playSound("ambience", soundSettings, "background_ambience");
	audioSystem.setProtected("background_ambience", true);

	SceneManager& sceneManager = SceneManager::getInstance();

	// register assets that are reused later with asset manager so they don't fall out of scope and can still be referenced

	float maxTerrainHeight = 25.0f;

	// Player
	{
		float playerHeight = 1.40f;
		float playerRadius = 0.3f;
		Ref<Shape> characterShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * playerHeight + playerRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * playerHeight, playerRadius)).Create().Get();

		CharacterCameraSettings cameraSettings = {};
		cameraSettings.cameraOffsetFromCharacter = glm::vec3(0.0f, playerHeight + playerRadius, 0.0f);

		physics::PhysicsPlayer::PlayerSettings playerSettings = {};
		playerSettings.movementSpeed = 7.0f;
		playerSettings.deathCallback = [this] { onPlayerDeath(); };

		JPH::CharacterSettings characterSettings = {};
		characterSettings.mGravityFactor = 1.0f;
		characterSettings.mFriction = 10.0f;
		characterSettings.mShape = characterShape;
		characterSettings.mLayer = physics::Layers::MOVING;
		characterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -playerRadius);	 // Accept contacts that touch the lower sphere of the capsule

		physics::PhysicsPlayer::PlayerCreationSettings playerCreationSettings = {};
		playerCreationSettings.characterSettings = characterSettings;
		playerCreationSettings.cameraSettings = cameraSettings;
		playerCreationSettings.playerSettings = playerSettings;
		playerCreationSettings.position = JPH::RVec3(0.0f, maxTerrainHeight + 5, 0.0f);  // Increased Y position to start higher above terrain

		sceneManager.setPlayer(std::make_unique<physics::PhysicsPlayer>(playerCreationSettings, physicsSimulation.getPhysicsSystem()));

		glm::vec3 playerPos = sceneManager.getPlayer()->getPosition();
		glm::vec3 sunPos = playerPos - baseSunDirection * sunDistance;
		
		sceneManager.setSun(make_unique<lighting::Sun>(sunPos, baseSunDirection, glm::vec3(1.0f, 1.0f, 1.0f)));
	}

	// TODO terrain generation and sync of collider and model is way too complicated here
	// Terrain
	int samplesPerSide = 100;
	{
		float noiseScale = 5.0f;  // Controls the "frequency" of the noise

		TessellationMaterial::MaterialCreationData terrainCreationData = {};
		terrainCreationData.textureRepetition = glm::vec2(samplesPerSide / 20.0f, samplesPerSide / 20.0f);
		terrainCreationData.heightScale = maxTerrainHeight; // offset in gpu

		// Generate terrain model with heightmap
		auto result = vk::Model::createTerrainModel(
			device,
			samplesPerSide,
			"textures:ground/dirt.png",	 // Tile texture path
			noiseScale,
			/* loadHeightTexture */ false,
			/* heightTexturePath */ "none",
			/* seed */ -1, // if -1: use random
			/* useTessellation */ true,
			terrainCreationData
		);

		// Store heightfield data for vegetation and parameter tuning
		if (result.second.size() >= samplesPerSide * samplesPerSide) {
			heightfieldData = result.second;  // Copy the heightfield data
		}
		else {
			// Create flat heightfield if result doesn't have enough data
			heightfieldData.resize(samplesPerSide * samplesPerSide, 0.0f);
		}

		// Store terrain parameters for regeneration
		terrainSamplesPerSide = samplesPerSide;
		terrainScale = glm::vec3{ 100.0f, maxTerrainHeight, 100.0f };
		terrainPosition = glm::vec3{ 0.0, -2.0, 0.0 };

		// create terrain with procedural heightmap using perlin noise
		// create terrain with the generated heightmap data
		auto terrain = std::make_unique<physics::Terrain>(
			physicsSimulation.getPhysicsSystem(),
			std::move(result.first),
			glm::vec3{ 0.0, -2.0, 0.0 },	// position slightly below origin to prevent falling through
			terrainScale, // physics model scale
			std::move(result.second));

		sceneManager.addTerrainObject(std::move(terrain));

		// Create shared vegetation resources to avoid descriptor pool exhaustion
		auto sharedResources = std::make_shared<procedural::VegetationSharedResources>(device);
	}

	// Vegetation (L-Systems) - moved inside terrain block to access heightfield data
	{
		procedural::VegetationIntegrator vegetationIntegrator(device);

		procedural::VegetationIntegrator::VegetationSettings vegSettings;
		vegSettings.terrainMin = glm::vec2(-70.0f, -70.0f);
		vegSettings.terrainMax = glm::vec2(70.0f, 70.0f);

		vegSettings.treeDensity = 0.002f;

		// Slope constraints for realistic placement
		vegSettings.maxTreeSlope = 30.0f;

		// Scale variation for much larger, more impressive trees
		vegSettings.treeScaleRange = glm::vec2(1.2f, 2.5f);

		// Use random seed for deterministic vegetation
		vegSettings.placementSeed = 12345;

		try {
			// Generate enhanced vegetation on terrain using the heightfield data
			vegetationIntegrator.generateEnhancedVegetationOnTerrain(
				vegSettings,
				heightfieldData,
				samplesPerSide,
				glm::vec3(100.0f, maxTerrainHeight, 100.0f),
				glm::vec3(0.0, -2.0, 0.0));

			// Add generated enhanced vegetation to scene
			vegetationIntegrator.addEnhancedVegetationToScene(sceneManager);

			// Report statistics
			auto stats = vegetationIntegrator.getVegetationStats();
			printf("Added enhanced L-System vegetation: %d trees\n", stats.treeCount);

		}
		catch (const std::exception& e) {
			printf("Error generating vegetation: %s\n", e.what());
		}
	}

	// Skybox
	{
		std::array<std::string, 6> cubemapFaces = {
			"textures:skybox/learnopengl/right.jpg",
			"textures:skybox/learnopengl/left.jpg",
			"textures:skybox/learnopengl/top.jpg",
			"textures:skybox/learnopengl/bottom.jpg",
			"textures:skybox/learnopengl/front.jpg",
			"textures:skybox/learnopengl/back.jpg"};
		sceneManager.addSpectralObject(std::make_unique<Skybox>(device, cubemapFaces));
	}

	// Enemies
	{
		float enemyHullHeight = 1.5f;
		float enemyRadius = 0.3f;
		JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));
		float enemySpawnMinRadius = 20.0f;
		float enemySpawnMaxRadius = 70.0f;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> angleDist(
			0.0f, 2.0f * glm::pi<float>());
		std::uniform_real_distribution<float> radiusSqDist(
			enemySpawnMinRadius * enemySpawnMinRadius,
			enemySpawnMaxRadius * enemySpawnMaxRadius);	 // squared to have density distribution uniformly in spawn ring

		Ref<Shape> enemyShape = enemyShapeSettings.Create().Get();
		physics::Sprinter::SprinterSettings sprinterSettings = {};
		sprinterSettings.model = enemyModel;

		JPH::CharacterSettings enemyCharacterSettings = {};
		enemyCharacterSettings.mLayer = physics::Layers::MOVING;
		enemyCharacterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -enemyRadius);	 // accept contacts that touch the lower sphere of the capsule
		enemyCharacterSettings.mFriction = 1.0f;
		enemyCharacterSettings.mShape = enemyShape;
		enemyCharacterSettings.mGravityFactor = 1.0f;

		physics::Sprinter::SprinterCreationSettings sprinterCreationSettings = {};
		sprinterCreationSettings.sprinterSettings = sprinterSettings;
		sprinterCreationSettings.characterSettings = enemyCharacterSettings;

		auto playerPos = sceneManager.getPlayer()->getPosition();

		for (int i = 0; i < 10; ++i) {
			float angle = angleDist(gen);
			float radius = std::sqrt(radiusSqDist(gen));
			auto playerPos = sceneManager.getPlayer()->getPosition();

			sprinterCreationSettings.position = RVec3(playerPos.x + std::cos(angle) * radius, maxTerrainHeight+1, playerPos.z + std::sin(angle) * radius);

			sceneManager.addEnemy(std::make_unique<physics::Sprinter>(sprinterCreationSettings, physicsSimulation.getPhysicsSystem()));
		}
	}

	// Water
	{
		int samplesPerSidePatch = 10;
		
		float patchSize = 50.0f;
		int patchesPerSide = 40;
		
		auto waterMaterial = std::make_shared<WaterMaterial>(device, "textures:water.png");

		CreateWaterData waterData{};
		waterData.maxTessLevel = 8.0f;
		waterData.minTessDistance = 50.0f;
		waterData.maxTessDistance = 500.0f;
		waterData.textureRepetition = glm::vec2(samplesPerSidePatch - 1.0f, samplesPerSidePatch - 1.0f);
		waterMaterial->setWaterData(waterData);

		std::vector<glm::vec4> waves;
		waves.push_back(glm::vec4{ 1.0f, 1.0f, 0.25f, 60.0f });
		waves.push_back(glm::vec4{ 1.0f, 0.6f, 0.25f, 31.0f });
		waves.push_back(glm::vec4{ 1.0f, 1.3f, 0.25f, 18.0f });
		waterMaterial->setWaves(waves);

		std::shared_ptr<Model> waterModel = std::shared_ptr<Model>(Model::createWaterModel(device, samplesPerSidePatch, waves));
		
		waterModel->setMaterial(waterMaterial);

		WaterObject::WaterCreationSettings waterCreationSettings = {};

		for (int i = -patchesPerSide / 2; i < patchesPerSide / 2; i++) {
			for (int j = -patchesPerSide / 2; j < patchesPerSide / 2; j++) {
				waterCreationSettings.position = glm::vec3{ i*patchSize*2, -20.0f, j*patchSize*2 };
				waterCreationSettings.waterScale = patchSize;
				sceneManager.addWaterObject(std::make_unique<WaterObject>(waterModel, waterCreationSettings));
			}
		}
	}

	// UI
	{
		UIComponentCreationSettings hudSettings{};
		Font font;
		hudSettings.window = window.getGLFWWindow();

		// Standard Debug quad
		hudSettings.model = Model::createModelFromFile(device, "models:quad.glb", true);
		hudSettings.name = "debug_quad_standard";
		hudSettings.controllable = false;
		hudSettings.anchorRight = true;
		hudSettings.anchorBottom = false;
		hudSettings.centerHorizontal = false;
		hudSettings.centerVertical = true;
		hudSettings.isDebugMenuComponent = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// Debug quad
		hudSettings.model = Model::createModelFromFile(device, "models:quad.glb", true);
		hudSettings.name = "debug_quad";
		hudSettings.controllable = false;
		hudSettings.anchorRight = true;
		hudSettings.anchorBottom = false;
		hudSettings.centerHorizontal = false;
		hudSettings.centerVertical = true;
		hudSettings.isDebugMenuComponent = true;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// F1: Toggle HUD
		TextComponent* debug_text_f1 = new TextComponent(
			device,
			font,
			"F1: Toggle HUD",
			"debug_text_toggle_hud",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ 175.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f1));

		// F8: Toggle Culling
		TextComponent* debug_text_f8 = new TextComponent(
			device,
			font,
			"F8: Toggle \n Culling",
			"debug_text_toggle_culling",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ 100.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f8));

		// F9: Toggle Wireframe terrain
		TextComponent* debug_text_f9 = new TextComponent(
			device,
			font,
			"F9: Toggle \n Wireframe Terrain",
			"debug_text_toggle_menu",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ 000.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f9));

		// F10: Toggle Debug Mode
		TextComponent* debug_text_f10 = new TextComponent(
			device,
			font,
			"F10: Toggle \n Debug Mode",
			"debug_text_toggle_menu",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ -100.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ false,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f10));

		// F11: Toggle Fullscreen
		TextComponent* debug_text_f11 = new TextComponent(
			device,
			font,
			"F11: Toggle \n Fullscreen",
			"debug_text_toggle_fullscreen",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ -200.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f11));

		// Clock quad
		hudSettings.model = Model::createModelFromFile(device, "models:quad.glb", true);
		hudSettings.name = "clock_quad";
		hudSettings.controllable = false;
		hudSettings.anchorRight = false;
		hudSettings.anchorBottom = false;
		hudSettings.centerHorizontal = true;
		hudSettings.centerVertical = false;
		hudSettings.isDebugMenuComponent = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// Health quad
		hudSettings.model = Model::createModelFromFile(device, "models:quad.glb", true);
		hudSettings.name = "health_quad";
		hudSettings.controllable = false;
		hudSettings.anchorRight = false;
		hudSettings.anchorBottom = true;
		hudSettings.centerHorizontal = true;
		hudSettings.centerVertical = false;
		hudSettings.isDebugMenuComponent = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// Health text
		TextComponent* healthText = new TextComponent(
			device,
			font,
			"Health: 100%",
			"health_text",
			/* controllable: */ false,
			/* centerHorizontal: */ true,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ false,
			/* verticalOffset: */ 0.0f,
			/* anchorRight: */ false,
			/* anchorBottom: */ true,
			/* isDebugMenuComponent: */ false,
			window.getGLFWWindow());
		gameHealthTextID = sceneManager.addUIObject(
			std::unique_ptr<UIComponent>(healthText));

		// Clock
		TextComponent* gameTimeText = new TextComponent(
			device,
			font,
			"Time: 00:00",
			"clock",
			/* controllable: */ false,
			/* centerHorizontal: */ true,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ false,
			/* verticalOffset: */ 0.0f,
			/* anchorRight: */ false,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ false,
			window.getGLFWWindow());
		gameTimeTextID = sceneManager.addUIObject(
			std::unique_ptr<UIComponent>(gameTimeText));

		// rendered objects
		TextComponent* renderedObjectsText = new TextComponent(
			device,
			font,
			"0",
			"rendered_objects",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ false,
			/* verticalOffset: */ 0.0f,
			/* anchorRight: */ false,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		renderedObjectsTextID = sceneManager.addUIObject(
			std::unique_ptr<UIComponent>(renderedObjectsText));

		// USPS
		hudSettings.model = Model::createModelFromFile(device, "models:USPS.glb", true);
		hudSettings.name = "usps";
		hudSettings.controllable = false;
		hudSettings.anchorRight = true;
		hudSettings.anchorBottom = true;
		hudSettings.centerHorizontal = false;
		hudSettings.centerVertical = false;
		hudSettings.isDebugMenuComponent = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// Crosshair
		hudSettings.model = Model::createModelFromFile(device, "models:crosshair.glb", true);
		hudSettings.name = "crosshair";
		hudSettings.controllable = false;
		hudSettings.anchorRight = false;
		hudSettings.anchorBottom = false;
		hudSettings.centerHorizontal = true;
		hudSettings.centerVertical = true;
		hudSettings.isDebugMenuComponent = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));
	}
}

void Swarm::gameActiveUpdate(float deltaTime) {
	SceneManager& sceneManager = SceneManager::getInstance();

	int newSecond = static_cast<int>(sceneManager.realTime);

	if (newSecond > oldSecond) {
		auto objPair = sceneManager.getObject(gameTimeTextID);
		if (objPair.first != SceneClass::INVALID) {
			if (auto ui = objPair.second) {
				// TODO this a bit too convoluted -> just store type in GameObject variable when creating a subclass (enum Type) and check it before static casting + remove the distinctions in sceneManager
				if (auto text = static_cast<TextComponent*>(ui)) {
					text->setText(fmt::format("Time: {:02}:{:02}", newSecond / 60, newSecond % 60));
				}
			}
		}
		oldSecond = newSecond;
	}

	if (sceneManager.realTime >= 10.0f && newSecond % 10 == 0 && newSecond != lastSpawnSecond) {
		printf("Spawning new enemy wave at %d seconds\n", newSecond);
		lastSpawnSecond = newSecond;
		float enemyHullHeight = 1.5f;
		float enemyRadius = 0.3f;
		JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));
		float enemySpawnMinRadius = 20.0f;
		float enemySpawnMaxRadius = 70.0f;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> angleDist(
			0.0f, 2.0f * glm::pi<float>());
		std::uniform_real_distribution<float> radiusSqDist(
			enemySpawnMinRadius * enemySpawnMinRadius,
			enemySpawnMaxRadius * enemySpawnMaxRadius);	 // squared to have density distribution uniformly in spawn ring

		Ref<Shape> enemyShape = enemyShapeSettings.Create().Get();
		physics::Sprinter::SprinterSettings sprinterSettings = {};
		sprinterSettings.model = enemyModel;
		sprinterSettings.maxMovementSpeed = 10.0f + static_cast<float>(newSecond / 60);
		sprinterSettings.turnSpeed = 0.5f + static_cast<float>(newSecond / 60) * 0.2f;
		sprinterSettings.accelerationToMaxSpeed = 1.0f + static_cast<float>(newSecond / 60) * 0.2f;

		JPH::CharacterSettings enemyCharacterSettings = {};
		enemyCharacterSettings.mLayer = physics::Layers::MOVING;
		enemyCharacterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -enemyRadius);	 // accept contacts that touch the lower sphere of the capsule
		enemyCharacterSettings.mFriction = 1.0f;
		enemyCharacterSettings.mShape = enemyShape;
		enemyCharacterSettings.mGravityFactor = 1.0f;

		physics::Sprinter::SprinterCreationSettings sprinterCreationSettings = {};
		sprinterCreationSettings.sprinterSettings = sprinterSettings;
		sprinterCreationSettings.characterSettings = enemyCharacterSettings;

		auto playerPos = sceneManager.getPlayer()->getPosition();

		for (int i = 0; i < 10; ++i) {
			float angle = angleDist(gen);
			float radius = std::sqrt(radiusSqDist(gen));
			sprinterCreationSettings.position = RVec3(playerPos.x + std::cos(angle) * radius, 15.0, playerPos.z + std::sin(angle) * radius);

			std::unique_ptr<physics::Enemy> enemy = std::make_unique<physics::Sprinter>(sprinterCreationSettings, physicsSimulation.getPhysicsSystem());
			enemy->awake(); // for sound effects

			sceneManager.addEnemy(std::move(enemy));
		}
	}

	sceneManager.updateEnemyVisuals(deltaTime);
	
	auto player = sceneManager.getPlayer();
	auto sun = sceneManager.getSun();

	// rotate sun direction around Y axis
	const float rotationSpeed = 0.01f; // radians per second
	sunRotationAngle += rotationSpeed * deltaTime;
	
	if (sunRotationAngle > 2.0f * glm::pi<float>()) {
		sunRotationAngle -= 2.0f * glm::pi<float>();
	}
	
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), sunRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 rotatedDir = rotationMatrix * glm::vec4(baseSunDirection, 0.0f);
	glm::vec3 newDir = glm::normalize(glm::vec3(rotatedDir));
	
	sun->setDirection(newDir);
	
	const float sunDistance = 100.0f;
	glm::vec3 playerPos = player->getPosition();
	glm::vec3 sunPos = playerPos - newDir * sunDistance;
	sun->setPosition(sunPos);

	// Update health text
	if (isDebugActive) {
		// In debug mode, we don't update the health text
		return;
	}

	if (player) {
		auto objPair = sceneManager.getObject(gameHealthTextID);
		if (objPair.first != SceneClass::INVALID) {
			if (auto ui = objPair.second) {
				if (auto text = static_cast<TextComponent*>(ui)) {
					float health = player->getCurrentHealth();
					text->setText(fmt::format("Health: {:.0f}%", health));
				}
			}
		}
	}
}

void Swarm::prePhysicsUpdate() {
	SceneManager& sceneManager = SceneManager::getInstance();

	if (!isDebugActive) {
		Player* player = sceneManager.getPlayer();
		if (player && player->isPhysicsPlayer()) {
			physics::PhysicsPlayer* physicsPlayer = static_cast<physics::PhysicsPlayer*>(player);
			physicsPlayer->handleMovement(physicsSimulation.cPhysicsDeltaTime);
			physicsPlayer->updateGrenadeCooldown(physicsSimulation.cPhysicsDeltaTime);
		}
	}

	// TODO hook an event manager and call update on all methods that are registered (objects register methods like with input polling but in a separate event manager -> also updates timers stored in sceneManager every frame)
	sceneManager.updateEnemyPhysics(physicsSimulation.cPhysicsDeltaTime);
	sceneManager.updatePhysicsEntities(physicsSimulation.cPhysicsDeltaTime);
}

void Swarm::postPhysicsUpdate() {}

void Swarm::gamePauseUpdate(float deltaTime) {
	// Update tree parameter tuning system even when paused
	// This allows real-time parameter visualization when escape menu is open
}

void Swarm::postRenderingUpdate(EngineStats engineStats, float deltaTime) {
	SceneManager& sceneManager = SceneManager::getInstance();

	auto objPair = sceneManager.getObject(renderedObjectsTextID);
	if (objPair.first != SceneClass::INVALID) {
		if (auto ui = objPair.second) {
			if (auto text = static_cast<TextComponent*>(ui)) {
				text->setText(fmt::format("rendered: {:d}", engineStats.renderedGameObjects));
			}
		}
	}
}

void Swarm::toggleCulling() {
	this->renderSystemSettings.enableFrustumCulling = !this->renderSystemSettings.enableFrustumCulling;
	
	if (this->renderSystemSettings.enableFrustumCulling) {
		std::cout << "Culling enabled" << std::endl;
	}
	else {
		std::cout << "Culling disabled" << std::endl;
	}
}