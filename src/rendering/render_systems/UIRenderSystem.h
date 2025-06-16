#pragma once

#include "BaseRenderSystem.h"

#include "../../vk/vk_frame_info.h"


namespace vk {

    struct UIPushConstantData {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
        int hasTexture;
    };

    class UIRenderSystem : public BaseRenderSystem<UIRenderSystem, UIPushConstantData> {

    public:
        static constexpr VkShaderStageFlags PushConstStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        UIRenderSystem(Device& device, Renderer& renderer);

        std::vector<std::weak_ptr<GameObject>> gatherObjects(const FrameInfo& frameInfo);
        void tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo);
        UIPushConstantData buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo& frameInfo, VkPipelineLayout layout);
    };
}