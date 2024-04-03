//
// Created by Vlad Dancea on 29.03.24.
//

#pragma once

#include "../vk_camera.h"
#include "../vk_device.h"
#include "../vk_game_object.h"
#include "../vk_pipeline.h"
#include "../vk_frame_info.h"


#include <memory>
#include <vector>

namespace vk {
    class SimpleRenderSystem {
    public:

        SimpleRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem&) = delete;
        SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

        void renderGameObjects(FrameInfo& frameInfo);
        void update(FrameInfo& frameInfo, GlobalUbo& ubo, Camera& camera);


    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    }
;}
