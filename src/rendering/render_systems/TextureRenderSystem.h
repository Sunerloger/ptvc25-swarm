#pragma once

#include "BaseRenderSystem.h"

#include "../../vk/vk_frame_info.h"


namespace vk {

    struct SimplePushConstantData {
        glm::mat4 modelMatrix{ 1.0f };
        glm::mat4 normalMatrix{ 1.0f };
    };

    class TextureRenderSystem : public BaseRenderSystem<TextureRenderSystem, SimplePushConstantData> {

    public:
        static constexpr VkShaderStageFlags PushConstStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        TextureRenderSystem(Device& device, Renderer& renderer, RenderSystemSettings& settings);

        std::vector<std::weak_ptr<GameObject>> gatherObjects(const FrameInfo& frameInfo);
        void tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo);
        SimplePushConstantData buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo& frameInfo, VkPipelineLayout layout);
    };
}