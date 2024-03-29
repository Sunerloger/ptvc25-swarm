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

                // Changes here need to be reflected in the
                // getAttributeDescriptions method
                glm::vec3 position;
                glm::vec3 color;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            struct Builder {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

            };

            Model(Device& device, const Model::Builder& builder);
            ~Model();

            Model(const Model&) = delete;
            void operator=(const Model&) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffers(const std::vector<Vertex>& vertices);
            void createIndexBuffers(const std::vector<uint32_t>& indices);

            Device& device;

            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;

            bool hasIndexBuffer =  false;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
            uint32_t indexCount;
    };
}
