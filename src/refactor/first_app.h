//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_FIRST_APP_H
#define GCGPROJECT_VK_FIRST_APP_H

#pragma once

#include "vk_device.h"
#include "vk_game_object.h"
#include "vk_pipeline.h"
#include "vk_window.h"
#include "vk_renderer.h"


#include <memory>
#include <vector>

namespace vk {
    class FirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp&) = delete;
        FirstApp& operator=(const FirstApp&) = delete;

        void run();

    private:
        void createPipelineLayout();
        void createPipeline();
        void renderGameObjects(VkCommandBuffer commandBuffer);
        void loadGameObjects();


        Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
        Device device{window};
        Renderer renderer{window, device};
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<GameObject> gameObjects;

    }
;}

#endif //GCGPROJECT_VK_FIRST_APP_H
