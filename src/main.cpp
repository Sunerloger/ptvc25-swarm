#include "Engine.h"
#include "Swarm.h"
#include "asset_utils/AssetLoader.h"
#include "asset_utils/AssetManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
	try {

		vk::AssetLoader::getInstance().initialize(argv[0]);
		// Additional custom paths can be registered if needed
		// vk::AssetLoader::getInstance().registerPath("customModels", "/path/to/custom/models");


		std::shared_ptr<SceneManager> sceneManager = std::make_shared<SceneManager>();
		AssetManager assetManager{};

		physics::PhysicsSimulation physicsSimulation{sceneManager};

		// TODO read via ini file
		int initialWindowWidth = 800;
		int initialWindowHeight = 800;

		// TODO abstract this into another class to not directly inject low level details into the game logic
		vk::Window window{ initialWindowWidth, initialWindowHeight, Swarm::Name };
		vk::Device device{ window };

		// TODO advanced input system
		controls::KeyboardMovementController movementController{ initialWindowWidth, initialWindowHeight };

		Swarm game{ physicsSimulation, sceneManager, assetManager, window, device, movementController };

		vk::Engine engine{game, physicsSimulation, sceneManager, window, device};

		engine.run();

	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}