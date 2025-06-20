#include "WaterRenderSystem.h"

#include "../../scene/SceneManager.h"


namespace vk {

    WaterRenderSystem::WaterRenderSystem(Device& device, Renderer& renderer, RenderSystemSettings& settings) : BaseRenderSystem(device, renderer, settings) {}

    std::vector<std::weak_ptr<GameObject>> WaterRenderSystem::gatherObjects(const FrameInfo&) {
        return SceneManager::getInstance().getWaterObjects();
    }

    void WaterRenderSystem::tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo) {
        // use standard pipeline from material (no tweaking)
    }

    WaterPushConstantData WaterRenderSystem::buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo& frameInfo, VkPipelineLayout) {
        WaterPushConstantData pc;
        pc.modelMatrix = obj->computeModelMatrix();
        pc.normalMatrix = obj->computeNormalMatrix();
        pc.gridInfo.x = obj->getModel()->patchCount;
        pc.timeData.x = SceneManager::getInstance().gameTime;
        return pc;
    }
}