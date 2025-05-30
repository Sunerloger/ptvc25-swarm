#include "Swarm.h"

#include "scene/SceneManager.h"
#include "procedural/VegetationIntegrator.h"
#include "procedural/VegetationSharedResources.h"

#include <fmt/format.h>
#include <random>

Swarm::Swarm(physics::PhysicsSimulation& physicsSimulation, AssetManager& assetManager, Window& window, Device& device, input::SwarmInputController& inputController, bool debugMode)
	: GameBase(inputController), physicsSimulation(physicsSimulation), assetManager(assetManager), window(window), device(device), debugMode(debugMode) {}

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
	SceneManager& sceneManager = SceneManager::getInstance();

	// register assets that are reused later with asset manager so they don't fall out of scope and can still be referenced

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
		playerCreationSettings.position = JPH::RVec3(0.0f, 15.0f, 0.0f);  // Increased Y position to start higher above terrain

		sceneManager.setPlayer(std::make_unique<physics::PhysicsPlayer>(playerCreationSettings, physicsSimulation.getPhysicsSystem()));

		sceneManager.setSun(make_unique<lighting::Sun>(glm::vec3(0.0f), glm::vec3(1.7, -1, 3.0), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f));
	}

	// Terrain
	float maxTerrainHeight = 15.0f;	 // Controls the height of the terrain
	int samplesPerSide = 100;		 // Resolution of the heightmap - moved out for vegetation use
	{
		float noiseScale = 5.0f;  // Controls the "frequency" of the noise

		// Generate terrain model with heightmap
		auto result = vk::Model::createTerrainModel(
			device,
			samplesPerSide,
			"textures:ground/dirt.png",	 // Tile texture path
			noiseScale,
			maxTerrainHeight);

		// Store heightfield data for vegetation
		std::vector<float> heightfieldData;
		if (result.second.size() >= samplesPerSide * samplesPerSide) {
			heightfieldData = result.second;  // Copy the heightfield data
		} else {
			// Create flat heightfield if result doesn't have enough data
			heightfieldData.resize(samplesPerSide * samplesPerSide, 0.0f);
		}

		// create terrain with procedural heightmap using perlin noise
		// create terrain with the generated heightmap data
		auto terrain = std::make_unique<physics::Terrain>(
			physicsSimulation.getPhysicsSystem(),
			glm::vec3{0.569, 0.29, 0},
			std::move(result.first),
			glm::vec3{0.0, -2.0, 0.0},	// position slightly below origin to prevent falling through
			glm::vec3{100.0f, maxTerrainHeight, 100.0f},
			std::move(result.second));
		sceneManager.addTessellationObject(std::move(terrain));

		// Create shared vegetation resources to avoid descriptor pool exhaustion
		auto sharedResources = std::make_shared<procedural::VegetationSharedResources>(device);

		// Vegetation (L-Systems) - moved inside terrain block to access heightfield data
		{
			procedural::VegetationIntegrator vegetationIntegrator(device);

			// Configure vegetation settings with optimized density for ferns only
			procedural::VegetationIntegrator::VegetationSettings vegSettings;
			vegSettings.fernDensity = 0.0008f;	// Low density for ferns to avoid resource exhaustion            // Set terrain bounds to match our terrain
			vegSettings.terrainMin = glm::vec2(-70.0f, -70.0f);
			vegSettings.terrainMax = glm::vec2(70.0f, 70.0f);

			// Increase fern density for more plants
			vegSettings.fernDensity = 0.002f;  // Increased density for more ferns

			// Slope constraints for realistic placement
			vegSettings.maxBushSlope = 30.0f;

			// Scale variation for much larger, more impressive ferns
			vegSettings.fernScaleRange = glm::vec2(1.2f, 2.5f);	 // Much bigger ferns for better visibility

			// Use random seed for deterministic vegetation
			vegSettings.placementSeed = 12345;	// Different seed for new patterns

			try {
				// Generate vegetation on terrain using the heightfield data
				vegetationIntegrator.generateVegetationOnTerrain(
					vegSettings,
					heightfieldData,
					samplesPerSide,
					glm::vec3(100.0f, maxTerrainHeight, 100.0f),
					glm::vec3(0.0, -2.0, 0.0));

				// Add generated vegetation to scene
				vegetationIntegrator.addVegetationToScene(sceneManager);

				// Report statistics
				auto stats = vegetationIntegrator.getVegetationStats();
				printf("Added L-System vegetation: %d ferns\n", stats.fernCount);

			} catch (const std::exception& e) {
				printf("Error generating vegetation: %s\n", e.what());
			}
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
		float enemyHullHeight = 1.25f;
		float enemyRadius = 0.3f;
		JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));
		shared_ptr<Model> enemyModel = Model::createModelFromFile(device, "models:CesiumMan.glb");
		float enemySpawnMinRadius = 20.0f;
		float enemySpawnMaxRadius = 70.0f;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> angleDist(
			0.0f, 2.0f * glm::pi<float>());
		std::uniform_real_distribution<float> radiusSqDist(
			enemySpawnMinRadius * enemySpawnMinRadius,
			enemySpawnMaxRadius * enemySpawnMaxRadius);	 // squared to have density distribution uniformly in spawn ring

		for (int i = 0; i < 100; ++i) {
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

			float angle = angleDist(gen);
			float radius = std::sqrt(radiusSqDist(gen));
			auto playerPos = sceneManager.getPlayer()->getPosition();

			sprinterCreationSettings.position = RVec3(playerPos.x + std::cos(angle) * radius, maxTerrainHeight, playerPos.z + std::sin(angle) * radius);

			sceneManager.addEnemy(std::make_unique<physics::Sprinter>(sprinterCreationSettings, physicsSimulation.getPhysicsSystem()));
		}
	}

	// Water
	{
		std::shared_ptr<Model> waterModel = std::shared_ptr<Model>(Model::createGridModel(device, 1000));

		auto waterMaterial = std::make_shared<WaterMaterial>(device, "textures:water.png");
		waterModel->setMaterial(waterMaterial);

		WaterObject::WaterCreationSettings waterCreationSettings = {};
		sceneManager.addWaterObject(std::make_unique<WaterObject>(waterModel, waterCreationSettings));
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
			/* verticalOffset: */ 100.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ true,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f11));

		// F12: Toggle Debug Mode
		TextComponent* debug_text_f12 = new TextComponent(
			device,
			font,
			"F12: Toggle \n Debug Mode",
			"debug_text_toggle_menu",
			/* controllable: */ false,
			/* centerHorizontal: */ false,
			/* horizontalOffset: */ 0.0f,
			/* centerVertical:   */ true,
			/* verticalOffset: */ 000.0f,
			/* anchorRight: */ true,
			/* anchorBottom: */ false,
			/* isDebugMenuComponent: */ false,
			window.getGLFWWindow());
		sceneManager.addUIObject(std::unique_ptr<UIComponent>(debug_text_f12));

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

		// USPS
		hudSettings.model = Model::createModelFromFile(device, "models:USPS.glb", true);
		hudSettings.name = "usps";
		hudSettings.controllable = false;
		hudSettings.anchorRight = true;
		hudSettings.anchorBottom = true;
		hudSettings.centerHorizontal = false;
		hudSettings.centerVertical = false;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

		// Crosshair
		hudSettings.model = Model::createModelFromFile(device, "models:crosshair.glb", true);
		hudSettings.name = "crosshair";
		hudSettings.controllable = false;
		hudSettings.anchorRight = false;
		hudSettings.anchorBottom = false;
		hudSettings.centerHorizontal = true;
		hudSettings.centerVertical = true;
		sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));
	}
}

void Swarm::gameActiveUpdate(float deltaTime) {
	SceneManager& sceneManager = SceneManager::getInstance();

	// TODO refactor into timer class
	elapsedTime += deltaTime;
	int newSecond = static_cast<int>(elapsedTime);

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

	sceneManager.updateEnemyVisuals(deltaTime);

	// Update health text
	if (isDebugActive) {
		// In debug mode, we don't update the health text
		return;
	}
	auto player = sceneManager.getPlayer();
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
			static_cast<physics::PhysicsPlayer*>(player)->handleMovement(physicsSimulation.cPhysicsDeltaTime);
		}
	}

	// TODO hook an event manager and call update on all methods that are registered (objects register methods like with input polling but in a separate event manager -> also updates timers stored in sceneManager every frame)
	sceneManager.updateEnemyPhysics(physicsSimulation.cPhysicsDeltaTime);
}

void Swarm::postPhysicsUpdate() {}

void Swarm::gamePauseUpdate(float deltaTime) {}