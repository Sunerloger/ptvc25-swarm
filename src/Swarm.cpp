#include "Swarm.h"

#include "scene/SceneManager.h"

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

	swarmInput.onToggleWireframeMode = [this, &sceneManager]() {
		sceneManager.toggleWireframeOnTessellationObjects();
		sceneManager.toggleWireframeOnWaterObjects();
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

	audio::AudioSystem::getInstance().loadSound("gun", "audio:gun_shot.mp3");

	SceneManager& sceneManager = SceneManager::getInstance();

	// register assets that are reused later with asset manager so they don't fall out of scope and can still be referenced

	float maxTerrainHeight = 30.0f;

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
		playerCreationSettings.position = JPH::RVec3(0.0f, maxTerrainHeight + 1, 0.0f);  // Increased Y position to start higher above terrain

		sceneManager.setPlayer(std::make_unique<physics::PhysicsPlayer>(playerCreationSettings, physicsSimulation.getPhysicsSystem()));

		sceneManager.setSun(make_unique<lighting::Sun>(glm::vec3(0.0f), glm::vec3(1.7, -1, 3.0), glm::vec3(1.0f, 1.0f, 1.0f)));
	}

	// Terrain
	{
		int samplesPerSide = 100;  // Resolution of the heightmap
		float noiseScale = 5.0f;   // Controls the "frequency" of the noise

		TessellationMaterial::MaterialCreationData terrainCreationData = {};
		terrainCreationData.textureRepetition = glm::vec2(samplesPerSide / 2.0f, samplesPerSide / 2.0f);
		terrainCreationData.heightScale = maxTerrainHeight;
		terrainCreationData.ka = 0.0f;
		terrainCreationData.kd = 1.0f;
		terrainCreationData.ks = 0.0f;
		terrainCreationData.alpha = 10.0f;

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

		// create terrain with procedural heightmap using perlin noise
		// create terrain with the generated heightmap data
		auto terrain = std::make_unique<physics::Terrain>(
			physicsSimulation.getPhysicsSystem(),
			std::move(result.first),
			glm::vec3{0.0, -2.0, 0.0},	// position slightly below origin to prevent falling through
			glm::vec3{100.0f, maxTerrainHeight, 100.0f},
			std::move(result.second));
		sceneManager.addTessellationObject(std::move(terrain));
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

			sprinterCreationSettings.position = RVec3(playerPos.x + std::cos(angle) * radius, maxTerrainHeight+1, playerPos.z + std::sin(angle) * radius);

			sceneManager.addEnemy(std::make_unique<physics::Sprinter>(sprinterCreationSettings, physicsSimulation.getPhysicsSystem()));
		}
	}

	// Water
	{
		int samplesPerSide = 2000;
		std::shared_ptr<Model> waterModel = std::shared_ptr<Model>(Model::createGridModelWithoutGeometry(device, samplesPerSide));

		auto waterMaterial = std::make_shared<WaterMaterial>(device, "textures:water.png");

		CreateWaterData waterData{};
		waterData.minTessDistance = 50.0f;
		waterData.maxTessDistance = 500.0f;
		waterData.textureRepetition = glm::vec2(samplesPerSide - 1.0f, samplesPerSide - 1.0f);
		waterMaterial->setWaterData(waterData);

		std::vector<glm::vec4> waves;
		// waves.push_back(glm::vec4{ 1.0f, 0.0f, 0.18f, 12.0f });
		// waves.push_back(glm::vec4{ 0.92f, 0.38f, 0.15f, 8.0f });
		// waves.push_back(glm::vec4{ -0.75f, 0.66f, 0.20f, 20.0f });
		// waves.push_back(glm::vec4{ 0.34f, -0.94f, 0.06f, 16.0f });

		waves.push_back(glm::vec4{ 1.0f, 1.0f, 0.25f, 60.0f });
		waves.push_back(glm::vec4{ 1.0f, 0.6f, 0.25f, 31.0f });
		waves.push_back(glm::vec4{ 1.0f, 1.3f, 0.25f, 18.0f });
		waterMaterial->setWaves(waves);
		
		waterModel->setMaterial(waterMaterial);

		WaterObject::WaterCreationSettings waterCreationSettings = {};
		waterCreationSettings.position = glm::vec3{ 0.0f, -20.0f, 0.0f };
		waterCreationSettings.waterScale = samplesPerSide - 1;
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
			/* verticalOffset: */ 100.0f,
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
			/* verticalOffset: */ 000.0f,
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
			/* verticalOffset: */ -100.0f,
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