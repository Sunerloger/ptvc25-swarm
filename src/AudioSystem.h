#pragma once

#include <soloud.h>
#include <soloud_wav.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace audio {
	
	class AudioSystem {

	public:

		static AudioSystem& getInstance();

		bool loadSound(std::string soundName, std::string path);

		bool playSound(std::string soundName, std::string handleName = "rand");

		bool playSoundAt(std::string soundName, glm::vec3 position, std::string handleName = "rand");

		void stopAllSounds();
		void stopSound(std::string soundName);

		void togglePauseAllSounds();
		void pauseSound(std::string handleName);
		void resumeSound(std::string handleName);
		
		void cleanupHandles();

	private:

		AudioSystem();
		~AudioSystem();
		AudioSystem(const AudioSystem&) = delete;
		AudioSystem& operator=(const AudioSystem&) = delete;

		SoLoud::Soloud soloud;

		std::unordered_map<std::string, std::unique_ptr<SoLoud::Wav>> soundMap;
		std::unordered_map<std::string, unsigned int> handleMap;

		std::string generateRandomHandle();

		bool allPaused = false;
	};
}