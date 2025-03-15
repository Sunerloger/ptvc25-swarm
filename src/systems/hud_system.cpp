#include "hud_system.h"
#include "../vk_renderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace vk {

    struct PushConstantData {
        alignas(16) float scale{1.0f};
        alignas(16) glm::vec3 translation{0.0f};
    };

    HudSystem::HudSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    HudSystem::~HudSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }


    void HudSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        VkDescriptorSetLayout layout = globalSetLayout;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if(vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
           VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }

    void HudSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.attributeDescriptions = std::vector<VkVertexInputAttributeDescription>(2);
        pipelineConfig.attributeDescriptions[0] = Model::Vertex::getAttributeDescriptions()[0];
        pipelineConfig.attributeDescriptions[1] = Model::Vertex::getAttributeDescriptions()[1];
        pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
        pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Use the source alpha value...
        pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // ...combined with the inverse of the source alpha value
        pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Add the two values together...
        pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        pipelineConfig.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Apply blending to all color channels

        pipeline = std::make_unique<Pipeline>(
            device,
            "hud.vert",
            "hud.frag",
            pipelineConfig
        );
    }

    // here are the push constants (for rotation or translation of the object)
    void HudSystem::renderGameObjects(FrameInfo& frameInfo, bool escapeMenuOpen) {
        pipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0,
                                1,
                                &frameInfo.globalDescriptorSet,
                                0,
                                nullptr);

        for (std::weak_ptr<UIComponent> weak_uiElement : frameInfo.sceneManager.getUIObjects()) {

            std::shared_ptr<UIComponent> uiElement = weak_uiElement.lock();
            if (!uiElement) {
                continue;
            }

            // TODO unite hud systems
            if (uiElement->isDrawLines) {
                continue;
            }

            // escape menu only gets drawn if open
            if(!escapeMenuOpen && uiElement->isEscapeMenu) {
                continue;
            }

            // other ui elements only get drawn if escape menu not open
            if (escapeMenuOpen && !uiElement->isEscapeMenu) {
                continue;
            }

            PushConstantData push{};
            push.scale = uiElement->getScale().x;
            push.translation = uiElement->getPosition();

            vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(PushConstantData),
                    &push
            );

            uiElement->getModel()->bind(frameInfo.commandBuffer);
            uiElement->getModel()->draw(frameInfo.commandBuffer);
        }
    }
}