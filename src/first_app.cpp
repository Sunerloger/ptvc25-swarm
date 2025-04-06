#include "first_app.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <glm/gtx/string_cast.hpp>

namespace vk {

	FirstApp::FirstApp() {
		window = std::make_unique<Window>(applicationSettings.windowWidth, applicationSettings.windowHeight, "Swarm");
		device = std::make_unique<Device>(*window);
		renderer = std::make_unique<Renderer>(*window, *device);

		globalPool = DescriptorPool::Builder(*device)
						 .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .build();

		this->sceneManager = make_shared<SceneManager>();
		this->physicsSimulation = make_unique<physics::PhysicsSimulation>(this->sceneManager, engineSettings.cPhysicsDeltaTime);

		loadGameObjects();
	}

	FirstApp::~FirstApp() {
		Model::cleanupTextureResources(*device);
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

		// Create render systems.
		TextureRenderSystem textureRenderSystem{*device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout(),
			Model::textureDescriptorSetLayout};

		glfwSetInputMode(window->getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		controls::KeyboardMovementController movementController{applicationSettings.windowWidth, applicationSettings.windowHeight};

		auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = startTime;
		int currentSecond = 0;
		float gameTimer = 0;
		float physicsTimeAccumulator = 0.0f;

		while (!window->shouldClose()) {
			auto newTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			deltaTime = std::min(deltaTime, engineSettings.maxFrameTime);

			glfwPollEvents();
			movementController.handleEscMenu(window->getGLFWWindow());

			if (!movementController.escapeMenuOpen) {
				physicsTimeAccumulator += deltaTime;
				gameTimer += deltaTime;
				int newSecond = static_cast<int>(gameTimer);
				if (engineSettings.debugTime && newSecond > currentSecond) {
					currentSecond = newSecond;
					// Debug: std::cout << "Time since start: " << currentSecond << "s" << std::endl;
				}
				movementController.handleRotation(window->getGLFWWindow(), *sceneManager->getPlayer());
				MovementIntent movementIntent = movementController.getMovementIntent(window->getGLFWWindow());
				while (physicsTimeAccumulator >= engineSettings.cPhysicsDeltaTime) {
					physicsSimulation->preSimulation(movementIntent);
					physicsSimulation->simulate();
					physicsSimulation->postSimulation(engineSettings.debugPlayer, engineSettings.debugEnemies);
					physicsTimeAccumulator -= engineSettings.cPhysicsDeltaTime;
				}
				float aspect = renderer->getAspectRatio();
				sceneManager->getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f, 100.0f);

				if (auto commandBuffer = renderer->beginFrame()) {
					int frameIndex = renderer->getFrameIndex();
					FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex], *sceneManager};

					GlobalUbo ubo{};
					ubo.projection = sceneManager->getPlayer()->getProjMat();
					ubo.view = sceneManager->getPlayer()->calculateViewMat();
					ubo.inverseView = glm::inverse(ubo.view);
					ubo.aspectRatio = aspect;
					uboBuffers[frameIndex]->writeToBuffer(&ubo);
					uboBuffers[frameIndex]->flush();

					renderer->beginSwapChainRenderPass(commandBuffer);
					textureRenderSystem.renderGameObjects(frameInfo);
					renderer->endSwapChainRenderPass(commandBuffer);
					renderer->endFrame();
				}
			} else {
				if (auto commandBuffer = renderer->beginFrame()) {
					int frameIndex = renderer->getFrameIndex();
					FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex], *sceneManager};
					renderer->beginSwapChainRenderPass(commandBuffer);
					textureRenderSystem.renderGameObjects(frameInfo);
					renderer->endSwapChainRenderPass(commandBuffer);
					renderer->endFrame();
				}
			}
			vkDeviceWaitIdle(device->device());
		}
	}

	void FirstApp::loadGameObjects() {
		std::shared_ptr<Model> floorModel = Model::createModelFromFile(*device, "models:BoxTextured.glb");

		// 2m player
		float playerHeight = 1.40f;
		float playerRadius = 0.3f;
		Ref<Shape> characterShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * playerHeight + playerRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * playerHeight, playerRadius)).Create().Get();

		std::unique_ptr<CharacterCameraSettings> cameraSettings = std::make_unique<CharacterCameraSettings>();
		cameraSettings->cameraOffsetFromCharacter = glm::vec3(0.0f, playerHeight + playerRadius, 0.0f);

		std::unique_ptr<physics::PlayerSettings> playerSettings = std::make_unique<physics::PlayerSettings>();

		std::unique_ptr<JPH::CharacterSettings> characterSettings = std::make_unique<JPH::CharacterSettings>();
		characterSettings->mGravityFactor = 0.0f;
		characterSettings->mFriction = 10.0f;
		characterSettings->mShape = characterShape;
		characterSettings->mLayer = physics::Layers::MOVING;
		characterSettings->mSupportingVolume = Plane(Vec3::sAxisY(), -playerRadius);  // Accept contacts that touch the lower sphere of the capsule

		std::unique_ptr<physics::PlayerCreationSettings> playerCreationSettings = std::make_unique<physics::PlayerCreationSettings>();
		playerCreationSettings->characterSettings = std::move(characterSettings);
		playerCreationSettings->cameraSettings = std::move(cameraSettings);
		playerCreationSettings->playerSettings = std::move(playerSettings);

		sceneManager->setPlayer(std::make_unique<physics::Player>(std::move(playerCreationSettings), physicsSimulation->getPhysicsSystem()));

		// add terrain to scene
		// rotate the model to match the terrain
		sceneManager->addManagedPhysicsEntity(std::make_unique<physics::Terrain>(physicsSimulation->getPhysicsSystem(), glm::vec3{0.569, 0.29, 0}, floorModel, glm::vec3{0.0, -1.0, 0.0}, glm::vec3{1.0f, 1.0f, 1.0f}));

		glfwSetWindowUserPointer(window->getGLFWWindow(), sceneManager.get());
	}
}