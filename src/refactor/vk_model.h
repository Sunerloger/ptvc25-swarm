//
// Created by Vlad Dancea on 28.03.24.
//

#ifndef GCGPROJECT_VK_MODEL_H
#define GCGPROJECT_VK_MODEL_H

#endif //GCGPROJECT_VK_MODEL_H

#pragma once

#include "vk_device.h"

#include <vector>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace vk {
    class Model{
        public:

            struct Vertex {
                glm::vec2 position;
                glm::vec3 color;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };
            Model(Device& device, const std::vector<Vertex>& vertices);
            ~Model();

            Model(const Model&) = delete;
            void operator=(const Model&) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffer(const std::vector<Vertex>& vertices);

            Device& device;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;

    };
}
