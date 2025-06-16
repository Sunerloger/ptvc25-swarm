#include "UIRenderSystem.h"

#include "../../scene/SceneManager.h"


namespace vk {

    UIRenderSystem::UIRenderSystem(Device& device, Renderer& renderer) : BaseRenderSystem(device, renderer) {}

    std::vector<std::weak_ptr<GameObject>> UIRenderSystem::gatherObjects(const FrameInfo&) {
        // grab all the ui weak_ptrs
        auto uiWeak = SceneManager::getInstance().getUIObjects();

        // filter out invalid or non-renderable
        std::vector<std::shared_ptr<GameObject>> valid;
        valid.reserve(uiWeak.size());
        for (auto& wp : uiWeak) {
            if (auto sp = wp.lock()) {
                if (sp->getModel() && sp->getModel()->getMaterial()) {
                    valid.push_back(sp);
                }
            }
        }

        // sort back-to-front by world-space z
        std::sort(valid.begin(), valid.end(),
            [](const std::shared_ptr<GameObject>& a, const std::shared_ptr<GameObject>& b) {
                return a->getPosition().z < b->getPosition().z;
            });

        // convert back to weak_ptr list
        std::vector<std::weak_ptr<GameObject>> sorted;
        sorted.reserve(valid.size());
        for (auto& sp : valid) {
            sorted.emplace_back(sp);
        }
        return sorted;
    }

    void UIRenderSystem::tweakPipelineConfig(PipelineConfigInfo&, const FrameInfo&) {
        // use standard pipeline from material (no tweaking)
    }

    UIPushConstantData UIRenderSystem::buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo&, VkPipelineLayout) {
        UIPushConstantData pc;
        pc.modelMatrix = obj->computeModelMatrix();
        pc.normalMatrix = obj->computeNormalMatrix();
        // TODO put in texture ubo and not dependent on descriptor set
        pc.hasTexture = (obj->getModel()->getMaterial()->getDescriptorSet(renderer.getFrameIndex()).handle != VK_NULL_HANDLE);
        return pc;
    }
}