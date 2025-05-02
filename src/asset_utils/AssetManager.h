#pragma once

// Only include those macros once - every other time just include tiny_gltf.h
// #define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION

// #include "tiny_gltf.h"
#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

namespace vk {

	class AssetManager {
	   public:
		bool debugText = false;
		static AssetManager& getInstance() {
			static AssetManager instance;
			return instance;
		}

		void initialize(const std::string& exePath) {
			fs::path executablePath = fs::path(exePath);
			m_executableDir = executablePath.parent_path();

			if (debugText)
				std::cout << "AssetManager: Executable directory: " << m_executableDir << std::endl;

			// Store relative paths
			registerPath("base", "");
			registerPath("assets", "assets");
			registerPath("models", "assets/models");
			registerPath("shaders", "assets/shaders_vk");
			registerPath("textures", "assets/textures");
			registerPath("settings", "assets/settings");
			registerPath("compiledShaders", "assets/shaders_vk/compiled");

// Add project source directory if defined
#ifdef PROJECT_SOURCE_DIR
			registerPath("project", PROJECT_SOURCE_DIR);
			registerPath("projectAssets", std::string(PROJECT_SOURCE_DIR) + "/assets");
			registerPath("projectModels", std::string(PROJECT_SOURCE_DIR) + "/assets/models");
			registerPath("projectShaders", std::string(PROJECT_SOURCE_DIR) + "/assets/shaders_vk");
			registerPath("projectTextures", std::string(PROJECT_SOURCE_DIR) + "/assets/textures");
			registerPath("projectSettings", std::string(PROJECT_SOURCE_DIR) + "/assets/settings");
#endif

			registerPath("buildShaders", (m_executableDir / "assets/shaders_vk/compiled").string());

			// Print registered paths for debugging
			for (const auto& [key, path] : m_pathRegistry) {
				if (debugText)
					std::cout << "AssetManager: Registered path '" << key << "': " << path << std::endl;
				if (fs::exists(path)) {
					if (debugText)
						std::cout << "  Directory exists" << std::endl;
				} else {
					if (debugText)
						std::cout << "  Directory does not exist" << std::endl;
				}
			}
		}

		std::string getPath(const std::string& pathKey) const {
			auto it = m_pathRegistry.find(pathKey);
			if (it != m_pathRegistry.end()) {
				return it->second;
			}
			return "";
		}

		std::string normalizePath(const std::string& messyPath) const {
			std::filesystem::path path(messyPath);
			std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
			std::string npath = canonicalPath.make_preferred().string();
			return npath;
		}

		void registerPath(const std::string& key, const std::string& relativePath) {
			m_pathRegistry[key] = normalizePath((m_executableDir / relativePath).string());
		}

		std::string resolvePath(const std::string& filepath) const {
			if (fs::exists(filepath)) {
				return filepath;
			}

			// Check for a registry prefix (e.g. "models:my_model.obj")
			size_t colonPos = filepath.find(':');
			if (colonPos != std::string::npos) {
				std::string pathKey = filepath.substr(0, colonPos);
				std::string filename = filepath.substr(colonPos + 1);
				std::string fullPath = normalizePath(getPath(pathKey) + "/" + filename);
				if (fs::exists(fullPath)) {
					return fullPath;
				}
			}

			// Check for problems with '/' '\' '\\'
			std::string normalized_filepath = normalizePath(filepath);
			if (fs::exists(normalized_filepath)) {
				return normalized_filepath;
			}

			// Try each registered path as a last resort
			for (const auto& [key, basePath] : m_pathRegistry) {
				std::string fullPath = normalizePath(basePath + "/" + filepath);
				if (fs::exists(fullPath)) {
					return fullPath;
				}
			}

			// Couldn't find the file, but return best guess for error reporting
			return filepath;
		}

