//
// Created by Vlad Dancea on 28.03.24.
//
#pragma once

#include "vk_device.h"
#include "vk_renderer.h"
#include "vk_window.h"
#include "vk_descriptors.h"
#include "vk_camera.h"

#include "simulation/objects/actors/Player.h"
#include "simulation/PhysicsSimulation.h"

#include <memory>
#include <vector>
#include "SceneManager.h"

namespace vk {
    class FirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        static constexpr float MAX_FRAME_TIME = 0.01f;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp&) = delete;
        FirstApp& operator=(const FirstApp&) = delete;

        void run();

    private:
        void loadGameObjects();

        Window window{WIDTH, HEIGHT, "Swarm"};
        Device device{window};
        Renderer renderer{window, device};

        std::unique_ptr<DescriptorPool> globalPool{};

        std::unique_ptr<physics::PhysicsSimulation> physicsSimulation = make_unique<physics::PhysicsSimulation>();

        unique_ptr<SceneManager> sceneManager = make_unique<SceneManager>();;
    }
;}
