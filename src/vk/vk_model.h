#pragma once

#include "vk_device.h"
#include "vk_buffer.h"
#include "vk_descriptors.h"
#include "../rendering/materials/Material.h"
#include "../rendering/materials/TessellationMaterial.h"

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "tiny_gltf.h"

namespace vk {

	class Model {
	   public:
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec2 uv;
			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
			bool operator==(const Vertex& other) const {
				return position == other.position &&
					   color == other.color &&
					   normal == other.normal &&
					   uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			// For texture loading from glTF:
			tinygltf::Model gltfModelData;
			int textureMaterialIndex = -1;

			bool isUI = false;
			void loadModel(const std::string& filename);
		};

		Model(Device& device, const Model::Builder& builder);
		~Model();
		Model(const Model&) = delete;
		void operator=(const Model&) = delete;

		void setMaterial(std::shared_ptr<Material> material) { this->material = material; }
		std::shared_ptr<Material> getMaterial() const { return material; }

		VkDescriptorSet getMaterialDescriptorSet(int frameIndex) const {
			return material ? material->getDescriptorSet(frameIndex) : VK_NULL_HANDLE;
		}

		static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filename, bool isUI = false);
		static std::unique_ptr<Model> createCubeModel(Device& device);
		static std::unique_ptr<Model> createGridModel(Device& device, int gridSize);
		static std::unique_ptr<Model> createGridModelWithoutGeometry(Device& device, int samplesPerSide);
		
		// Generate a heightmap texture and return both the model with the heightmap and the height data
		static std::pair<std::unique_ptr<Model>, std::vector<float>> createTerrainModel(
			Device& device,
			int gridSize,
			const std::string& tileTexturePath,
			float noiseScale = 1.0f,
			bool loadHeightTexture = false,
			const std::string& heightTexturePath = "none",
			int seed = -1, // if -1: use random
			bool useTessellation = true,
			TessellationMaterial::MaterialCreationData creationData = {});

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

		uint32_t patchCount = 0;
		uint32_t pointsPerPatch = 0;

	   private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		void createStandardMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex);
		void createUIMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex);

		Device& device;
		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		bool hasVertexBuffer = false;
		bool hasIndexBuffer = false;

		std::shared_ptr<Material> material;
	};

}  // namespace vk