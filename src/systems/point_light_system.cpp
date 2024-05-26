//
// Created by Vlad Dancea on 29.03.24.
//

#include "point_light_system.h"
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


    struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    PointLightSystem::PointLightSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }


    void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstants);

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

    void PointLightSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        // output the current directoy;
        std::cout << "Current directory is: " << std::filesystem::current_path() << std::endl;
        pipeline = std::make_unique<Pipeline>(device, std::string(PROJECT_SOURCE_DIR) + "/assets/shaders_vk/point_light.vert.spv",
                                              std::string(PROJECT_SOURCE_DIR) + "/assets/shaders_vk/point_light.frag.spv",
                                              pipelineConfig);
    }

    void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {

        auto rotateLight = glm::rotate(
                glm::mat4(1.0f),
                frameInfo.frameTime,
                {0.0f, -1.f, 0.f});
        int lightIndex = 0;
        for (shared_ptr<lighting::PointLight> light: frameInfo.sceneManager.getLights()) {

            assert(lightIndex < MAX_LIGHTS && "Too many lights in the scene");

            // update light positions
            light->setPosition(glm::vec3(rotateLight * glm::vec4(light->getPosition(), 1.0f)));

            ubo.pointLights[lightIndex].position = glm::vec4(light->getPosition(), 1.0f);
            ubo.pointLights[lightIndex].color = glm::vec4(light->color, light->getIntensity());
            lightIndex += 1;
        }
        ubo.numLights = lightIndex;
    }

    // here are the push constants (for rotation or translation of the object)
    void PointLightSystem::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0,
                                1,
                                &frameInfo.globalDescriptorSet,
                                0,
                                nullptr);

        for (shared_ptr<lighting::PointLight> light: frameInfo.sceneManager.getLights()) {

            PointLightPushConstants push{};
            push.position = glm::vec4(light->getPosition(), 1.0f);
            push.color = glm::vec4(light->color, light->getIntensity());
            push.radius = light->getRadius();

            vkCmdPushConstants(frameInfo.commandBuffer,
                               pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PointLightPushConstants),
                               &push);
            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }
    }
}