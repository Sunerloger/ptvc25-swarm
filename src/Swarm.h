#pragma once

#include "IGame.h"

#include "vk/vk_model.h"

#include "keyboard_movement_controller.h"
#include "keyboard_placement_controller.h"
#include "keyboard_menu_controller.h"

#include "simulation/objects/actors/Player.h"
#include "simulation/objects/static/Terrain.h"
#include "simulation/objects/actors/enemies/Sprinter.h"

#include "ui/Font.h"
#include "ui/TextComponent.h"
#include "ui/UIComponent.h"

#include "rendering/materials/CubemapMaterial.h"
#include "rendering/materials/TessellationMaterial.h"
#include "rendering/structures/Skybox.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtx/string_cast.hpp>

using namespace vk;

class Swarm : public IGame {

public:

	Swarm() = default;
	~Swarm() = default;

	Swarm(const Swarm&) = delete;
	Swarm& operator=(const Swarm&) = delete;

	void init() override;
	void prePhysicsUpdate() override;
	void postPhysicsUpdate() override;
	void menuUpdate() override;

	const string getName() const override { return name; }

private:

	id_t gameTimeTextID;
	int oldSecond = 0;

	const string name = "Swarm";

};