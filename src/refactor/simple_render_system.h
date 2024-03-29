//
// Created by Vlad Dancea on 29.03.24.
//

#pragma once

#include "vk_camera.h"
#include "vk_device.h"
#include "vk_game_object.h"
#include "vk_pipeline.h"


#include <memory>
#include <vector>

namespace vk {
    class SimpleRenderSystem {
    public:

        SimpleRenderSystem(Device& device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem&) = delete;
        SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

        void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera);


    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    }
;}
