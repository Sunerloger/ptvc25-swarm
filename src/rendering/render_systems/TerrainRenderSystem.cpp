#include "TerrainRenderSystem.h"

#include "../../scene/SceneManager.h"

#include <cassert>


namespace vk {

    TerrainRenderSystem::TerrainRenderSystem(Device& device, Renderer& renderer) : BaseRenderSystem(device, renderer) {}

    std::vector<std::weak_ptr<GameObject>> TerrainRenderSystem::gatherObjects(const FrameInfo&) {
        return SceneManager::getInstance().getTerrainRenderObjects();
    }

    void TerrainRenderSystem::tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo) {
        if (frameInfo.renderPassType == RenderPassType::SHADOW_PASS) {
            Pipeline::terrainShadowPipelineConfigInfo(config);
        }
    }

    TerrainPushConstantData TerrainRenderSystem::buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo&, VkPipelineLayout) {
        TerrainPushConstantData pc;
        pc.modelMatrix = obj->computeModelMatrix();
        pc.normalMatrix = obj->computeNormalMatrix();
        return pc;
    }
}