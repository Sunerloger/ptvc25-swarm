#include "TextureRenderSystem.h"

#include "../../scene/SceneManager.h"


namespace vk {

    TextureRenderSystem::TextureRenderSystem(Device& device, Renderer& renderer, RenderSystemSettings& settings) : BaseRenderSystem(device, renderer, settings) {}

    std::vector<std::weak_ptr<GameObject>> TextureRenderSystem::gatherObjects(const FrameInfo&) {
        return SceneManager::getInstance().getStandardRenderObjects();
    }

    void TextureRenderSystem::tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo) {
        if (frameInfo.renderPassType == RenderPassType::SHADOW_PASS) {
            Pipeline::shadowPipelineConfigInfo(config);
        }
    }

    SimplePushConstantData TextureRenderSystem::buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo&, VkPipelineLayout) {
        SimplePushConstantData pc;
        pc.modelMatrix = obj->computeModelMatrix();
        pc.normalMatrix = obj->computeNormalMatrix();
        return pc;
    }
}