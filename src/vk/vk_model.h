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

			bool dynamic = false;

			glm::vec3 boundsMin{ std::numeric_limits<float>::max() };
			glm::vec3 boundsMax{ -std::numeric_limits<float>::max() };

			bool isUI = false;
			void loadModel(const std::string& filename);
		};

		Model(Device& device, const Model::Builder& builder);
		~Model();
		Model(const Model&) = delete;
		void operator=(const Model&) = delete;

		void setMaterial(std::shared_ptr<Material> material) { this->material = material; }
		std::shared_ptr<Material> getMaterial() const { return material; }

		DescriptorSet getMaterialDescriptorSet(int frameIndex) const {
			return material->getDescriptorSet(frameIndex);
		}

		static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filename, bool isUI = false);
		static std::unique_ptr<Model> createCubeModel(Device& device);
		static std::unique_ptr<Model> createGridModel(Device& device, int gridSize);
		static std::unique_ptr<Model> createGridModelWithoutGeometry(Device& device, int samplesPerSide);

		static std::unique_ptr<Model> createWaterModel(Device& device, int samplesPerSide, std::vector<glm::vec4> waves);

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

		std::pair<glm::vec3, glm::vec3> getAABB() const { return { m_boundsMin, m_boundsMax }; }

		void updateMesh(const std::vector<Vertex>& newVerts, const std::vector<uint32_t>& newIdx);

		uint32_t patchCount = 0;
		uint32_t pointsPerPatch = 0;

	   private:
		void createVertexBuffer(const std::vector<Vertex>& vertices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);

		void createVertexBuffer(size_t elementCount);
		void createIndexBuffer(size_t elementCount);

		void createStandardMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex);
		void createUIMaterialFromGltf(const tinygltf::Model& gltfModel, int materialIndex);

		Device& device;
		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		bool hasVertexBuffer = false;
		bool hasIndexBuffer = false;

		bool m_dynamic = false;
		VkMemoryPropertyFlags m_memFlags = 0;

		// buffer capacities
		size_t vertexCapacityElements = 0;
		size_t indexCapacityElements = 0;

		std::shared_ptr<Material> material;

		glm::vec3 m_boundsMin{ std::numeric_limits<float>::max() };
		glm::vec3 m_boundsMax{ -std::numeric_limits<float>::max() };
	};

}  // namespace vk