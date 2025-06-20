#pragma once

#include "BaseRenderSystem.h"

#include "../../vk/vk_frame_info.h"


namespace vk {

    struct WaterPushConstantData {
        glm::vec4 timeData;
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
        glm::vec4 gridInfo;
    };

    class WaterRenderSystem : public BaseRenderSystem<WaterRenderSystem, WaterPushConstantData> {

    public:
        static constexpr VkShaderStageFlags PushConstStages =
            VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT |
            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
            VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        WaterRenderSystem(Device& device, Renderer& renderer, RenderSystemSettings& settings);

        std::vector<std::weak_ptr<GameObject>> gatherObjects(const FrameInfo& frameInfo);
        void tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo);
        WaterPushConstantData buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo& frameInfo, VkPipelineLayout layout);
    };
}