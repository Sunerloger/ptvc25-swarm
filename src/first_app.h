//
// Created by Vlad Dancea on 28.03.24.
//
#pragma once

#include "vk_device.h"
#include "vk_renderer.h"
#include "vk_window.h"
#include "vk_descriptors.h"

#include "scene/SceneManager.h"

#include "vk_buffer.h"
#include "systems/simple_render_system.h"
#include "systems/point_light_system.h"
#include "systems/cross_hair_system.h"
#include "systems/hud_system.h"
#include "keyboard_movement_controller.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "tiny_obj_loader.h"

#include "simulation/objects/actors/Player.h"
#include "simulation/PhysicsSimulation.h"
#include "simulation/objects/static/Terrain.h"
#include "simulation/objects/actors/enemies/Sprinter.h"

#include "ui/UIComponent.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

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
        glm::mat2x3 loadBoundingBoxFromFile(const std::string& filename);

        Window window{WIDTH, HEIGHT, "Swarm"};
        Device device{window};
        Renderer renderer{window, device};

        std::unique_ptr<DescriptorPool> globalPool{};

        std::unique_ptr<physics::PhysicsSimulation> physicsSimulation;

        std::shared_ptr<SceneManager> sceneManager;
    }
;}
