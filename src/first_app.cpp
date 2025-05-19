#include "first_app.h"

namespace vk {

	FirstApp::FirstApp() {
		window = std::make_unique<Window>(applicationSettings.windowWidth, applicationSettings.windowHeight, "Swarm");
		menuController = std::make_unique<controls::KeyboardMenuController>(window->getGLFWWindow());
		device = std::make_unique<Device>(*window);
		renderer = std::make_unique<Renderer>(*window, *device);
		menuController->setConfigChangeCallback([this]() {
			int w, h;
			window->getFramebufferSize(w, h);
			renderer->recreateSwapChain();
		});

		globalPool = DescriptorPool::Builder(*device)
						 .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .build();

		this->sceneManager = make_shared<SceneManager>();
		this->physicsSimulation = make_unique<physics::PhysicsSimulation>(this->sceneManager, engineSettings.cPhysicsDeltaTime);

		loadGameObjects();
	}

	FirstApp::~FirstApp() {}

	glm::vec3 loadData() {
		auto iniPath = std::filesystem::current_path() / "sun.ini";
		INIReader reader(iniPath.string());

		glm::vec3 dir;
		std::string section = "Sun";

		// defaults
		dir = {0.0f, -1.0f, 0.0f};

		if (reader.ParseError() < 0)
			return dir;
		if (reader.Get(section, "dir", "") == "")
			return dir;

		{
			std::stringstream ss(reader.Get(section, "dir", ""));
			char c;
			ss >> dir.x >> c >> dir.y >> c >> dir.z;
		}
		return dir;
	}

