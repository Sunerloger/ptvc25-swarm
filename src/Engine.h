#pragma once

#include "IGame.h"

#include "vk/vk_device.h"
#include "vk/vk_renderer.h"
#include "vk/vk_window.h"
#include "vk/vk_descriptors.h"
#include "vk/vk_buffer.h"
#include "vk/vk_destruction_queue.h"

#include "simulation/PhysicsSimulation.h"

#include "rendering/render_systems/TextureRenderSystem.h"
#include "rendering/render_systems/UIRenderSystem.h"
#include "rendering/render_systems/TerrainRenderSystem.h"
#include "rendering/render_systems/WaterRenderSystem.h"

#include "rendering/ShadowMap.h"

#include "scene/SceneManager.h"
#include "logical_systems/input/InputManager.h"

#include "camera/CameraUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>

namespace vk {

	struct EngineSettings {
		float cPhysicsDeltaTime = 1.0f / 60.0f;
		float maxFrameTime = 0.2f; // 5 fps
		bool debugTime = false;
		bool debugPlayer = false;
		bool debugEnemies = false; // be careful with this flag, it heavily impacts performance
		bool useShadowMap = true; // broken if you use shadow mapping in the shaders and set this to false or the other way around
	};

	class Engine {
	   public:
		Engine(IGame& game, physics::PhysicsSimulation& physicsSimulation, vk::Window& window, vk::Device& device, input::InputManager& inputManager);
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void run();
		
		static DestructionQueue* getDestructionQueue() { return destructionQueue.get(); }
		
		static void scheduleResourceDestruction(VkBuffer buffer, VkDeviceMemory memory);

		  private:

		IGame& game;
		physics::PhysicsSimulation& physicsSimulation;
		input::InputManager& inputManager;

		vk::Window& window;
		vk::Device& device;
		
		Renderer renderer;

		std::unique_ptr<DescriptorPool> globalPool{};
		
		static std::unique_ptr<DestructionQueue> destructionQueue;

		chrono::steady_clock::time_point startTime;
		
		// TODO read via ini file
		EngineSettings engineSettings = {};
		
		std::unique_ptr<ShadowMap> shadowMap;
	};
}
