//
// Created by Vlad Dancea on 03.04.24.
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
    class HudSystem {
    public:

        HudSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~HudSystem();

        HudSystem(const HudSystem&) = delete;
        HudSystem& operator=(const HudSystem&) = delete;

        void renderGameObjects(FrameInfo& frameInfo, bool escapeMenuOpen);


    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    }
    ;}