#pragma once

#include "BaseRenderSystem.h"
#include "../../vk/vk_frame_info.h"
#include "../materials/TessellationMaterial.h"

namespace vk {

    struct TerrainPushConstantData {
        glm::mat4 modelMatrix{ 1.0f };
        glm::mat4 normalMatrix{ 1.0f };
    };

    class TerrainRenderSystem : public BaseRenderSystem<TerrainRenderSystem, TerrainPushConstantData> {

    public:
        static constexpr VkShaderStageFlags PushConstStages =
            VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT |
            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
            VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        TerrainRenderSystem(Device& device, Renderer& renderer);

        std::vector<std::weak_ptr<GameObject>> gatherObjects(const FrameInfo& frameInfo);
        void tweakPipelineConfig(PipelineConfigInfo& config, const FrameInfo& frameInfo);
        TerrainPushConstantData buildPushConstant(std::shared_ptr<GameObject> obj, const FrameInfo& frameInfo, VkPipelineLayout layout);
    };
}