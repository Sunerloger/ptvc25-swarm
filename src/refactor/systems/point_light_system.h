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
    class PointLightSystem {
    public:

        PointLightSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~PointLightSystem();

        PointLightSystem(const PointLightSystem&) = delete;
        PointLightSystem& operator=(const PointLightSystem&) = delete;

        void render(FrameInfo& frameInfo);


    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    }
;}
