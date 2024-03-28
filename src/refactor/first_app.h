//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_FIRST_APP_H
#define GCGPROJECT_VK_FIRST_APP_H

#pragma once
#include "vk_window.h"
#include "vk_pipeline.h"
#include "filesystem"
#include "iostream"

namespace vk {
    class FirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        void run();

    private:
        Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
        Device device{window};
        Pipeline pipeline{device,
                          "../src/refactor/shaders/simple_shader.vert.spv",
                          "../src/refactor/shaders/simple_shader.frag.spv",
                          Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
    }
;}

#endif //GCGPROJECT_VK_FIRST_APP_H