	void FirstApp::run() {
		std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<Buffer>(*device,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = DescriptorSetLayout::Builder(*device)
								   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
								   .build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		TextureRenderSystem textureRenderSystem{*device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};
		WaterRenderSystem waterRenderSystem{*device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};
		TessellationRenderSystem tessellationRenderSystem{*device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};
		UIRenderSystem uiRenderSystem{*device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};

		glfwSetInputMode(window->getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		controls::KeyboardMovementController movementController{applicationSettings.windowWidth, applicationSettings.windowHeight};
		controls::KeyboardPlacementController placementController;

		// TODO: update every second
		Font font;
		TextComponent* clockText = new TextComponent(*device, font, "Time: 0", "clock", false);
		sceneManager->addUIObject(std::unique_ptr<UIComponent>(clockText));

		auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = startTime;
		int currentSecond = 0;
		float gameTimer = 0;
		float physicsTimeAccumulator = 0.0f;

		int fbWidth, fbHeight;
		window->getFramebufferSize(fbWidth, fbHeight);
		float windowWidth = static_cast<float>(fbWidth);
		float windowHeight = static_cast<float>(fbHeight);

		while (!window->shouldClose()) {
			auto newTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			deltaTime = std::min(deltaTime, engineSettings.maxFrameTime);

			glfwPollEvents();

			int placementTransform = placementController.updateModelMatrix(window->getGLFWWindow());
			sceneManager->updateUITransforms(deltaTime, placementTransform);

			if (!menuController->isMenuOpen()) {
				// Time
				physicsTimeAccumulator += deltaTime;
				gameTimer += deltaTime;
				int newSecond = static_cast<int>(gameTimer);
				if (engineSettings.debugTime && newSecond > currentSecond) {
					currentSecond = newSecond;
					std::cout << "Time since start: " << currentSecond << "s" << std::endl;
				}

				// Movement
				movementController.handleRotation(window->getGLFWWindow(), *sceneManager->getPlayer());
				MovementIntent movementIntent = movementController.getMovementIntent(window->getGLFWWindow());
				while (physicsTimeAccumulator >= engineSettings.cPhysicsDeltaTime) {
					physicsSimulation->preSimulation(movementIntent);
					physicsSimulation->simulate();
					physicsSimulation->postSimulation(engineSettings.debugPlayer, engineSettings.debugEnemies);
					physicsTimeAccumulator -= engineSettings.cPhysicsDeltaTime;
				}
			}

			// Camera
			float aspect = renderer->getAspectRatio();
			sceneManager->getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

			if (auto commandBuffer = renderer->beginFrame()) {
				int frameIndex = renderer->getFrameIndex();
				FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex], *sceneManager};

				// Update global uniform buffer
				GlobalUbo ubo{};
				ubo.projection = sceneManager->getPlayer()->getProjMat();
				ubo.view = sceneManager->getPlayer()->calculateViewMat();
				ubo.uiOrthographicProjection = CharacterCamera::getOrthographicProjection(0, windowWidth, 0, windowHeight, 0.1f, 500.0f);
				ubo.sunDirection = glm::vec4(1.7, -1, 3.0, 0.0f);
				ubo.sunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				renderer->beginSwapChainRenderPass(commandBuffer);
				textureRenderSystem.renderGameObjects(frameInfo);
				waterRenderSystem.renderGameObjects(frameInfo);
				tessellationRenderSystem.renderGameObjects(frameInfo);

				VkClearAttachment clearAttachment{};
				clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				clearAttachment.clearValue.depthStencil = {/* depth = */ 1.0f, /* stencil = */ 0};
				VkClearRect clearRect{};
				clearRect.rect.offset = {0, 0};
				clearRect.rect.extent = {
					static_cast<uint32_t>(windowWidth),
					static_cast<uint32_t>(windowHeight)};
				clearRect.baseArrayLayer = 0;
				clearRect.layerCount = 1;

				vkCmdClearAttachments(
					frameInfo.commandBuffer,
					1,
					&clearAttachment,
					1,
					&clearRect);

				uiRenderSystem.renderGameObjects(frameInfo);
				renderer->endSwapChainRenderPass(commandBuffer);
				renderer->endFrame();
			}

			int fbWidth2, fbHeight2;
			window->getFramebufferSize(fbWidth2, fbHeight2);
			float windowWidth2 = static_cast<float>(fbWidth2);
			float windowHeight2 = static_cast<float>(fbHeight2);
			if (windowWidth != windowWidth2 || windowHeight != windowHeight2) {
				windowWidth = windowWidth2;
				windowHeight = windowHeight2;
				renderer->recreateSwapChain();
			}

			// TODO use fences / semaphores instead (next line forces sync of cpu and gpu and heavily impacts performance):
			vkDeviceWaitIdle(device->device());
		}
	}

	void FirstApp::loadGameObjects() {
		// Terrain
		{
			int samplesPerSide = 200;	// Resolution of the heightmap
			float noiseScale = 30.0f;	// Controls the "frequency" of the noise
			float heightScale = 10.0f;	// Controls the height of the terrain

			// Generate terrain model with heightmap
			auto result = Model::createTerrainModel(
				*device,
				samplesPerSide,
				"textures:ground/dirt.png",	 // Tile texture path
				noiseScale,
				heightScale);

			// Extract the model and height data
			auto& heightData = result.second;

			// Create a terrain with procedural heightmap using Perlin noise
			// Parameters: physics_system, color, model, position, scale, samplesPerSide, noiseScale, heightScale
			// Create a terrain with our generated heightmap data
			auto terrain = std::make_unique<physics::Terrain>(
				physicsSimulation->getPhysicsSystem(),
				glm::vec3{0.569, 0.29, 0},
				std::move(result.first),				 // Move the model
				glm::vec3{0.0, -2.0, 0.0},				 // Position slightly below origin to prevent falling through
				glm::vec3{500.0f, heightScale, 500.0f},	 // Larger size and taller
				heightData);
			sceneManager->addTessellationObject(std::move(terrain));
		}

		// Player
		{
			float playerHeight = 1.40f;
			float playerRadius = 0.3f;
			Ref<Shape> characterShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * playerHeight + playerRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * playerHeight, playerRadius)).Create().Get();

