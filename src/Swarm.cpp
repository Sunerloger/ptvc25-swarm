#include "Swarm.h"


Swarm::Swarm(physics::PhysicsSimulation& physicsSimulation, std::shared_ptr<SceneManager> sceneManager, AssetManager& assetManager, Window& window, Device& device, controls::KeyboardMovementController& movementController)
	: physicsSimulation(physicsSimulation), sceneManager(sceneManager), assetManager(assetManager), window(window), device(device), movementController(movementController) {}

void Swarm::init() {

	// TODO be careful that nothing important falls out of scope
	// TODO remove pointers -> references

	// Parameters for the terrain
	int samplesPerSide = 200;	// Resolution of the heightmap
	float noiseScale = 30.0f;	// Controls the "frequency" of the noise
	float heightScale = 10.0f;	// Controls the height of the terrain

	// Generate terrain model with heightmap
	auto result = vk::Model::createTerrainModel(
		device,
		samplesPerSide,
		"textures:ground/dirt.png",	 // Tile texture path
		noiseScale,
		heightScale);

	float playerHeight = 1.40f;
	float playerRadius = 0.3f;
	Ref<Shape> characterShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * playerHeight + playerRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * playerHeight, playerRadius)).Create().Get();

	std::unique_ptr<CharacterCameraSettings> cameraSettings = std::make_unique<CharacterCameraSettings>();
	cameraSettings->cameraOffsetFromCharacter = glm::vec3(0.0f, playerHeight + playerRadius, 0.0f);

	std::unique_ptr<physics::PlayerSettings> playerSettings = std::make_unique<physics::PlayerSettings>();
	playerSettings->movementSpeed = 10.0f;

	// TODO remove all the unnecessary moves

	std::unique_ptr<JPH::CharacterSettings> characterSettings = std::make_unique<JPH::CharacterSettings>();
	characterSettings->mGravityFactor = 1.0f;
	characterSettings->mFriction = 10.0f;
	characterSettings->mShape = characterShape;
	characterSettings->mLayer = physics::Layers::MOVING;
	characterSettings->mSupportingVolume = Plane(Vec3::sAxisY(), -playerRadius);  // Accept contacts that touch the lower sphere of the capsule

	std::unique_ptr<physics::PlayerCreationSettings> playerCreationSettings = std::make_unique<physics::PlayerCreationSettings>();
	playerCreationSettings->characterSettings = std::move(characterSettings);
	playerCreationSettings->cameraSettings = std::move(cameraSettings);
	playerCreationSettings->playerSettings = std::move(playerSettings);
	playerCreationSettings->position = JPH::RVec3(0.0f, 15.0f, 0.0f);  // Increased Y position to start higher above terrain

	sceneManager->setPlayer(std::make_unique<physics::Player>(std::move(playerCreationSettings), physicsSimulation.getPhysicsSystem()));

	// Terrain
	// Create a terrain with procedural heightmap using Perlin noise
	// Parameters: physics_system, color, model, position, scale, samplesPerSide, noiseScale, heightScale
	// Create a terrain with our generated heightmap data
	auto terrain = std::make_unique<physics::Terrain>(
		physicsSimulation.getPhysicsSystem(),
		glm::vec3{ 0.569, 0.29, 0 },
		std::move(result.first),				 // Move the model
		glm::vec3{ 0.0, -2.0, 0.0 },				 // Position slightly below origin to prevent falling through
		glm::vec3{ 500.0f, heightScale, 500.0f },	 // Larger size and taller
		result.second);
	sceneManager->addTessellationObject(std::move(terrain));

	// Skybox
	std::array<std::string, 6> cubemapFaces = {
		"textures:skybox/learnopengl/right.jpg",
		"textures:skybox/learnopengl/left.jpg",
		"textures:skybox/learnopengl/top.jpg",
		"textures:skybox/learnopengl/bottom.jpg",
		"textures:skybox/learnopengl/front.jpg",
		"textures:skybox/learnopengl/back.jpg" };
	auto skybox = std::make_unique<Skybox>(device, cubemapFaces);
	sceneManager->addSpectralObject(std::move(skybox));

	// Enemies
	float enemyHullHeight = 1.25f;
	float enemyRadius = 0.3f;
	JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));
	shared_ptr<Model> enemyModel = Model::createModelFromFile(device, "models:CesiumMan.glb");
	for (int i = 0; i < 15; ++i) {
		Ref<Shape> enemyShape = enemyShapeSettings.Create().Get();
		std::unique_ptr<physics::SprinterSettings> sprinterSettings = std::make_unique<physics::SprinterSettings>();
		sprinterSettings->model = enemyModel;

		std::unique_ptr<JPH::CharacterSettings> enemyCharacterSettings = std::make_unique<JPH::CharacterSettings>();
		enemyCharacterSettings->mLayer = physics::Layers::MOVING;
		enemyCharacterSettings->mSupportingVolume = Plane(Vec3::sAxisY(), -enemyRadius);  // Accept contacts that touch the lower sphere of the capsule
		enemyCharacterSettings->mFriction = 10.0f;
		enemyCharacterSettings->mShape = enemyShape;
		enemyCharacterSettings->mGravityFactor = 1.0f;

		std::unique_ptr<physics::SprinterCreationSettings> sprinterCreationSettings = std::make_unique<physics::SprinterCreationSettings>();
		sprinterCreationSettings->sprinterSettings = std::move(sprinterSettings);
		sprinterCreationSettings->characterSettings = std::move(enemyCharacterSettings);
		sprinterCreationSettings->position = RVec3(i + 10.0f, 15.0f, 10.0f);
		sceneManager->addEnemy(std::move(make_unique<physics::Sprinter>(std::move(sprinterCreationSettings), physicsSimulation.getPhysicsSystem())));
	}

	// UI
	UIComponentCreationSettings hudSettings{};
	hudSettings.model = Model::createModelFromFile(device, "models:gray_quad.glb", true);
	hudSettings.name = "gray_quad";
	hudSettings.controllable = false;
	sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));

	hudSettings.model = Model::createModelFromFile(device, "models:DamagedHelmet.glb", true);
	hudSettings.name = "damaged_helmet";
	hudSettings.controllable = false;
	sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));

	hudSettings.model = Model::createModelFromFile(device, "models:USPS.glb", true);
	hudSettings.name = "usps";
	hudSettings.controllable = true;
	hudSettings.window = window.getGLFWWindow();
	hudSettings.anchorRight = true;
	hudSettings.anchorBottom = true;
	sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));

	Font font;
	TextComponent* gameTimeText = new TextComponent(device, font, "Time: 0", "clock", false);
	gameTimeTextID = sceneManager->addUIObject(std::unique_ptr<UIComponent>(gameTimeText));

	// TODO handle clicking (raycast + damage) -> register callback in player with input system
}

