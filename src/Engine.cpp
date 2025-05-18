#include "Engine.h"

namespace vk {

	Engine::Engine(IGame& game, physics::PhysicsSimulation& physicsSimulation, std::shared_ptr<SceneManager> sceneManager, vk::Window& window, vk::Device& device)
		: physicsSimulation(physicsSimulation), game(game), sceneManager(sceneManager), window(window), device(device) {

		// TODO inject window, device and physics simulation
		// TODO create windowSettings in window and load with ini reader
		// TODO create physicsSettings in physicsSimulation and load with ini reader
		// TODO boolean flags for testing directly in physicsSimulation settings

		menuController = std::make_unique<controls::KeyboardMenuController>(window.getGLFWWindow());
		
		renderer = std::make_unique<Renderer>(window, device);
		menuController->setConfigChangeCallback([&]() {
			int w, h;
			window.getFramebufferSize(w, h);
			renderer->recreateSwapChain();
		});

		globalPool = DescriptorPool::Builder(device)
						 .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .build();

		game.init();
	}

	Engine::~Engine() {}

	void Engine::run() {
		std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<Buffer>(device,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = DescriptorSetLayout::Builder(device)
								   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
								   .build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		// TODO create an additional uniform buffer for lighting information stored in sceneManager (updated every frame)

		// Create render systems
		TextureRenderSystem textureRenderSystem{device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};

		TessellationRenderSystem tessellationRenderSystem{device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};

		UIRenderSystem uiRenderSystem{device,
			renderer->getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()};

		glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		
		// TODO maybe also handle this via the specific game instead of in the engine
		controls::KeyboardPlacementController placementController;

		startTime = std::chrono::high_resolution_clock::now();
		float gameTime = 0;
		auto currentTime = startTime;
		float physicsTimeAccumulator = 0.0f;

		int fbWidth, fbHeight;
		window.getFramebufferSize(fbWidth, fbHeight);
		float windowWidth = static_cast<float>(fbWidth);
		float windowHeight = static_cast<float>(fbHeight);

		while (!window.shouldClose()) {
			auto newTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			deltaTime = std::min(deltaTime, engineSettings.maxFrameTime);

			glfwPollEvents();

			int placementTransform = placementController.updateModelMatrix(window.getGLFWWindow());
			sceneManager->updateUITransforms(deltaTime, placementTransform);

			if (!menuController->isMenuOpen()) {
				// Time
				int newSecond = floor(gameTime + deltaTime);
				if (engineSettings.debugTime && newSecond > floor(gameTime)) {
					std::cout << "Time since start: " << newSecond << "s" << std::endl;
				}
				physicsTimeAccumulator += deltaTime;
				gameTime += deltaTime;

				game.gameActiveUpdate(deltaTime);

				while (physicsTimeAccumulator >= engineSettings.cPhysicsDeltaTime) {
					game.prePhysicsUpdate();
					
					physicsSimulation.preSimulation();
					physicsSimulation.simulate();
					physicsSimulation.postSimulation(engineSettings.debugPlayer, engineSettings.debugEnemies);
					
					physicsTimeAccumulator -= engineSettings.cPhysicsDeltaTime;

					game.postPhysicsUpdate();
				}
			}
			else {
				game.gamePauseUpdate(deltaTime);
			}

			// Camera
			float aspect = renderer->getAspectRatio();
			sceneManager->getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

			// menu / death screen is just rendered on top of game while physics / logic is disabled
			if (auto commandBuffer = renderer->beginFrame()) {
				int frameIndex = renderer->getFrameIndex();
				FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex], *sceneManager};

				GlobalUbo ubo{};
				ubo.projection = sceneManager->getPlayer()->getProjMat();
				ubo.view = sceneManager->getPlayer()->calculateViewMat();
				ubo.uiOrthographicProjection = CharacterCamera::getOrthographicProjection(0, windowWidth, 0, windowHeight, 0.1f, 500.0f);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				renderer->beginSwapChainRenderPass(commandBuffer);
				textureRenderSystem.renderGameObjects(frameInfo);
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
			window.getFramebufferSize(fbWidth2, fbHeight2);
			float windowWidth2 = static_cast<float>(fbWidth2);
			float windowHeight2 = static_cast<float>(fbHeight2);
			if (windowWidth != windowWidth2 || windowHeight != windowHeight2) {
				windowWidth = windowWidth2;
				windowHeight = windowHeight2;
				renderer->recreateSwapChain();
			}

			// TODO use fences / semaphores instead (next line forces sync of cpu and gpu and heavily impacts performance):
			vkDeviceWaitIdle(device.device());
		}
	}
}