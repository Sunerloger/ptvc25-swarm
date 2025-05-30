#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>

#include "logical_systems/input/IInputController.h"
#include "logical_systems/input/InputManager.h"
#include "vk/vk_window.h"

namespace input {

	class SwarmInputController : public IInputController {
	   public:
		enum ContextID {
			Global = 0,	 // also reserved for global in input manager
			Gameplay = 1,
			MainMenu = 2,
			Death = 3,
			Debug = 4
		};

		SwarmInputController(vk::Window& w, InputManager& im);

		void setup(bool enableDebugMode = false) override;

		void setDebugModeEnabled(bool enabled) {
			debugMode = enabled;
		}
		void deregister() override;

		bool isPaused() const override;

		void setContext(ContextID ctx);
		int getContext() const;

		// define input actions
		std::function<void(const glm::vec3& dir)> onMove;
		std::function<void(float dx, float dy)> onLook;
		std::function<void()> onJump;
		std::function<void()> onShoot;

		std::function<void()> onToggleDebug;

		std::function<void(float deltaTime, const glm::vec3& dir)> onMoveUI;
		std::function<void(float deltaTime, const glm::vec3& rotDir)> onRotateUI;
		std::function<void(float deltaTime, int scaleDir)> onScaleUI;

		std::function<void(float dt, const glm::vec3& dir)> onMoveDebug;
		std::function<void(float dx, float dy)> onLookDebug;
		std::function<void(float scrollOffset)> onChangeSpeedDebug;
		std::function<void()> onToggleHudDebug;
		std::function<void()> onToggleWireframeMode;

	   private:
		vk::Window& window;

		input::InputManager& inputManager;

		double lastX, lastY;

		ContextID lastActiveContext = ContextID::Gameplay;

		// fullscreen window restoration variables
		int prevX, prevY, prevW, prevH, prevRefresh;

		bool debugMode = false;
	};

}