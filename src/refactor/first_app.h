//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_FIRST_APP_H
#define GCGPROJECT_VK_FIRST_APP_H

#pragma once
#include "vk_window.h"
#include "vk_pipeline.h"
#include "vk_swap_chain.h"
#include "vk_window.h"
#include "vk_game_object.h"

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
        void createCommandBuffers();
        void freeCommandBuffers();
        void drawFrame();
        void loadGameObjects();
        void recreateSwapChain();
        void recordCommandBuffer(int imageIndex);
        void renderGameObjects(VkCommandBuffer commandBuffer);


        Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
        Device device{window};
        std::unique_ptr<SwapChain> swapChain;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<GameObject> gameObjects;

    }
;}

#endif //GCGPROJECT_VK_FIRST_APP_H
