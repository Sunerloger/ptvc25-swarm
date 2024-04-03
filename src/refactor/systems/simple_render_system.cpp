//
// Created by Vlad Dancea on 29.03.24.
//

#include "simple_render_system.h"
#include "../vk_renderer.h"
#include "../vk_camera.h"

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

    struct SimplePushConstantData {
        glm::mat4 modelMatrix{1.0f};
        glm::mat4 normalMatrix{1.0f};
    };

    SimpleRenderSystem::SimpleRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }


    void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

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

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Pipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        // output the current directoy;
        std::cout << "Current directory is: " << std::filesystem::current_path() << std::endl;
        pipeline = std::make_unique<Pipeline>(device, std::string(PROJECT_SOURCE_DIR) + "/assets/shaders_vk/refactor/simple_shader.vert.spv",
                                              std::string(PROJECT_SOURCE_DIR) + "/assets/shaders_vk/refactor/simple_shader.frag.spv",
                                              pipelineConfig);
    }


    // here are the push constants (for rotation or translation of the object)
    void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0,
                                1,
                                &frameInfo.globalDescriptorSet,
                                0,
                                nullptr);

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;
           if(obj.isEntity == nullptr) {
                continue;
           }

            //obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
            //obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());


            SimplePushConstantData push{};
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants(
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(SimplePushConstantData),
                    &push
            );
            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        }
    }

    void SimpleRenderSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo, Camera& camera) {
        //move objects towards camera position
        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;
            if(obj.isEnemy == nullptr) {
                continue;
            }
            auto cameraPosition = camera.getPosition();
            auto direction = glm::normalize(cameraPosition - obj.transform.translation);
            obj.transform.translation += direction * 0.01f;
        }
    }
}