void Swarm::gameActiveUpdate(float deltaTime) {

	int fbWidth, fbHeight;
	window.getFramebufferSize(fbWidth, fbHeight);
	float windowWidth = static_cast<float>(fbWidth);
	float windowHeight = static_cast<float>(fbHeight);

	// TODO input logic should be created by game and not the input controller itself
	movementController.handleRotation(window.getGLFWWindow(), *sceneManager->getPlayer());

	// TODO refactor into timer class
	elapsedTime += deltaTime;
	int newSecond = static_cast<int>(elapsedTime);

	if (newSecond > oldSecond) {
		if (auto objPair = sceneManager->getObject(gameTimeTextID)) {
			if (auto ui = objPair->second.lock()) {
				// TODO this is unsafe and terrible -> just store type in GameObject variable when creating a subclass (enum Type) and check it before static casting + remove the distinctions in sceneManager
				if (auto text = static_cast<TextComponent*>(ui.get())) {
					text->setText("Time: " + std::to_string(newSecond));
				}
			}
		}
		oldSecond = newSecond;
	}
}

void Swarm::prePhysicsUpdate() {

	MovementIntent movementIntent = movementController.getMovementIntent(window.getGLFWWindow());

	shared_ptr<physics::Player> player = sceneManager->getPlayer();

	JPH::Vec3 movementDirection = GLMToRVec3(movementIntent.direction);

	// only update if something happened
	// TODO this should be in game logic, not directly in physics system + shooting too
	if (movementDirection != JPH::Vec3{ 0,0,0 } || movementIntent.jump) {
		player->handleMovement(movementDirection, movementIntent.jump, physicsSimulation.cPhysicsDeltaTime);
	}

	// TODO hook an event manager and call update on all methods that are registered (objects register methods that should be called here)
	sceneManager->updateEnemies(physicsSimulation.cPhysicsDeltaTime);
}

void Swarm::postPhysicsUpdate() {}

void Swarm::gamePauseUpdate(float deltaTime) {}