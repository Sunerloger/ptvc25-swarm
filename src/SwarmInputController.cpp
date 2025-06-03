#include "SwarmInputController.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "AudioSystem.h"

namespace input {

	SwarmInputController::SwarmInputController(vk::Window& w, InputManager& im)
		: window(w), inputManager(im), lastX(w.getWidth() * 0.5), lastY(w.getHeight() * 0.5) {
		glfwSetCursorPos(window.getGLFWWindow(), lastX, lastY);
	}

	void SwarmInputController::setup(bool enableDebugMode) {
		debugMode = enableDebugMode;

		setContext(ContextID::Gameplay);

		inputManager.registerKeyCallback(
			GLFW_KEY_ESCAPE,
			[this]() {
				lastActiveContext = ContextID::Gameplay;
				setContext(ContextID::MainMenu);
				audio::AudioSystem::getInstance().togglePauseAllSounds();
			},
			this,
			ContextID::Gameplay);

		inputManager.registerKeyCallback(
			GLFW_KEY_ESCAPE,
			[this]() {
				setContext(lastActiveContext);
				audio::AudioSystem::getInstance().togglePauseAllSounds();
			},
			this,
			ContextID::MainMenu);

		if (enableDebugMode) {
			inputManager.registerKeyCallback(
				GLFW_KEY_ESCAPE,
				[this]() {
					lastActiveContext = ContextID::Debug;
					setContext(ContextID::MainMenu);
					audio::AudioSystem::getInstance().togglePauseAllSounds();
				},
				this,
				ContextID::Debug);
		}

		// use guards in case input action is not defined
		inputManager.registerKeyCallback(GLFW_KEY_SPACE, [this]() { if (onJump)  onJump(); }, this, ContextID::Gameplay);
		inputManager.registerMouseButtonCallback(GLFW_MOUSE_BUTTON_LEFT, [this]() { if (onShoot) onShoot(); }, this, ContextID::Gameplay);

		inputManager.registerPollingAction(
			[this](float dt) {
				// movement
				glm::vec3 dir{0.0f};
				if (this->inputManager.isKeyPressed(GLFW_KEY_W))
					dir.z -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_S))
					dir.z += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_A))
					dir.x -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_D))
					dir.x += 1;
				if (auto len = glm::length(dir); len > 0.0f)
					dir /= len;
				if (onMove)
					onMove(dir);
			},
			this,
			ContextID::Gameplay);
		inputManager.registerPollingAction(
			[this](float dt) {
				// looking
				auto [x, y] = this->inputManager.getCursorPos();
				float dx = float(x - lastX), dy = float(y - lastY);
				lastX = x;
				lastY = y;
				if ((dx || dy) && onLook)
					onLook(dx, dy);
			},
			this,
			ContextID::Gameplay);

		inputManager.registerPollingAction(
			[this](float dt) {
				// UI movement
				glm::vec3 dir{0.0f};
				if (this->inputManager.isKeyPressed(GLFW_KEY_LEFT))
					dir.x -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_RIGHT))
					dir.x += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_UP))
					dir.y += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_DOWN))
					dir.y -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_COMMA))
					dir.z += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_PERIOD))
					dir.z -= 1;
				if (auto len = glm::length(dir); len > 0.0f)
					dir /= len;
				if (dir != glm::vec3(0.0f) && onMoveUI)
					onMoveUI(dt, dir);
			},
			this,
			ContextID::Debug);
		inputManager.registerPollingAction(
			[this](float dt) {
				// UI rotation
				glm::vec3 rotDir{0.0f};
				if (this->inputManager.isKeyPressed(GLFW_KEY_Z))
					rotDir.x -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_X))
					rotDir.x += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_C))
					rotDir.y -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_V))
					rotDir.y += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_B))
					rotDir.z -= 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_N))
					rotDir.z += 1;
				if (rotDir != glm::vec3(0.0f) && onRotateUI)
					onRotateUI(dt, rotDir);
			},
			this,
			ContextID::Debug);
		inputManager.registerPollingAction(
			[this](float dt) {
				// UI scale
				int scaleDir = 0;
				if (this->inputManager.isKeyPressed(GLFW_KEY_EQUAL))
					scaleDir += 1;
				if (this->inputManager.isKeyPressed(GLFW_KEY_MINUS))
					scaleDir -= 1;
				if (scaleDir != 0 && onScaleUI)
					onScaleUI(dt, scaleDir);
			},
			this,
			ContextID::Debug);

		if (enableDebugMode) {
			inputManager.registerPollingAction(
				[this](float dt) {
					// movement
					glm::vec3 dir{0.0f};
					if (this->inputManager.isKeyPressed(GLFW_KEY_W))
						dir.z -= 1;
					if (this->inputManager.isKeyPressed(GLFW_KEY_S))
						dir.z += 1;
					if (this->inputManager.isKeyPressed(GLFW_KEY_A))
						dir.x -= 1;
					if (this->inputManager.isKeyPressed(GLFW_KEY_D))
						dir.x += 1;
					if (this->inputManager.isKeyPressed(GLFW_KEY_SPACE))
						dir.y += 1;
					if (this->inputManager.isKeyPressed(GLFW_KEY_LEFT_SHIFT))
						dir.y -= 1;
					if (auto len = glm::length(dir); len > 0.0f)
						dir /= len;
					if (onMoveDebug)
						onMoveDebug(dt, dir);
				},
				this,
				ContextID::Debug);
			inputManager.registerPollingAction(
				[this](float dt) {
					// looking
					auto [x, y] = this->inputManager.getCursorPos();
					float dx = float(x - lastX), dy = float(y - lastY);
					lastX = x;
					lastY = y;
					if ((dx || dy) && onLookDebug)
						onLookDebug(dx, dy);
				},
				this,
				ContextID::Debug);
			inputManager.registerScrollCallback(
				[this](float xOffset, float yOffset) {
					if (yOffset)
						onChangeSpeedDebug(yOffset);
				},
				this,
				ContextID::Debug);
			inputManager.registerKeyCallback(
				GLFW_KEY_F1,
				[this]() {
					onToggleHudDebug();
				},
				this,
				ContextID::Debug);
			inputManager.registerKeyCallback(
				GLFW_KEY_F9,
				[this]() {
					onToggleWireframeMode();
				},
				this,
				ContextID::Debug);

			inputManager.registerKeyCallback(
				GLFW_KEY_F10,
				[this]() {
					setContext(ContextID::Gameplay);
					onToggleDebug();
				},
				this,
				ContextID::Debug);
			inputManager.registerKeyCallback(
				GLFW_KEY_F10,
				[this]() {
					setContext(ContextID::Debug);
					onToggleDebug();
				},
				this,
				ContextID::Gameplay);
		}

		inputManager.registerKeyCallback(
			GLFW_KEY_F11,
			[this]() {
				// toggle fullscreen
				GLFWwindow* glfwWindow = this->window.getGLFWWindow();
				const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
				if (glfwGetWindowMonitor(glfwWindow)) {
					glfwSetWindowMonitor(glfwWindow, nullptr,
						prevX, prevY, prevW, prevH, prevRefresh);
				} else {  // switch to fullscreen
					glfwGetWindowPos(glfwWindow, &prevX, &prevY);
					glfwGetWindowSize(glfwWindow, &prevW, &prevH);
					prevRefresh = vm->refreshRate;
					glfwSetWindowMonitor(glfwWindow, glfwGetPrimaryMonitor(),
						0, 0, vm->width, vm->height, vm->refreshRate);
				}
			},
			this,
			ContextID::Global);
	}

	void SwarmInputController::deregister() {
		inputManager.deregisterOwner(this);
	}

	void SwarmInputController::setContext(ContextID ctx) {
		// failsafe
		if (!debugMode && ctx == ContextID::Debug) {
			printf("Debug mode is not enabled, cannot set context to Debug\n");
			return;
		}

		inputManager.setActiveContext(ctx);

		// lock mouse for gameplay and debug, free otherwise
		if (ctx == ContextID::Gameplay || ctx == ContextID::Debug) {
			glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// reposition to center so mouse delta starts fresh
			lastX = window.getWidth() * 0.5;
			lastY = window.getHeight() * 0.5;
			glfwSetCursorPos(window.getGLFWWindow(), lastX, lastY);

			printf("Context set to %s, cursor disabled\n",
				ctx == ContextID::Gameplay ? "Gameplay" : "Debug");
		} else {
			glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			printf("Context set to %d, cursor normal\n", ctx);
		}
	}

	int SwarmInputController::getContext() const {
		return inputManager.getActiveContext();
	}

	bool SwarmInputController::isPaused() const {
		return inputManager.getActiveContext() != ContextID::Gameplay && inputManager.getActiveContext() != ContextID::Debug;
	}
}