			std::unique_ptr<CharacterCameraSettings> cameraSettings = std::make_unique<CharacterCameraSettings>();
			cameraSettings->cameraOffsetFromCharacter = glm::vec3(0.0f, playerHeight + playerRadius, 0.0f);

			std::unique_ptr<physics::PlayerSettings> playerSettings = std::make_unique<physics::PlayerSettings>();
			playerSettings->movementSpeed = 100.0f;

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
			playerCreationSettings->position = JPH::RVec3(0.0f, 10.0f, 0.0f);  // Increased Y position to start higher above terrain

			sceneManager->setPlayer(std::make_unique<physics::Player>(std::move(playerCreationSettings), physicsSimulation->getPhysicsSystem()));
			sceneManager->getPlayer()->setPerspectiveProjection(glm::radians(60.0f), (float) (window->getWidth() / window->getHeight()), 0.1f, 100.0f);
		}

		// Water
		{
			std::unique_ptr<Model> waterModelUnique = Model::createGridModel(*device, 1000);
			std::shared_ptr<Model> waterModel = std::shared_ptr<Model>(std::move(waterModelUnique));
			auto waterMaterial = std::make_shared<WaterMaterial>(*device, "textures:water.png");
			waterModel->setMaterial(waterMaterial);

			// Define a simple GameObject for water
			class WaterGameObject : public GameObject {
			   public:
				WaterGameObject(std::shared_ptr<Model> m, glm::mat4 transform)
					: modelPtr(std::move(m)), transformMat(transform) {}
				glm::mat4 computeModelMatrix() const override {
					return transformMat;
				}
				glm::mat4 computeNormalMatrix() const override {
					return glm::transpose(glm::inverse(transformMat));
				}
				glm::vec3 getPosition() const override {
					return glm::vec3(transformMat[3]);
				}
				std::shared_ptr<Model> getModel() const override {
					return modelPtr;
				}

			   private:
				std::shared_ptr<Model> modelPtr;
				glm::mat4 transformMat;
			};
			float waterSize = 2000.0f;
			float waterHeight = -10.0f;
			glm::mat4 waterTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, waterHeight, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(waterSize, 1.0f, waterSize));
			std::unique_ptr<GameObject> waterObject = std::make_unique<WaterGameObject>(waterModel, waterTransform);
			sceneManager->addWaterObject(std::move(waterObject));
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
			auto skybox = std::make_unique<Skybox>(*device, cubemapFaces);
			sceneManager->addSpectralObject(std::move(skybox));
		}

		// Enemies
		{
			float enemyHullHeight = 1.25f;
			float enemyRadius = 0.3f;
			JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));
			shared_ptr<Model> enemyModel = Model::createModelFromFile(*device, "models:CesiumMan.glb");
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
				sceneManager->addEnemy(std::move(make_unique<physics::Sprinter>(std::move(sprinterCreationSettings), physicsSimulation->getPhysicsSystem())));
			}
		}

		// UI
		{
			UIComponentCreationSettings hudSettings{};
			hudSettings.model = Model::createModelFromFile(*device, "models:gray_quad.glb", true);
			hudSettings.name = "gray_quad";
			hudSettings.controllable = false;
			sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));

			hudSettings.model = Model::createModelFromFile(*device, "models:DamagedHelmet.glb", true);
			hudSettings.name = "damaged_helmet";
			hudSettings.controllable = false;
			sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));

			hudSettings.model = Model::createModelFromFile(*device, "models:USPS.glb", true);
			hudSettings.name = "usps";
			hudSettings.controllable = false;
			hudSettings.window = window->getGLFWWindow();
			hudSettings.anchorRight = true;
			hudSettings.anchorBottom = true;
			sceneManager->addUIObject(std::make_unique<UIComponent>(hudSettings));
		}
	}
}