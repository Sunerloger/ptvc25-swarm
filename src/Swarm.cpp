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

	isDebugActive = !isDebugActive;
}

void Swarm::onPlayerDeath() {
	input::SwarmInputController& swarmInput = static_cast<input::SwarmInputController&>(inputController);
	swarmInput.setContext(input::SwarmInputController::ContextID::Death);

	Font font;
	TextComponent* deathText = new TextComponent(device, font, "You died.", "deathText", false);
	SceneManager::getInstance().addUIObject(std::unique_ptr<UIComponent>(deathText));
}

void Swarm::init() {
	SceneManager& sceneManager = SceneManager::getInstance();

	// register assets that are reused later with asset manager so they don't fall out of scope and can still be referenced

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

	// Parameters for the terrain
	int samplesPerSide = 100;	// Resolution of the heightmap
	float noiseScale = 5.0f;	// Controls the "frequency" of the noise
	float heightScale = 15.0f;	// Controls the height of the terrain

	// Generate terrain model with heightmap
	auto result = vk::Model::createTerrainModel(
		device,
		samplesPerSide,
		"textures:ground/dirt.png",	 // Tile texture path
		noiseScale,
		heightScale);

	// create terrain with procedural heightmap using perlin noise
	// create terrain with the generated heightmap data
	auto terrain = std::make_unique<physics::Terrain>(
		physicsSimulation.getPhysicsSystem(),
		glm::vec3{0.569, 0.29, 0},
		std::move(result.first),
		glm::vec3{0.0, -2.0, 0.0},	// position slightly below origin to prevent falling through
		glm::vec3{100.0f, heightScale, 100.0f},
		std::move(result.second));
	sceneManager.addTessellationObject(std::move(terrain));

	// Skybox
	std::array<std::string, 6> cubemapFaces = {
		"textures:skybox/learnopengl/right.jpg",
		"textures:skybox/learnopengl/left.jpg",
		"textures:skybox/learnopengl/top.jpg",
		"textures:skybox/learnopengl/bottom.jpg",
		"textures:skybox/learnopengl/front.jpg",
		"textures:skybox/learnopengl/back.jpg"};
	sceneManager.addSpectralObject(std::make_unique<Skybox>(device, cubemapFaces));

	// Enemies
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

		sprinterCreationSettings.position = RVec3(playerPos.x + std::cos(angle) * radius, heightScale, playerPos.z + std::sin(angle) * radius);

		sceneManager.addEnemy(std::make_unique<physics::Sprinter>(sprinterCreationSettings, physicsSimulation.getPhysicsSystem()));
	}

	// Water
	std::shared_ptr<Model> waterModel = std::shared_ptr<Model>(Model::createGridModel(device, 1000));

	auto waterMaterial = std::make_shared<WaterMaterial>(device, "textures:water.png");
	waterModel->setMaterial(waterMaterial);

	WaterObject::WaterCreationSettings waterCreationSettings = {};
	sceneManager.addWaterObject(std::make_unique<WaterObject>(waterModel, waterCreationSettings));

	// UI
	UIComponentCreationSettings hudSettings{};
	hudSettings.model = Model::createModelFromFile(device, "models:gray_quad.glb", true);
	hudSettings.name = "gray_quad";
	hudSettings.controllable = false;
	sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

	hudSettings.model = Model::createModelFromFile(device, "models:DamagedHelmet.glb", true);
	hudSettings.name = "damaged_helmet";
	hudSettings.controllable = false;
	sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

	hudSettings.model = Model::createModelFromFile(device, "models:USPS.glb", true);
	hudSettings.name = "usps";
	hudSettings.controllable = false;
	hudSettings.window = window.getGLFWWindow();
	hudSettings.anchorRight = true;
	hudSettings.anchorBottom = true;
	sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

	hudSettings.model = Model::createModelFromFile(device, "models:red_crosshair.glb", true);
	hudSettings.name = "red_crosshair";
	hudSettings.controllable = true;
	hudSettings.window = window.getGLFWWindow();
	hudSettings.anchorRight = false;
	hudSettings.anchorBottom = false;
	hudSettings.placeInMiddle = true;
	sceneManager.addUIObject(std::make_unique<UIComponent>(hudSettings));

	Font font;
	TextComponent* gameTimeText = new TextComponent(device, font, "Time: 00:00", "clock", false);
	gameTimeTextID = sceneManager.addUIObject(std::unique_ptr<UIComponent>(gameTimeText));
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