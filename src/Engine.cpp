#include "Engine.h"

#include "AudioSystem.h"

namespace vk {

	std::unique_ptr<DestructionQueue> Engine::destructionQueue = nullptr;

	Engine::Engine(IGame& game, physics::PhysicsSimulation& physicsSimulation, vk::Window& window, vk::Device& device, input::InputManager& inputManager)
		: physicsSimulation(physicsSimulation), game(game), window(window), device(device), inputManager(inputManager), renderer(window, device) {

		globalPool = DescriptorPool::Builder(device)
						 .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
						 .build();
		
		if (!destructionQueue) {
			std::cout << "Engine: Creating destruction queue" << std::endl;
			destructionQueue = std::make_unique<DestructionQueue>(device, &renderer.getSwapChain());
		}

		std::cout << "Engine: Creating shadow map" << std::endl;
		ShadowMap::ShadowMapSettings shadowSettings{};
		shadowMap = std::make_unique<ShadowMap>(device, shadowSettings);

		std::cout << "Engine: Initializing game" << std::endl;
		game.init();
		game.setupInput();
		
		std::cout << "Engine: Initializing audio system" << std::endl;
		audio::AudioSystem::getInstance().init();
	}

	Engine::~Engine() {
	    std::cout << "Engine: Starting shutdown sequence" << std::endl;
	    
	    if (destructionQueue) {
	        std::cout << "Engine: Cleaning up destruction queue" << std::endl;
	        destructionQueue->cleanup();
	        std::cout << "Engine: Resetting destruction queue" << std::endl;
	        destructionQueue.reset();
	    } else {
	        std::cout << "Engine: Warning - No destruction queue to clean up" << std::endl;
	    }
	    
	    if (globalPool) {
	        globalPool.reset();
	    }
	    
	    std::cout << "Engine: Shutdown sequence complete" << std::endl;
	}

	void Engine::run() {
		SceneManager& sceneManager = SceneManager::getInstance();
		
		sceneManager.awakeAll();

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

		TerrainRenderSystem terrainRenderSystem{
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
			deltaTime = (deltaTime < engineSettings.maxFrameTime) ? deltaTime : engineSettings.maxFrameTime;

			glfwPollEvents();

			inputManager.processPolling(deltaTime);

			if (window.framebufferResized) {
				renderer.recreateSwapChain();
				destructionQueue->setSwapChain(&renderer.getSwapChain());
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
				physicsTimeAccumulator = (physicsTimeAccumulator < engineSettings.cPhysicsDeltaTime) ?
				                                    physicsTimeAccumulator : engineSettings.cPhysicsDeltaTime;
			}
			else {
				game.gamePauseUpdate(deltaTime);
			}

			// Camera
			float aspect = renderer.getAspectRatio();
			sceneManager.getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.01f, 10000.0f);

			audio::AudioSystem::getInstance().update3dAudio();

			// menu / death screen is just rendered on top of game while physics / logic is disabled
			if (auto commandBuffer = renderer.beginFrame()) {

				int frameIndex = renderer.getFrameIndex();
				FrameInfo frameInfo{deltaTime, commandBuffer, globalDescriptorSets[frameIndex]};

				shadowMap->updateShadowUbo(frameIndex);

				GlobalUbo ubo{};
				ubo.uiOrthographicProjection = getOrthographicProjection(0, window.getWidth(), 0, window.getHeight(), 0.1f, 500.0f);
				ubo.sunDirection = glm::vec4(sceneManager.getSun()->getDirection(), 1.0f);
				ubo.sunColor = glm::vec4(sceneManager.getSun()->getColor(), 1.0f);
				ubo.cameraPosition = glm::vec4(sceneManager.getPlayer()->getCameraPosition(), 1.0f);
				
				// shadow map rendering pass
				{
					// TODO maybe later use an enum for also e.g. post-processing
					frameInfo.isShadowPass = true;
					
					// temporarily set projection and view with light's perspective
					const ShadowMap::ShadowUbo& shadowUbo = shadowMap->getShadowUbo();
					ubo.projection = shadowUbo.lightProjectionMatrix;
					ubo.view = shadowUbo.lightViewMatrix;
					
					uboBuffers[frameIndex]->writeToBuffer(&ubo);
					uboBuffers[frameIndex]->flush();

					VkDescriptorSetLayout shadowMapLayout = shadowMap->getDescriptorSetLayout();
					textureRenderSystem.setShadowMapLayout(shadowMapLayout);
					terrainRenderSystem.setShadowMapLayout(shadowMapLayout);
					
					// begin shadow pass
					std::vector<VkClearValue> clearValues = shadowMap->getClearValues();
					renderer.beginRenderPass(
						commandBuffer,
						shadowMap->getRenderPass(),
						shadowMap->getFramebuffer(),
						shadowMap->getExtent(),
						clearValues
					);
					
					textureRenderSystem.renderGameObjects(frameInfo);
					terrainRenderSystem.renderGameObjects(frameInfo);
					
					// end shadow pass
					renderer.endRenderPass(commandBuffer);
					
					frameInfo.isShadowPass = false;
					
					ubo.projection = sceneManager.getPlayer()->getProjMat();
					ubo.view = sceneManager.getPlayer()->calculateViewMat();
					uboBuffers[frameIndex]->writeToBuffer(&ubo);
					uboBuffers[frameIndex]->flush();

					textureRenderSystem.setShadowMapLayout(VK_NULL_HANDLE);
					terrainRenderSystem.setShadowMapLayout(VK_NULL_HANDLE);
				}
				
				// begin main render pass
				std::vector<VkClearValue> clearValues = {
					{0.01f, 0.01f, 0.01f, 1.0f},
					{1.0f, 0}
				};
				renderer.beginRenderPass(
					commandBuffer,
					renderer.getSwapChainRenderPass(),
					renderer.getSwapChain().getFrameBuffer(frameIndex),
					renderer.getSwapChain().getSwapChainExtent(),
					clearValues
				);
				
				// bind shadow map descriptor set to texture render system
				shadowMap->bindDescriptorSet(
					commandBuffer,
					textureRenderSystem.getPipelineLayout(),
					frameIndex
				);
				
				// bind shadow map descriptor set to terrain render system
				shadowMap->bindDescriptorSet(
					commandBuffer,
					terrainRenderSystem.getPipelineLayout(),
					frameIndex
				);
				
				// render main scene
				textureRenderSystem.renderGameObjects(frameInfo);
				terrainRenderSystem.renderGameObjects(frameInfo);
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
				renderer.endRenderPass(commandBuffer);
				renderer.endFrame();
			}
		}
		if (destructionQueue) {
			for (auto& bufPtr : uboBuffers) {
				if (bufPtr) {
					bufPtr->scheduleDestroy(*destructionQueue);
					bufPtr.reset();
				}
			}
		}
	}

	void Engine::scheduleResourceDestruction(VkBuffer buffer, VkDeviceMemory memory) {
		if (destructionQueue) {
			std::cout << "Engine: Scheduling buffer " << std::hex << (uint64_t)buffer
				<< " and memory " << (uint64_t)memory << std::dec << " for destruction" << std::endl;
			destructionQueue->pushBuffer(buffer, memory);
		} else {
			std::cout << "Engine: Warning - Cannot schedule buffer " << std::hex << (uint64_t)buffer
				<< " for destruction - no destruction queue" << std::dec << std::endl;
		}
	}
}