		bool endsWith(const std::string& str, const std::string& suffix) const {
			if (suffix.size() > str.size()) {
				return false;
			}
			return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		std::vector<char> readFile(const std::string& filepath, bool isBinary = true) const {
			std::string resolvedPath = resolvePath(filepath);

			std::ifstream file(resolvedPath,
				std::ios::ate | (isBinary ? std::ios::binary : std::ios::in));

			if (!file.is_open()) {
				std::string errorMsg = "AssetManager: Failed to open file: " + resolvedPath;
				std::cerr << errorMsg << std::endl;

				std::cerr << "Attempted paths:" << std::endl;
				for (const auto& [key, path] : m_pathRegistry) {
					std::cerr << "  " << normalizePath(path + "/" + filepath) << std::endl;
				}

				throw std::runtime_error(errorMsg);
			}

			// Read the file
			size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			if (debugText)
				std::cout << "AssetManager: Successfully read file: " << resolvedPath << " (" << fileSize << " bytes)" << std::endl;
			return buffer;
		}

		std::vector<char> loadShader(const std::string& shaderName) const {
			// Try several possible locations and naming conventions
			std::vector<std::string> attempts;

			if (!endsWith(shaderName, ".spv")) {
				attempts.push_back("compiledShaders:" + shaderName + ".spv");
				attempts.push_back("buildShaders:" + shaderName + ".spv");
				attempts.push_back(shaderName + ".spv");
			}

			attempts.push_back(shaderName);
			attempts.push_back("compiledShaders:" + shaderName);
			attempts.push_back("buildShaders:" + shaderName);
			attempts.push_back("shaders:" + shaderName);

			for (const auto& attempt : attempts) {
				try {
					return readFile(attempt);
				} catch (const std::exception&) {
					continue;
				}
			}

			std::string errorMsg = "AssetManager: Failed to load shader: " + shaderName;
			std::cerr << errorMsg << std::endl;
			std::cerr << "Attempted paths: ";
			for (const auto& path : attempts) {
				std::cerr << path << ", ";
			}
			std::cerr << std::endl;
			throw std::runtime_error(errorMsg);
		}

		std::string getExecutableDir() const {
			return m_executableDir.string();
		}

		std::string getAssetPath(const std::string& assetRelativePath) const {
			return (m_executableDir / "assets" / assetRelativePath).string();
		}

		bool loadOBJModel(const std::string& filepath,
			tinyobj::attrib_t& attrib,
			std::vector<tinyobj::shape_t>& shapes,
			std::vector<tinyobj::material_t>& materials) const {
			std::string resolvedPath = resolvePath(filepath);
			std::string err;

			if (debugText)
				std::cout << "AssetManager: Loading OBJ model: " << resolvedPath << std::endl;

			bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, resolvedPath.c_str());

			if (!err.empty()) {
				std::cerr << "AssetManager: OBJ Error: " << err << std::endl;
			}

			if (ret) {
				if (debugText)
					std::cout << "AssetManager: Successfully loaded OBJ model with "
							  << shapes.size() << " shapes and "
							  << materials.size() << " materials" << std::endl;
			}

			return ret;
		}

		// GLTF model loading (placeholder for future implementation)
		//        bool loadGLTFModel(const std::string& filepath, tinygltf::Model& model) {
		// TODO
		//        }

		// Texture loading structure
		struct TextureData {
		    std::vector<unsigned char> pixels;
		    int width = 0;
		    int height = 0;
		    int channels = 0;
		};

		// Load a texture from a file
		TextureData loadTexture(const std::string& filepath) {
		    TextureData result;
		    std::string resolvedPath = resolvePath(filepath);
		    
		    // Load the image
		    #include "stb_image.h"
		    unsigned char* data = stbi_load(resolvedPath.c_str(), &result.width, &result.height, &result.channels, 0);
		    
		    if (!data) {
		        throw std::runtime_error("Failed to load texture: " + resolvedPath);
		    }
		    
		    // Copy the data to our vector
		    size_t dataSize = result.width * result.height * result.channels;
		    result.pixels.resize(dataSize);
		    std::memcpy(result.pixels.data(), data, dataSize);
		    
		    // Free the original data
		    stbi_image_free(data);
		    
		    return result;
		}

	   private:
		AssetManager() = default;
		~AssetManager() = default;

		fs::path m_executableDir;
		std::unordered_map<std::string, std::string> m_pathRegistry;
	};
}