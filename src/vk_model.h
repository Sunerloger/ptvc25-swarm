#pragma once

#include "vk_device.h"
#include "vk_buffer.h"

#include <vector>
#include <memory>

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
                glm::vec3 normal;
                glm::vec2 uv;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

                bool operator==(const Vertex& other) const {
                    return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
                }
            };

            struct Builder {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                void loadModel(const std::string& filename);

                void loadModelWithoutTinyObjLoader(const std::string& filename);
            };

            Model(Device& device, const Model::Builder& builder);
            ~Model();

            static std::unique_ptr<Model> createModelFromFile(bool useTinyObjLoader, Device& device, const std::string& filename);

            Model(const Model&) = delete;
            void operator=(const Model&) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffers(const std::vector<Vertex>& vertices);
            void createIndexBuffers(const std::vector<uint32_t>& indices);

            Device& device;

            std::unique_ptr<Buffer> vertexBuffer;
            uint32_t vertexCount;

            bool hasIndexBuffer =  false;
            std::unique_ptr<Buffer> indexBuffer;
            uint32_t indexCount;
    };
}
