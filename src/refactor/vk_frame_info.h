//
// Created by Vlad Dancea on 30.03.24.
//

#pragma once

#include "vk_camera.h"

#include "vulkan/vulkan.h"

namespace vk {
    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        Camera &camera;
    };
}

