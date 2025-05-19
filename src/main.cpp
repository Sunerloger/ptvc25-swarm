#include "first_app.h"
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
		vk::AssetManager::getInstance().initialize(argv[0]);

		// Additional custom paths can be registered if needed
		// vk::AssetManager::getInstance().registerPath("customModels", "/path/to/custom/models");

		vk::FirstApp app{};
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}