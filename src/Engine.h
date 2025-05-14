#pragma once

#include "IGame.h"

#include "vk/vk_device.h"
#include "vk/vk_renderer.h"
#include "vk/vk_window.h"
#include "vk/vk_descriptors.h"
#include "vk/vk_buffer.h"

#include "simulation/PhysicsSimulation.h"

#include "systems/texture_render_system.h"
#include "systems/ui_render_system.h"
#include "systems/tessellation_render_system.h"

#include "keyboard_movement_controller.h"
#include "keyboard_placement_controller.h"
#include "keyboard_menu_controller.h"

#include "scene/SceneManager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtx/string_cast.hpp>

namespace vk {

	struct EngineSettings {
		float cPhysicsDeltaTime = 1.0f / 60.0f;
		float maxFrameTime = 0.01f;
		bool debugTime = false;
		bool debugPlayer = false;
		bool debugEnemies = false;
	};

	class Engine {
	   public:
		Engine(shared_ptr<IGame> game);
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void run();

	   private:

		std::shared_ptr<IGame> game;
		
		std::unique_ptr<Window> window;
		std::unique_ptr<controls::KeyboardMenuController> menuController;
		std::unique_ptr<Device> device;
		std::unique_ptr<Renderer> renderer;

		std::unique_ptr<DescriptorPool> globalPool{};

		std::unique_ptr<physics::PhysicsSimulation> physicsSimulation;

		std::shared_ptr<SceneManager> sceneManager;

		chrono::steady_clock::time_point startTime;
		
		// TODO read via ini file
		EngineSettings engineSettings = {};
		int windowWidth = 800;
		int windowHeight = 800;
	};
}
