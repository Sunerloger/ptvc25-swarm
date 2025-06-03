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

	void AudioSystem::init() {
		soloud.set3dSoundSpeed(343.0f); // Speed of sound in m/s

		auto player = SceneManager::getInstance().getPlayer();
		if (!player) return;
		
		glm::vec3 position = player->getCameraPosition();
		soloud.set3dListenerPosition(position.x, position.y, position.z);
		glm::vec3 forward = player->getFront();
		soloud.set3dListenerAt(forward.x, forward.y, forward.z);
		glm::vec3 up = player->getUp();
		soloud.set3dListenerUp(up.x, up.y, up.z);
		
		soloud.update3dAudio();
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

	bool AudioSystem::playSound(std::string soundName, const SoundSettings& settings, std::string handleName) {
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
		
		it->second->setLooping(settings.looping);
		
		unsigned int handle = soloud.play(*it->second, settings.volume, 0.0f, settings.isInitiallyPaused);
		soloud.setRelativePlaySpeed(handle, settings.pitch);
		
		handleMap[handleName] = handle;
		return true;
	}

	bool AudioSystem::playSoundAt(std::string soundName, glm::vec3 position, const SoundSettings& settings, std::string handleName) {

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

		it->second->setLooping(settings.looping);
		
		unsigned int handle = soloud.play3d(*it->second,
			position.x,
			position.y,
			position.z,
			0.0f, 0.0f, 0.0f,  // velocity
			settings.volume,
			settings.isInitiallyPaused);
		
		unsigned int attModel = static_cast<unsigned int>(settings.attenuationModel);
		soloud.set3dSourceAttenuation(handle, attModel, settings.rolloffFactor);
		soloud.set3dSourceMinMaxDistance(handle, settings.minDistance, settings.maxDistance);
		soloud.setRelativePlaySpeed(handle, settings.pitch);
		
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

	void AudioSystem::resumeSound(std::string handleName) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setPause(it->second, false);
	}

	void AudioSystem::cleanupHandles() {
		// remove handles for sounds that are no longer playing and not paused
		auto it = handleMap.begin();
		while (it != handleMap.end()) {
			if (!soloud.isValidVoiceHandle(it->second) && !soloud.getPause(it->second)) {
				it = handleMap.erase(it);
			} else {
				++it;
			}
		}
	}

	void AudioSystem::setVolume(std::string handleName, float volume) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setVolume(it->second, volume);
	}

	void AudioSystem::setPitch(std::string handleName, float pitch) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setRelativePlaySpeed(it->second, pitch);
	}

	void AudioSystem::setLooping(std::string handleName, bool looping) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.setLooping(it->second, looping);
	}

	void AudioSystem::set3dSourceParameters(std::string handleName, glm::vec3 position, glm::vec3 velocity) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.set3dSourcePosition(it->second, position.x, position.y, position.z);
		soloud.set3dSourceVelocity(it->second, velocity.x, velocity.y, velocity.z);
	}

	void AudioSystem::set3dSourceAttenuation(std::string handleName, AttenuationModel model, float rolloffFactor) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		unsigned int attModel = static_cast<unsigned int>(model);
		soloud.set3dSourceAttenuation(it->second, attModel, rolloffFactor);
	}

	void AudioSystem::set3dSourceMinMaxDistance(std::string handleName, float minDistance, float maxDistance) {
		auto it = handleMap.find(handleName);
		if (it == handleMap.end()) return;
		
		soloud.set3dSourceMinMaxDistance(it->second, minDistance, maxDistance);
	}

	void AudioSystem::update3dAudio() {
		auto player = SceneManager::getInstance().getPlayer();
		if (!player) return;
		
		glm::vec3 position = player->getCameraPosition();
		
		glm::vec3 forward = player->getFront();
		glm::vec3 up = player->getUp();
		
		soloud.set3dListenerPosition(position.x, position.y, position.z);
		soloud.set3dListenerAt(forward.x, forward.y, forward.z);
		soloud.set3dListenerUp(up.x, up.y, up.z);
		
		soloud.update3dAudio();
	}
}