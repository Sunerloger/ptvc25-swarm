//
// Created by Vlad Dancea on 28.03.24.
//

#include "first_app.h"
#include "simple_render_system.h"
#include "vk_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace vk {

    FirstApp::FirstApp() {
        loadGameObjects();
    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};


        while (!window.shouldClose()) {
            glfwPollEvents();

            if (auto commandBuffer = renderer.beginFrame()) {
                renderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device());
    }

    void FirstApp::loadGameObjects() {
        std::vector<Model::Vertex> vertices{
                {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
        auto lveModel = std::make_shared<Model>(device, vertices);

        auto triangle = GameObject::createGameObject();
        triangle.model = lveModel;
        triangle.color = {.1f, .8f, .1f};
        triangle.transform2d.translation.x = .2f;
        triangle.transform2d.scale = {2.f, .5f};
        triangle.transform2d.rotation = .25f * glm::two_pi<float>();

        gameObjects.push_back(std::move(triangle));
    }
}