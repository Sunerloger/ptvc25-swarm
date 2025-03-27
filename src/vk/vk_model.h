#pragma once

#include "vk_device.h"
#include "vk_buffer.h"
#include "vk_descriptors.h"

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

			void loadModel(const std::string& filename);
		};

		Model(Device& device, const Model::Builder& builder);
		~Model();
		Model(const Model&) = delete;
		void operator=(const Model&) = delete;

		static VkDescriptorSetLayout textureDescriptorSetLayout;
		static std::unique_ptr<DescriptorPool> textureDescriptorPool;

		bool hasTexture() const {
			return _hasTexture;
		}
		VkDescriptorSet getTextureDescriptorSet() const {
			return textureDescriptorSet;
		}

		static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filename);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	   private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		void createTextureResources(const tinygltf::Model& gltfModel, int materialIndex, const std::string& modelPath);

		Device& device;
		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
		bool hasIndexBuffer = false;

		bool _hasTexture = false;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		VkDescriptorSet textureDescriptorSet;
	};

}  // namespace vk