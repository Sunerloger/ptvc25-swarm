#include "vk_model.h"
#include "vk_utils.hpp"
#include "vk_buffer.h"
#include "vk_descriptors.h"

#include "../asset_utils/AssetLoader.h"
#include "../rendering/materials/StandardMaterial.h"
#include "../rendering/materials/UIMaterial.h"
#include "../rendering/materials/TessellationMaterial.h"

#include <random>
#include <algorithm>
#include <cmath>

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
	Model::Model(Device& device, const Builder& builder) : device(device) {
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
		if (builder.textureMaterialIndex >= 0) {
			// Create material from GLTF data
			if (builder.isUI) {
				createUIMaterialFromGltf(builder.gltfModelData, builder.textureMaterialIndex);
			} else {
				createStandardMaterialFromGltf(builder.gltfModelData, builder.textureMaterialIndex);
			}
		}
	}

	Model::~Model() {}

	std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filename, bool isUI) {
		Builder builder{};
		builder.isUI = isUI;
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

	void Model::Builder::loadModel(const std::string& filename) {
		vertices.clear();
		indices.clear();

		std::string resolvedPath = AssetLoader::getInstance().resolvePath(filename);

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

		const tinygltf::Mesh& mesh = gltfModel.meshes[0];
		int materialIndex = -1;
		for (const auto& primitive : mesh.primitives) {
			// Load indices.
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

			// POSITION attribute.
			if (primitive.attributes.find("POSITION") == primitive.attributes.end())
				throw std::runtime_error("No POSITION attribute found in glTF primitive");
			const tinygltf::Accessor& posAccessor = gltfModel.accessors.at(primitive.attributes.find("POSITION")->second);
			const tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];
			const unsigned char* posData = posBuffer.data.data() + posBufferView.byteOffset + posAccessor.byteOffset;

			// NORMAL attribute.
			const tinygltf::Accessor* normAccessor = nullptr;
			const unsigned char* normData = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				normAccessor = &gltfModel.accessors.at(primitive.attributes.find("NORMAL")->second);
				const tinygltf::BufferView& normBufferView = gltfModel.bufferViews[normAccessor->bufferView];
				const tinygltf::Buffer& normBuffer = gltfModel.buffers[normBufferView.buffer];
				normData = normBuffer.data.data() + normBufferView.byteOffset + normAccessor->byteOffset;
			}

			// TEXCOORD_0 attribute.
			const tinygltf::Accessor* uvAccessor = nullptr;
			const unsigned char* uvData = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				uvAccessor = &gltfModel.accessors.at(primitive.attributes.find("TEXCOORD_0")->second);
				const tinygltf::BufferView& uvBufferView = gltfModel.bufferViews[uvAccessor->bufferView];
				const tinygltf::Buffer& uvBuffer = gltfModel.buffers[uvBufferView.buffer];
				uvData = uvBuffer.data.data() + uvBufferView.byteOffset + uvAccessor->byteOffset;
			}

			// COLOR_0 attribute.
			const tinygltf::Accessor* colorAccessor = nullptr;
			const unsigned char* colorData = nullptr;
			if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
				colorAccessor = &gltfModel.accessors.at(primitive.attributes.find("COLOR_0")->second);
				const tinygltf::BufferView& colorBufferView = gltfModel.bufferViews[colorAccessor->bufferView];
				const tinygltf::Buffer& colorBuffer = gltfModel.buffers[colorBufferView.buffer];
				colorData = colorBuffer.data.data() + colorBufferView.byteOffset + colorAccessor->byteOffset;
			}

			// Build vertex for each element.
			for (size_t i = 0; i < posAccessor.count; i++) {
				Vertex vertex{};
				const float* pos = reinterpret_cast<const float*>(posData + i * 3 * sizeof(float));

				// Apply a -90 degree rotation around the X-axis to correct the model orientation
				vertex.position = {pos[0], pos[2], -pos[1]};

				if (colorAccessor) {
					int comps = (colorAccessor->type == TINYGLTF_TYPE_VEC3 ? 3 : 4);
					const float* col = reinterpret_cast<const float*>(colorData + i * comps * sizeof(float));
					vertex.color = {col[0], col[1], col[2]};
				} else if (primitive.material >= 0) {
					const tinygltf::Material& material = gltfModel.materials[primitive.material];
					if (material.values.find("baseColorFactor") != material.values.end()) {
						const auto& factor = material.values.at("baseColorFactor").ColorFactor();
						if (factor.size() >= 3) {
							vertex.color = {static_cast<float>(factor[0]), static_cast<float>(factor[1]), static_cast<float>(factor[2])};
						} else {
							vertex.color = {1.0f, 1.0f, 1.0f};
						}
					} else {
						vertex.color = {1.0f, 1.0f, 1.0f};
					}
				} else {
					vertex.color = {1.0f, 1.0f, 1.0f};
				}
				if (normAccessor) {
					const float* norm = reinterpret_cast<const float*>(normData + i * 3 * sizeof(float));

					vertex.normal = {norm[0], norm[2], -norm[1]};
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
			// Use the material index from the first primitive that specifies one.
			if (primitive.material >= 0 && materialIndex == -1) {
				materialIndex = primitive.material;
			}
		}
		std::cout << "Loaded glTF model with " << vertices.size() << " vertices and " << indices.size() << " indices" << std::endl;

		// Store glTF data and material index in builder for later texture creation.
		gltfModelData = gltfModel;
		textureMaterialIndex = materialIndex;
	}

	void Model::createUIMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex) {
		// Create a StandardMaterial for the model
		if (materialIndex >= 0 && materialIndex < gltfModel.materials.size()) {
			const tinygltf::Material& gltfMaterial = gltfModel.materials[materialIndex];

			// Check if material has a texture
			std::string texturePath;
			bool hasTexture = false;
			bool hasEmbeddedTexture = false;
			int embeddedWidth = 0, embeddedHeight = 0, embeddedChannels = 0;
			std::vector<unsigned char> embeddedImageData;

			if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
				int textureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
				if (textureIndex >= 0 && textureIndex < gltfModel.textures.size()) {
					const tinygltf::Texture& gltfTexture = gltfModel.textures[textureIndex];
					int imageIndex = gltfTexture.source;
					if (imageIndex >= 0 && imageIndex < gltfModel.images.size()) {
						const tinygltf::Image& gltfImage = gltfModel.images[imageIndex];

						// Check if the image has a URI (external texture)
						if (!gltfImage.uri.empty()) {
							// Use AssetManager to resolve the texture path
							texturePath = AssetLoader::getInstance().resolvePath(gltfImage.uri);
							std::cout << "Loading external texture from: " << texturePath << std::endl;
							hasTexture = true;
						}
						// Check if the image has embedded data
						else if (!gltfImage.image.empty()) {
							std::cout << "Found embedded texture in GLTF file, size: " << gltfImage.image.size()
									  << " bytes, dimensions: " << gltfImage.width << "x" << gltfImage.height
									  << ", components: " << gltfImage.component << std::endl;

							// Store the embedded image data for direct use
							embeddedImageData = gltfImage.image;
							embeddedWidth = gltfImage.width;
							embeddedHeight = gltfImage.height;
							embeddedChannels = gltfImage.component;
							hasEmbeddedTexture = true;
						}
					}
				}
			}

			if (hasTexture) {
				// Create material with external texture
				material = std::make_shared<UIMaterial>(device, texturePath);
			} else if (hasEmbeddedTexture) {
				// Create material directly from embedded texture data
				material = std::make_shared<UIMaterial>(
					device,
					embeddedImageData,
					embeddedWidth,
					embeddedHeight,
					embeddedChannels);
				std::cout << "Created material from embedded texture data" << std::endl;
			} else {
				// No baseColorTexture → embed a 1×1 white pixel so we can still sample
				std::vector<unsigned char> whitePixel = {255, 255, 255, 255};
				material = std::make_shared<UIMaterial>(device, whitePixel, 1, 1, 4);
				std::cout << "Using embedded white pixel for vertex-color fallback\n";
			}
		} else {
			// Create a default material if no material is specified
			material = std::make_shared<UIMaterial>(device, "textures:missing.png");
			std::cout << "No material specified, using default texture" << std::endl;
		}
	}

	void Model::createStandardMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex) {
		// Create a StandardMaterial for the model
		if (materialIndex >= 0 && materialIndex < gltfModel.materials.size()) {
			const tinygltf::Material& gltfMaterial = gltfModel.materials[materialIndex];

			// Check if material has a texture
			std::string texturePath;
			bool hasTexture = false;
			bool hasEmbeddedTexture = false;
			int embeddedWidth = 0, embeddedHeight = 0, embeddedChannels = 0;
			std::vector<unsigned char> embeddedImageData;

			if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
				int textureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
				if (textureIndex >= 0 && textureIndex < gltfModel.textures.size()) {
					const tinygltf::Texture& gltfTexture = gltfModel.textures[textureIndex];
					int imageIndex = gltfTexture.source;
					if (imageIndex >= 0 && imageIndex < gltfModel.images.size()) {
						const tinygltf::Image& gltfImage = gltfModel.images[imageIndex];

						// Check if the image has a URI (external texture)
						if (!gltfImage.uri.empty()) {
							// Use AssetManager to resolve the texture path
							texturePath = AssetLoader::getInstance().resolvePath(gltfImage.uri);
							std::cout << "Loading external texture from: " << texturePath << std::endl;
							hasTexture = true;
						}
						// Check if the image has embedded data
						else if (!gltfImage.image.empty()) {
							std::cout << "Found embedded texture in GLTF file, size: " << gltfImage.image.size()
									  << " bytes, dimensions: " << gltfImage.width << "x" << gltfImage.height
									  << ", components: " << gltfImage.component << std::endl;

							// Store the embedded image data for direct use
							embeddedImageData = gltfImage.image;
							embeddedWidth = gltfImage.width;
							embeddedHeight = gltfImage.height;
							embeddedChannels = gltfImage.component;
							hasEmbeddedTexture = true;
						}
					}
				}
			}

			if (hasTexture) {
				// Create material with external texture
				material = std::make_shared<StandardMaterial>(device, texturePath);
			} else if (hasEmbeddedTexture) {
				// Create material directly from embedded texture data
				material = std::make_shared<StandardMaterial>(
					device,
					embeddedImageData,
					embeddedWidth,
					embeddedHeight,
					embeddedChannels);
				std::cout << "Created material from embedded texture data" << std::endl;
			} else {
				// Create a default material with a solid color
				// For now, we'll use a white texture as a fallback
				material = std::make_shared<StandardMaterial>(device, "textures:missing.png");
				std::cout << "Using default texture: textures:missing.png" << std::endl;

				// Set color from material if available
				if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() >= 3) {
					// Note: In a more complete implementation, we would set the color
					// as a uniform in the material
					std::cout << "Material has color factor: "
							  << gltfMaterial.pbrMetallicRoughness.baseColorFactor[0] << ", "
							  << gltfMaterial.pbrMetallicRoughness.baseColorFactor[1] << ", "
							  << gltfMaterial.pbrMetallicRoughness.baseColorFactor[2] << std::endl;
				}
			}
		} else {
			// Create a default material if no material is specified
			material = std::make_shared<StandardMaterial>(device, "textures:missing.png");
			std::cout << "No material specified, using default texture" << std::endl;
		}
	}

	// Helper function to check file extension remains unchanged.
	bool EndsWith(const std::string& str, const std::string& suffix) {
		return str.size() >= suffix.size() &&
			   0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
	}

	std::unique_ptr<Model> Model::createCubeModel(Device& device) {
		Builder builder{};

		const float size = 1.0f;

		// position, color, normal, uv
		std::vector<Model::Vertex> vertices = {
			// front face
			{{-size, -size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{size, -size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{size, size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-size, size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

			// back face
			{{-size, -size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
			{{-size, size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
			{{size, size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
			{{size, -size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

			// top face
			{{-size, size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{-size, size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
			{{size, size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			{{size, size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},

			// bottom face
			{{-size, -size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
			{{size, -size, -size}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
			{{size, -size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
			{{-size, -size, size}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

			// right face
			{{size, -size, -size}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{size, size, -size}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{size, size, size}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			{{size, -size, size}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

			// left face
			{{-size, -size, -size}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{-size, -size, size}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{-size, size, size}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
			{{-size, size, -size}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}};

		std::vector<uint32_t> indices = {
			// front face
			0, 1, 2, 2, 3, 0,
			// back face
			4, 5, 6, 6, 7, 4,
			// top face
			8, 9, 10, 10, 11, 8,
			// bottom face
			12, 13, 14, 14, 15, 12,
			// right face
			16, 17, 18, 18, 19, 16,
			// left face
			20, 21, 22, 22, 23, 20};

		builder.vertices = std::move(vertices);
		builder.indices = std::move(indices);

		return std::make_unique<Model>(device, builder);
	}

	// Helper function for Perlin noise generation
	float fade(float t) {
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	float lerp(float a, float b, float t) {
		return a + t * (b - a);
	}

	float grad(int hash, float x, float y, float z) {
		// Convert lower 4 bits of hash code into 12 gradient directions
		int h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : h == 12 || h == 14 ? x
												 : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}

	float perlinNoise(float x, float y, const std::vector<int>& p) {
		// Find unit grid cell containing point
		int X = static_cast<int>(std::floor(x)) & 255;
		int Y = static_cast<int>(std::floor(y)) & 255;

		// Get relative xy coordinates of point within cell
		x -= std::floor(x);
		y -= std::floor(y);

		// Compute fade curves for each coordinate
		float u = fade(x);
		float v = fade(y);

		// Hash coordinates of the 4 square corners
		int A = p[X] + Y;
		int AA = p[A];
		int AB = p[A + 1];
		int B = p[X + 1] + Y;
		int BA = p[B];
		int BB = p[B + 1];

		// Add blended results from 4 corners of square
		return lerp(
			lerp(grad(p[AA], x, y, 0), grad(p[BA], x - 1, y, 0), u),
			lerp(grad(p[AB], x, y - 1, 0), grad(p[BB], x - 1, y - 1, 0), u),
			v);
	}

	std::pair<std::unique_ptr<Model>, std::vector<float>> Model::createTerrainModel(
		Device& device,
		int gridSize,
		const std::string& tileTexturePath,
		float noiseScale,
		float heightScale) {
		// Create a vector to store the heightmap data
		std::vector<float> heightData(gridSize * gridSize);
		std::vector<unsigned char> imageData(gridSize * gridSize * 4);	// RGBA format (this only enables png for now)

		// Initialize permutation table for Perlin noise
		std::vector<int> p(512);
		for (int i = 0; i < 256; i++) {
			p[i] = i;
		}

		// Use a random seed for the shuffle
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(p.begin(), p.begin() + 256, g);

		// Duplicate the permutation to avoid overflow
		for (int i = 0; i < 256; i++) {
			p[i + 256] = p[i];
		}

		// Generate heightmap using Perlin noise
		for (int z = 0; z < gridSize; z++) {
			for (int x = 0; x < gridSize; x++) {
				float nx = x * noiseScale / gridSize;
				float nz = z * noiseScale / gridSize;

				// Use multiple octaves for more natural terrain
				float h = 0.0f;
				float amplitude = 1.0f;
				float frequency = 1.0f;
				float maxValue = 0.0f;

				for (int i = 0; i < 4; i++) {  // Use 4 octaves
					h += perlinNoise(nx * frequency, nz * frequency, p) * amplitude;
					maxValue += amplitude;
					amplitude *= 0.5f;
					frequency *= 2.0f;
				}

				// normalize to [-1, 1]
				h /= maxValue;

				// Store in heightmap
				int index = z * gridSize + x;
				heightData[index] = h;

				// Convert height to grayscale for the image (0-255)
				unsigned char value = static_cast<unsigned char>((h * 0.5f + 0.5f) * 255);
				imageData[index * 4] = value;	   // R
				imageData[index * 4 + 1] = value;  // G
				imageData[index * 4 + 2] = value;  // B
				imageData[index * 4 + 3] = 255;	   // A (fully opaque)
			}
		}

		// Save the heightmap using the AssetManager
		std::string heightmapPath = "terrain/temp_heightmap.png";
		std::string texturePath = AssetLoader::getInstance().saveTexture(
			heightmapPath,
			imageData.data(),
			gridSize,
			gridSize,
			4  // RGBA format
		);

		if (texturePath.empty()) {
			std::cerr << "Failed to save heightmap texture!" << std::endl;
		} else {
			std::cout << "Generated heightmap texture: " << texturePath << std::endl;
		}

		// Create a grid model
		Builder builder{};

		// Ensure gridSize is at least 2x2
		if (gridSize < 2)
			gridSize = 2;

		// Calculate the number of vertices and indices
		int numVertices = gridSize * gridSize;
		int numIndices = (gridSize - 1) * (gridSize - 1) * 6;  // 2 triangles per grid cell

		// Create vertices
		std::vector<Model::Vertex> vertices;
		vertices.reserve(numVertices);

		// Size of the grid (from -1 to 1 in both x and z)
		const float size = 1.0f;

		// Calculate the step size
		float step = (2.0f * size) / (gridSize - 1);

		// Generate vertices
		for (int z = 0; z < gridSize; z++) {
			for (int x = 0; x < gridSize; x++) {
				float xPos = -size + x * step;
				float zPos = -size + z * step;

				// offset applied in shader based on heightmap
				glm::vec3 position = {xPos, 0.0f, zPos};

				glm::vec3 color = {1.0f, 1.0f, 1.0f};

				// Calculate normal based on neighboring heights
				glm::vec3 normal = {0.0f, 1.0f, 0.0f};	// Default to up

				// If not on the edge, calculate normal from neighboring vertices
				if (x > 0 && x < gridSize - 1 && z > 0 && z < gridSize - 1) {
					float hL = heightData[z * gridSize + (x - 1)];	// Left
					float hR = heightData[z * gridSize + (x + 1)];	// Right
					float hD = heightData[(z - 1) * gridSize + x];	// Down
					float hU = heightData[(z + 1) * gridSize + x];	// Up

					// Calculate normal using central differences
					glm::vec3 tangent = glm::normalize(glm::vec3(2.0f * step, hR - hL, 0.0f));
					glm::vec3 bitangent = glm::normalize(glm::vec3(0.0f, hU - hD, 2.0f * step));
					normal = glm::normalize(glm::cross(tangent, bitangent));
				}

				// UV coordinates (tiled)
				// Map UV from 0 to gridSize to create tiling effect
				glm::vec2 uv = {
					static_cast<float>(x) / (gridSize - 1) * 4.0f,	// Tile 4 times
					static_cast<float>(z) / (gridSize - 1) * 4.0f	// Tile 4 times
				};

				vertices.push_back({position, color, normal, uv});
			}
		}

		// Create indices
		std::vector<uint32_t> indices;
		indices.reserve(numIndices);

		// Generate indices for triangles
		for (int z = 0; z < gridSize - 1; z++) {
			for (int x = 0; x < gridSize - 1; x++) {
				// Calculate the indices of the four corners of the current grid cell
				uint32_t topLeft = z * gridSize + x;
				uint32_t topRight = topLeft + 1;
				uint32_t bottomLeft = (z + 1) * gridSize + x;
				uint32_t bottomRight = bottomLeft + 1;

				// counter-clockwise
				indices.push_back(topLeft);
				indices.push_back(bottomRight);
				indices.push_back(bottomLeft);

				// counter-clockwise
				indices.push_back(topLeft);
				indices.push_back(topRight);
				indices.push_back(bottomRight);
			}
		}

		std::cout << "Created terrain model with " << vertices.size() << " vertices and "
				  << indices.size() << " indices" << std::endl;

		builder.vertices = std::move(vertices);
		builder.indices = std::move(indices);

		// Create the model
		auto model = std::make_unique<Model>(device, builder);

		// Create a material with the tile texture and heightmap
		auto material = std::make_shared<TessellationMaterial>(
			device,
			tileTexturePath,
			texturePath,  // Use the path returned by the AssetLoader
			"terrain_shader.vert",
			"terrain_shader.frag",
			"terrain_tess_control.tesc",
			"terrain_tess_eval.tese");

		// Set tessellation parameters
		material->setTileScale(0.25f, 0.25f);  // Control texture tiling
		material->setTessellationParams(16.0f, 20.0f, 100.0f, heightScale);

		// Enable tessellation
		auto& config = material->getPipelineConfig();
		config.useTessellation = true;

		// Apply the material to the model
		model->setMaterial(material);

		return {std::move(model), heightData};
	}

	std::unique_ptr<Model> Model::createGridModel(Device& device, int gridSize) {
		Builder builder{};

		// Ensure gridSize is at least 2x2
		if (gridSize < 2)
			gridSize = 2;

		// Calculate the number of vertices and indices
		int numVertices = gridSize * gridSize;
		int numIndices = (gridSize - 1) * (gridSize - 1) * 6;  // 2 triangles per grid cell

		// Create vertices
		std::vector<Model::Vertex> vertices;
		vertices.reserve(numVertices);

		// Size of the grid (from -1 to 1 in both x and z)
		const float size = 1.0f;

		// Calculate the step size
		float step = (2.0f * size) / (gridSize - 1);

		// Generate vertices
		for (int z = 0; z < gridSize; z++) {
			for (int x = 0; x < gridSize; x++) {
				float xPos = -size + x * step;
				float zPos = -size + z * step;

				// Position (centered at origin)
				glm::vec3 position = {xPos, 0.0f, zPos};

				// Color (white)
				glm::vec3 color = {1.0f, 1.0f, 1.0f};

				// Normal (pointing up)
				glm::vec3 normal = {0.0f, 1.0f, 0.0f};

				// UV coordinates (tiled)
				// Map UV from 0 to gridSize to create tiling effect
				glm::vec2 uv = {
					static_cast<float>(x) / (gridSize - 1) * gridSize,
					static_cast<float>(z) / (gridSize - 1) * gridSize};

				vertices.push_back({position, color, normal, uv});
			}
		}

		// Create indices
		std::vector<uint32_t> indices;
		indices.reserve(numIndices);

		// Generate indices for triangles
		for (int z = 0; z < gridSize - 1; z++) {
			for (int x = 0; x < gridSize - 1; x++) {
				// Calculate the indices of the four corners of the current grid cell
				uint32_t topLeft = z * gridSize + x;
				uint32_t topRight = topLeft + 1;
				uint32_t bottomLeft = (z + 1) * gridSize + x;
				uint32_t bottomRight = bottomLeft + 1;

				// First triangle (top-left, bottom-left, bottom-right)
				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);

				// Second triangle (top-left, bottom-right, top-right)
				indices.push_back(topLeft);
				indices.push_back(bottomRight);
				indices.push_back(topRight);
			}
		}

		std::cout << "Created grid model with " << vertices.size() << " vertices and "
				  << indices.size() << " indices" << std::endl;

		builder.vertices = std::move(vertices);
		builder.indices = std::move(indices);

		return std::make_unique<Model>(device, builder);
	}
}