#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <memory>
#include <vector>

#include "../ui/Font.h"
#include "../vk/vk_model.h"
#include "../rendering/materials/Material.h"


namespace vk {

	class AssetManager {

	public:

		using ModelPtr = std::shared_ptr<vk::Model>;
		using MaterialPtr = std::shared_ptr<Material>;
		using HeightMap = std::vector<float>;
		struct TextureData { std::vector<unsigned char> pixels; int width, height, channels; };

		using AssetVariant = std::variant<
			ModelPtr,
			MaterialPtr,
			HeightMap,
			TextureData,
			Font
		>;

		AssetManager() = default;
		~AssetManager() = default;
		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;

		template<typename T>
		void add(const std::string& key, T asset) {
			m_assets.emplace_or_assign(key, std::move(asset));
		}

		template<typename T>
		T& get(const std::string& key) {
			return std::get<T>(m_assets.at(key));
		}

		bool contains(const std::string& key) const {
			return m_assets.find(key) != m_assets.end();
		};

	private:

		std::unordered_map<std::string, AssetVariant> m_assets;
	};

}