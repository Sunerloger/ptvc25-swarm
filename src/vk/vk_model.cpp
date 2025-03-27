#include "vk_model.h"
#include "vk_utils.hpp"
#include "vk_buffer.h"
#include "../asset_utils/AssetManager.h"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
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
	template <>
	struct hash<vk::Model::Vertex> {
		size_t operator()(vk::Model::Vertex const& vertex) const {
			size_t seed = 0;
			vk::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace vk {
	Model::Model(Device& device, const Model::Builder& builder) : device(device) {
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	Model::~Model() {}

	std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filename) {
		Builder builder{};
		builder.loadModel(filename);
		return std::make_unique<Model>(device, builder);
	}

	void Model::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		Buffer stagingBuffer{
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*) vertices.data());

		vertexBuffer = std::make_unique<Buffer>(
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void Model::createIndexBuffers(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;
		if (!hasIndexBuffer) {
			return;
		}
		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
		uint32_t indexSize = sizeof(indices[0]);

		Buffer stagingBuffer{device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*) indices.data());

		indexBuffer = std::make_unique<Buffer>(
			device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void Model::draw(VkCommandBuffer commandBuffer) {
		if (hasIndexBuffer) {
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

		if (hasIndexBuffer) {
			// If model has more than 2^32 vertices, change the index type to uint64_t and the indexType to VK_INDEX_TYPE_UINT64
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

	// change in the vertex struct needs to be reflected here
	std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
		attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
		attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
		attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

		// alternative way of doing the same thing

		// attributeDescriptions[0].binding = 0;
		// attributeDescriptions[0].location = 0;
		// attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		// attributeDescriptions[0].offset = offsetof(Vertex, position);
		//...
		return attributeDescriptions;
	}

	void Model::Builder::loadModelWithoutTinyObjLoader(const std::string& filename) {
		vertices.clear();
		indices.clear();

		std::string resolvedPath = AssetManager::getInstance().resolvePath(filename);

		std::ifstream file(resolvedPath);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + resolvedPath);
		}

		std::string line;
		while (getline(file, line)) {
			std::istringstream iss(line);
			std::string prefix;
			iss >> prefix;

			if (prefix == "v") {  // Vertex position and color
				glm::vec3 vertex;
				glm::vec3 color;
				iss >> vertex.x >> vertex.y >> vertex.z;
				iss >> color.r >> color.g >> color.b;  // Assuming color values are provided after vertex positions
				vertices.push_back({vertex, color, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)});
			} else if (prefix == "l") {	 // Line index
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

	void Model::Builder::loadModel(const std::string& filename) {
		vertices.clear();
		indices.clear();

		std::string resolvedPath = AssetManager::getInstance().resolvePath(filename);

		tinygltf::TinyGLTF loader;
		tinygltf::Model gltfModel;
		std::string err;
		std::string warn;
		bool ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, resolvedPath);

		if (!warn.empty()) {
			std::cout << "Warning: " << warn << std::endl;
		}
		if (!err.empty()) {
			std::cerr << "Error: " << err << std::endl;
		}
		if (!ret) {
			throw std::runtime_error("Failed to load glTF model: " + resolvedPath);
		}

		if (gltfModel.meshes.empty()) {
			throw std::runtime_error("No mesh found in glTF file: " + resolvedPath);
		}

		// For simplicity, we load only the first mesh and iterate its primitives.
		const tinygltf::Mesh& mesh = gltfModel.meshes[0];
		for (const auto& primitive : mesh.primitives) {
			// Load indices if provided.
			if (primitive.indices >= 0) {
				const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
				const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;
				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(dataPtr);
					for (size_t i = 0; i < indexAccessor.count; i++) {
						indices.push_back(buf[i]);
					}
				} else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(dataPtr);
					for (size_t i = 0; i < indexAccessor.count; i++) {
						indices.push_back(buf[i]);
					}
				} else {
					throw std::runtime_error("Unsupported index component type in glTF");
				}
			}

			// Retrieve POSITION attribute (required)
			if (primitive.attributes.find("POSITION") == primitive.attributes.end())
				throw std::runtime_error("No POSITION attribute found in glTF primitive");
			const tinygltf::Accessor& posAccessor = gltfModel.accessors.at(primitive.attributes.find("POSITION")->second);
			const tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];
			const unsigned char* posData = posBuffer.data.data() + posBufferView.byteOffset + posAccessor.byteOffset;

			// Retrieve NORMAL attribute if available
			const tinygltf::Accessor* normAccessor = nullptr;
			const unsigned char* normData = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				normAccessor = &gltfModel.accessors.at(primitive.attributes.find("NORMAL")->second);
				const tinygltf::BufferView& normBufferView = gltfModel.bufferViews[normAccessor->bufferView];
				const tinygltf::Buffer& normBuffer = gltfModel.buffers[normBufferView.buffer];
				normData = normBuffer.data.data() + normBufferView.byteOffset + normAccessor->byteOffset;
			}

			// Retrieve TEXCOORD_0 attribute if available
			const tinygltf::Accessor* uvAccessor = nullptr;
			const unsigned char* uvData = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				uvAccessor = &gltfModel.accessors.at(primitive.attributes.find("TEXCOORD_0")->second);
				const tinygltf::BufferView& uvBufferView = gltfModel.bufferViews[uvAccessor->bufferView];
				const tinygltf::Buffer& uvBuffer = gltfModel.buffers[uvBufferView.buffer];
				uvData = uvBuffer.data.data() + uvBufferView.byteOffset + uvAccessor->byteOffset;
			}

			// Retrieve COLOR_0 attribute if available
			const tinygltf::Accessor* colorAccessor = nullptr;
			const unsigned char* colorData = nullptr;
			if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
				colorAccessor = &gltfModel.accessors.at(primitive.attributes.find("COLOR_0")->second);
				const tinygltf::BufferView& colorBufferView = gltfModel.bufferViews[colorAccessor->bufferView];
				const tinygltf::Buffer& colorBuffer = gltfModel.buffers[colorBufferView.buffer];
				colorData = colorBuffer.data.data() + colorBufferView.byteOffset + colorAccessor->byteOffset;
			}

			// For each vertex in POSITION accessor, build our vertex structure.
			// Note: this simple implementation does not de-duplicate vertices based on indices.
			for (size_t i = 0; i < posAccessor.count; i++) {
				Vertex vertex{};
				const float* pos = reinterpret_cast<const float*>(posData + i * 3 * sizeof(float));
				vertex.position = {pos[0], pos[1], pos[2]};

				if (colorAccessor) {
					// Assume COLOR_0 is a vec3 (or vec4; we ignore alpha here)
					int comps = (colorAccessor->type == TINYGLTF_TYPE_VEC3 ? 3 : 4);
					const float* col = reinterpret_cast<const float*>(colorData + i * comps * sizeof(float));
					vertex.color = {col[0], col[1], col[2]};
				} else {
					vertex.color = {1.0f, 1.0f, 1.0f};
				}

				if (normAccessor) {
					const float* norm = reinterpret_cast<const float*>(normData + i * 3 * sizeof(float));
					vertex.normal = {norm[0], norm[1], norm[2]};
				} else {
					vertex.normal = {0.0f, 0.0f, 0.0f};
				}

				if (uvAccessor) {
					const float* uv = reinterpret_cast<const float*>(uvData + i * 2 * sizeof(float));
					vertex.uv = {uv[0], uv[1]};
				} else {
					vertex.uv = {0.0f, 0.0f};
				}

				vertices.push_back(vertex);
			}
		}
		std::cout << "Loaded glTF model with " << vertices.size() << " vertices and " << indices.size() << " indices" << std::endl;
	}

	// Helper function to check file extension remains unchanged.
	bool EndsWith(const std::string& str, const std::string& suffix) {
		return str.size() >= suffix.size() &&
			   0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
	}
}