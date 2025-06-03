#include "AudioSystem.h"

#include "asset_utils/AssetLoader.h"
#include "scene/SceneManager.h"

#include <iostream>
#include <random>
#include <sstream>

namespace audio {

	AudioSystem::AudioSystem() {
		soloud.init();
	}

	AudioSystem::~AudioSystem() {
		soloud.deinit();
		soundMap.clear();
		handleMap.clear();
	}

	AudioSystem& AudioSystem::getInstance() {
		static AudioSystem instance;
		return instance;
	}

	bool AudioSystem::loadSound(std::string soundName, std::string path) {
		std::string resolvedPath = vk::AssetLoader::getInstance().resolvePath(path);

		if (soundMap.find(soundName) != soundMap.end()) {
			return false;
		}

		auto wav = std::make_unique<SoLoud::Wav>();
		
		SoLoud::result result = wav->load(resolvedPath.c_str());
		if (result != SoLoud::SO_NO_ERROR) {
			std::cerr << "Failed to load sound file: " << resolvedPath << " (Error: " << result << ")\n";
			return false;
		}

		soundMap[soundName] = std::move(wav);
		return true;
	}

	std::string AudioSystem::generateRandomHandle() {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dis(0, 999999);
		
		std::string handle;
		bool collision = true;
		
		do {
			std::stringstream ss;
			ss << "sound_" << dis(gen);
			handle = ss.str();
			
			collision = (handleMap.find(handle) != handleMap.end());
		} while (collision);
		
		return handle;
	}

	bool AudioSystem::playSound(std::string soundName, std::string handleName) {
		auto it = soundMap.find(soundName);
		if (it == soundMap.end()) {
			return false;
		}
		
		if (handleName == "rand") {
			handleName = generateRandomHandle();
		} else {
			auto handleIt = handleMap.find(handleName);
			if (handleIt != handleMap.end()) {
				soloud.stop(handleIt->second);
			}
		}
		
		if (handleMap.size() > 100) {
			cleanupHandles();
		}
		
		unsigned int handle = soloud.play(*it->second);
		handleMap[handleName] = handle;
		return true;
	}

	bool AudioSystem::playSoundAt(std::string soundName, glm::vec3 position, std::string handleName) {
		glm::mat4 viewMat = SceneManager::getInstance().getPlayer()->calculateViewMat();

		auto it = soundMap.find(soundName);
		if (it == soundMap.end()) {
			return false;
		}

		if (handleName == "rand") {
			handleName = generateRandomHandle();
		} else {
			auto handleIt = handleMap.find(handleName);
			if (handleIt != handleMap.end()) {
				soloud.stop(handleIt->second);
			}
		}

		if (handleMap.size() > 100) {
			cleanupHandles();
		}

		position = glm::vec3{ viewMat * glm::vec4{ position, 1.0f } };
		
		unsigned int handle = soloud.play3d(*it->second,
			position.x,
			position.y,
			position.z);
			
		handleMap[handleName] = handle;
		return true;
	}

	void AudioSystem::stopAllSounds() {
		soloud.stopAll();
	}

	void AudioSystem::stopSound(std::string handleName) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.stop(it->second);
	}

	void AudioSystem::togglePauseAllSounds() {
		allPaused = !allPaused;
		
		if (allPaused) {
			soloud.setPauseAll(true);
		} else {
			soloud.setPauseAll(false);
		}
	}

	void AudioSystem::pauseSound(std::string handleName) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setPause(it->second, true);
	}

	// TODO settings

	void AudioSystem::resumeSound(std::string handleName) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setPause(it->second, false);
	}

	void AudioSystem::cleanupHandles() {
		// remove handles for sounds that are no longer playing
		auto it = handleMap.begin();
		while (it != handleMap.end()) {
			if (!soloud.isValidVoiceHandle(it->second)) {
				it = handleMap.erase(it);
			} else {
				++it;
			}
		}
	}
}