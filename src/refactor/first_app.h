//
// Created by Vlad Dancea on 28.03.24.
//
#pragma once

#include "vk_device.h"
#include "vk_game_object.h"
#include "vk_renderer.h"
#include "vk_window.h"
#include "vk_descriptors.h"

#include <memory>
#include <vector>

namespace vk {
    class FirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        static constexpr float MAX_FRANE_TIME = 1.0f;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp&) = delete;
        FirstApp& operator=(const FirstApp&) = delete;

        void run();

    private:
        void loadGameObjects();

        Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
        Device device{window};
        Renderer renderer{window, device};

        std::unique_ptr<DescriptorPool> globalPool{};
        std::vector<GameObject> gameObjects;
    }
;}
