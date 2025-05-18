#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <filesystem>

#include "tiny_obj_loader.h"
#include "stb_image.h"
#include "stb_image_write.h"

namespace fs = std::filesystem;


namespace vk {

	class AssetLoader {
	   public:

		bool debugText = false;

		static AssetLoader& getInstance();

		struct TextureData { std::vector<unsigned char> pixels; int width, height, channels; };
		
		void initialize(const std::string& exePath);

		std::string getPath(const std::string& pathKey) const;
		std::string normalizePath(const std::string& messyPath) const;
		void registerPath(const std::string& key, const std::string& relativePath);

		// Resolve a path to a file
		// If forSaving is true, the file doesn't need to exist (used when saving files)
		std::string resolvePath(const std::string& filepath, bool forSaving = false) const;

		bool endsWith(const std::string& str, const std::string& suffix) const;

		std::vector<char> readFile(const std::string& filepath, bool isBinary = true) const;

		std::vector<char> loadShader(const std::string& shaderName) const;

		std::string getExecutableDir() const {
			return m_executableDir.string();
		}

		std::string getAssetPath(const std::string& assetRelativePath) const {
			return (m_executableDir / "assets" / assetRelativePath).string();
		}

		bool loadOBJModel(const std::string& filepath,
			tinyobj::attrib_t& attrib,
			std::vector<tinyobj::shape_t>& shapes,
			std::vector<tinyobj::material_t>& materials) const;

		// save a texture to a file in the generated directory
		// returns the path that can be used to load the texture later
		std::string saveTexture(const std::string& filename, const unsigned char* data, int width, int height, int channels);
		TextureData loadTexture(const std::string& filepath);
		
		// save a text file to the generated directory
		// returns the path that can be used to load the file later
		std::string saveTxtFile(const std::string& filename, const std::string& content);
		std::string readTxtFile(const std::string& filepath);

	private:
		AssetLoader() = default;
		~AssetLoader() = default;
		AssetLoader(const AssetLoader&) = delete;
		AssetLoader& operator=(const AssetLoader&) = delete;

		fs::path m_executableDir;
		std::unordered_map<std::string, std::string> m_pathRegistry;
	};
}