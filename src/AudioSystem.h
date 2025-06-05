#pragma once

#include <soloud.h>
#include <soloud_wav.h>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace audio {

	enum class AttenuationModel {
		NO_ATTENUATION = 0,
		INVERSE_DISTANCE = 1,    	// 1/d
		LINEAR_DISTANCE = 2,     	// 1 - d/max
		EXPONENTIAL_DISTANCE = 3 	// 1/d²
	};

	struct SoundSettings {
		float volume = 1.0f;  // [0,1]
		float pitch = 1.0f;	  // speed/pitch multiplier
		bool looping = false;
		bool isInitiallyPaused = false;
		
		// Attenuation settings
		AttenuationModel attenuationModel = AttenuationModel::LINEAR_DISTANCE;
		float minDistance = 1.0f;    	// distance where attenuation begins
		float maxDistance = 1000.0f; 	// distance where attenuation ends
		float rolloffFactor = 1.0f;  	// how quickly the sound attenuates
	};
	
	class AudioSystem {

	public:

		static AudioSystem& getInstance();

		void init();

		bool loadSound(std::string soundName, std::string path);

		bool playSound(std::string soundName, const SoundSettings& settings = {}, std::string handleName = "rand");

		bool playSoundAt(std::string soundName, glm::vec3 position, const SoundSettings& settings = {}, std::string handleName = "rand");

		void stopAllSounds();
		void stopSound(std::string handleName);

		void togglePauseAllSounds();
		void pauseSound(std::string handleName);
		void resumeSound(std::string handleName);
		
		void cleanupHandles();

		void setVolume(std::string handleName, float volume);
		void setPitch(std::string handleName, float pitch);
		void setLooping(std::string handleName, bool looping);
		void setProtected(std::string handleName, bool isProtected);
		
		void set3dSourceParameters(std::string handleName, glm::vec3 position, glm::vec3 velocity = {0,0,0});
		void set3dSourceAttenuation(std::string handleName, AttenuationModel model, float rolloffFactor);
		void set3dSourceMinMaxDistance(std::string handleName, float minDistance, float maxDistance);
		
		void update3dAudio();

	private:

		AudioSystem();
		~AudioSystem();
		AudioSystem(const AudioSystem&) = delete;
		AudioSystem& operator=(const AudioSystem&) = delete;

		SoLoud::Soloud soloud;

		std::unordered_map<std::string, std::unique_ptr<SoLoud::Wav>> soundMap;
		std::unordered_map<std::string, unsigned int> handleMap;
		std::unordered_set<std::string> protectedHandles;

		std::string generateRandomHandle();

		bool allPaused = false;
	};
}