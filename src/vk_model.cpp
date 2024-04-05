//
// Created by Vlad Dancea on 28.03.24.
//

#include "vk_model.h"
#include "vk_utils.hpp"
#include "vk_buffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


#include <cassert>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>


namespace std {
    template<>
    struct hash<vk::Model::Vertex> {
        size_t operator()(vk::Model::Vertex const& vertex) const {
            size_t seed = 0;
            vk::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}

namespace vk{
    Model::Model(Device& device, const Model::Builder& builder) : device(device){
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }

    Model::~Model(){}

    std::unique_ptr<Model> Model::createModelFromFile(bool useTinyObjLoader, Device& device, const std::string& filename) {
        Builder builder{};
        if(useTinyObjLoader){
            builder.loadModel(filename);
        } else {
            builder.loadModelWithoutTinyObjLoader(filename);
        }
        return std::make_unique<Model>(device, builder);
    }

    void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        Buffer stagingBuffer{device,
                             vertexSize,
                             vertexCount,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)vertices.data());

        vertexBuffer = std::make_unique<Buffer>(
                device,
                vertexSize,
                vertexCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    void Model::createIndexBuffers(const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;
        if(!hasIndexBuffer){
            return;
        }
        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        Buffer stagingBuffer{device,
                             indexSize,
                             indexCount,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)indices.data());

        indexBuffer = std::make_unique<Buffer>(
                device,
                indexSize,
                indexCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


        device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void Model::draw(VkCommandBuffer commandBuffer) {
        if(hasIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
            return;
        } else {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void Model::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if(hasIndexBuffer){
            //If model has more than 2^32 vertices, change the index type to uint64_t and the indexType to VK_INDEX_TYPE_UINT64
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    //change in the vertex struct needs to be reflected here
    std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, position)});
        attributeDescriptions.push_back({1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, color)});
        attributeDescriptions.push_back({2,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex, uv)});

        //alternative way of doing the same thing

        //attributeDescriptions[0].binding = 0;
        //attributeDescriptions[0].location = 0;
        //attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        //attributeDescriptions[0].offset = offsetof(Vertex, position);
        //...
        return attributeDescriptions;
    }

    void Model::Builder::loadModelWithoutTinyObjLoader(const std::string &filename) {
        vertices.clear();
        indices.clear();

        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::string line;
        while (getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") { // Vertex position and color
                glm::vec3 vertex;
                glm::vec3 color;
                iss >> vertex.x >> vertex.y >> vertex.z;
                iss >> color.r >> color.g >> color.b; // Assuming color values are provided after vertex positions
                vertices.push_back({vertex, color, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)});
            } else if (prefix == "l") { // Line index
                uint32_t index1, index2;
                iss >> index1 >> index2;

                // OBJ file indices start from 1, but we need them to start from 0
                index1 -= 1;
                index2 -= 1;

                indices.push_back(index1);
                indices.push_back(index2);
            }
        }

        std::cout << "Loaded model with " << vertices.size() << " vertices and " << indices.size() << " indices (from lines)" << std::endl;
    }


    void Model::Builder::loadModel(const std::string &filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())){
            throw std::runtime_error(warn + err);
        }

        vertices.clear();
        indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for(const auto& shape : shapes){
            for(const auto& index : shape.mesh.indices){
                Vertex vertex{};

                if(index.vertex_index >= 0) {
                    vertex.position = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2]
                    };
                }

                if(index.normal_index >= 0) {
                    vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if(index.texcoord_index >= 0) {
                    vertex.uv = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if(uniqueVertices.count(vertex) == 0){
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
        std::cout << "Loaded model with " << vertices.size() << " vertices and " << indices.size() << " indices" << std::endl;
    }
}