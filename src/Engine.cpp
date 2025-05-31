#include "Engine.h"

namespace vk {

	Engine::Engine(IGame& game, physics::PhysicsSimulation& physicsSimulation, vk::Window& window, vk::Device& device, input::InputManager& inputManager)
		: physicsSimulation(physicsSimulation), game(game), window(window), device(device), inputManager(inputManager), renderer(window, device) {

		globalPool = DescriptorPool::Builder(device)
						 .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .build();

		game.init();
		game.setupInput();
	}

	Engine::~Engine() {}

	void Engine::run() {
		SceneManager& sceneManager = SceneManager::getInstance();

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
		TextureRenderSystem textureRenderSystem{
			device,
			renderer,
			globalSetLayout->getDescriptorSetLayout()
		};

		TessellationRenderSystem tessellationRenderSystem{
			device,
			renderer,
			globalSetLayout->getDescriptorSetLayout()
		};

		WaterRenderSystem waterRenderSystem{
			device,
			renderer,
			globalSetLayout->getDescriptorSetLayout()
		};

		UIRenderSystem uiRenderSystem{
			device,
			renderer,
			globalSetLayout->getDescriptorSetLayout()
		};

		startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = startTime;
		float physicsTimeAccumulator = 0.0f;

		while (!window.shouldClose()) {
			auto newTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			float realDeltaTime = deltaTime;
			deltaTime = std::min(deltaTime, engineSettings.maxFrameTime);

			glfwPollEvents();

			inputManager.processPolling(deltaTime);

			if (window.framebufferResized) {
				renderer.recreateSwapChain();
				window.framebufferResized = false;
			}

			if (!game.isPaused()) {

				sceneManager.realTime += realDeltaTime;
				sceneManager.gameTime += deltaTime;

				// Time
				int newSecond = floor(sceneManager.realTime + realDeltaTime);
				if (engineSettings.debugTime && newSecond > floor(sceneManager.realTime)) {
					std::cout << "Time since start: " << newSecond << "s" << std::endl;
				}

				game.gameActiveUpdate(deltaTime);

				physicsTimeAccumulator += deltaTime;

				// maximum: subSteps * cPhysicsDeltaTime, if more: physics runs slower to prevent spiral of death
				for (int subSteps = 0; physicsTimeAccumulator >= engineSettings.cPhysicsDeltaTime && subSteps < physicsSimulation.maxPhysicsSubSteps; subSteps++) {
					game.prePhysicsUpdate();
					
					physicsSimulation.preSimulation();
					physicsSimulation.simulate();
					physicsSimulation.postSimulation(engineSettings.debugPlayer, engineSettings.debugEnemies);
					
					physicsTimeAccumulator -= engineSettings.cPhysicsDeltaTime;
					sceneManager.simulationTime += engineSettings.cPhysicsDeltaTime;

					game.postPhysicsUpdate();
				}
				// throw away more than one physics update to prevent physics running too often next step
				physicsTimeAccumulator = glm::min(physicsTimeAccumulator, engineSettings.cPhysicsDeltaTime);
			}
			else {
				game.gamePauseUpdate(deltaTime);
			}

			// Camera
			float aspect = renderer.getAspectRatio();
			sceneManager.getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

			// menu / death screen is just rendered on top of game while physics / logic is disabled
			if (auto commandBuffer = renderer.beginFrame()) {
				int frameIndex = renderer.getFrameIndex();
				FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex]};

				GlobalUbo ubo{};
				ubo.projection = sceneManager.getPlayer()->getProjMat();
				ubo.view = sceneManager.getPlayer()->calculateViewMat();
				ubo.uiOrthographicProjection = getOrthographicProjection(0, window.getWidth(), 0, window.getHeight(), 0.1f, 500.0f);
				ubo.sunDirection = glm::vec4(sceneManager.getSun()->getDirection(), 1.0f);
				ubo.sunColor = glm::vec4(sceneManager.getSun()->color, 1.0f);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				renderer.beginSwapChainRenderPass(commandBuffer);
				textureRenderSystem.renderGameObjects(frameInfo);
				tessellationRenderSystem.renderGameObjects(frameInfo);
				waterRenderSystem.renderGameObjects(frameInfo);

				VkClearAttachment clearAttachment{};
				clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				clearAttachment.clearValue.depthStencil = {/* depth = */ 1.0f, /* stencil = */ 0};
				VkClearRect clearRect{};
				clearRect.rect.offset = {0, 0};
				clearRect.rect.extent = {
					static_cast<uint32_t>(window.getWidth()),
					static_cast<uint32_t>(window.getHeight())};
				clearRect.baseArrayLayer = 0;
				clearRect.layerCount = 1;

				// write values directly into color/depth attachments for ui drawing
				vkCmdClearAttachments(
					frameInfo.commandBuffer,
					1,
					&clearAttachment,
					1,
					&clearRect);

				uiRenderSystem.renderGameObjects(frameInfo);
				renderer.endSwapChainRenderPass(commandBuffer);
				renderer.endFrame();
			}
		}
	}
}