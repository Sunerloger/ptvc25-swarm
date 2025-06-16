#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <glm/glm.hpp>

#include "../../rendering/materials/Material.h"

#include "../../vk/vk_pipeline.h"
#include "../../vk/vk_frame_info.h"
#include "../../vk/vk_device.h"
#include "../../vk/vk_renderer.h"
#include "../../vk/vk_descriptors.h"

namespace vk {

    // PushConstStages must be defined by Derived
    // Derived must implement:
    //   std::vector<std::weak_ptr<GameObject>> gatherObjects(const FrameInfo&);
    //   void tweakPipelineConfig(PipelineConfigInfo&, const FrameInfo&);
    //   PushConst buildPushConstant(std::shared_ptr<GameObject>, const FrameInfo&, VkPipelineLayout);

    // crtp
    template<typename Derived, typename PushConst>
    class BaseRenderSystem {

    protected:
        Device& device;
        Renderer& renderer;

        std::unordered_map<std::vector<VkDescriptorSetLayout>, VkPipelineLayout, DescriptorSetLayoutVectorHash> pipelineLayoutCache;
        std::unordered_map<PipelineConfigInfo, PipelineInfo> pipelineCache;

        // Check if we already have a pipeline layout
        VkPipelineLayout getOrCreatePipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts) {
            auto it = pipelineLayoutCache.find(setLayouts);
            if (it != pipelineLayoutCache.end()) {
                return it->second;
            }

            VkPushConstantRange range{};
            range.stageFlags = Derived::PushConstStages;
            range.offset = 0;
            range.size = sizeof(PushConst);

            VkPipelineLayoutCreateInfo info{ };
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.setLayoutCount = uint32_t(setLayouts.size());
            info.pSetLayouts = setLayouts.data();
            info.pushConstantRangeCount = 1;
            info.pPushConstantRanges = &range;

            VkPipelineLayout layout;
            
            if (vkCreatePipelineLayout(device.device(), &info, nullptr, &layout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create pipeline layout");
            }

            pipelineLayoutCache.emplace(std::move(setLayouts), layout);
            return layout;
        }

        PipelineInfo& getOrCreatePipeline(PipelineConfigInfo config, std::vector<VkDescriptorSetLayout> setLayouts) {
            // create or retrieve pipeline layout
            VkPipelineLayout pl = getOrCreatePipelineLayout(std::move(setLayouts));

            config.renderPass = renderer.getCurrentRenderPass();
            config.pipelineLayout = pl;

            // check if we already have a pipeline for this configuration
            auto it = pipelineCache.find(config);
            if (it != pipelineCache.end()) {
                return it->second;
            }

            // create pipeline because it doesn't exist yet
            PipelineInfo pi;
            pi.pipelineLayout = pl;
            pi.pipeline = std::make_unique<Pipeline>(device, config);

            // cache and return
            return pipelineCache.emplace(std::move(config), std::move(pi)).first->second;
        }

    public:

        BaseRenderSystem(Device& dev, Renderer& renderer) : device(dev), renderer(renderer) {}

        virtual ~BaseRenderSystem() {
            for (auto& [key, layout] : pipelineLayoutCache) {
                vkDestroyPipelineLayout(device.device(), layout, nullptr);
            }
        }

        void renderGameObjects(FrameInfo& frameInfo) {
            auto objects = static_cast<Derived*>(this)->gatherObjects(frameInfo);
            for (auto& weakObj : objects) {
                if (auto obj = weakObj.lock()) {

                    if (!obj || !obj->getModel()) {
                        continue;
                    }

                    auto material = obj->getModel()->getMaterial();
                    if (!material) continue;

                    material->updateDescriptorSet(renderer.getFrameIndex());

                    std::vector<DescriptorSet> allSets = frameInfo.systemDescriptorSets;
                    allSets.push_back(material->getDescriptorSet(renderer.getFrameIndex()));
                    std::sort(allSets.begin(), allSets.end(), [](auto& a, auto& b) { return a.binding < b.binding; });

                    std::vector<VkDescriptorSetLayout> layouts;
                    std::vector<VkDescriptorSet>       handles;

                    layouts.reserve(allSets.size());
                    handles.reserve(allSets.size());
                    
                    for (auto& ds : allSets) {
                        layouts.push_back(ds.layout);
                        handles.push_back(ds.handle);
                    }

                    PipelineConfigInfo cfg = material->getPipelineConfig();
                    static_cast<Derived*>(this)->tweakPipelineConfig(cfg, frameInfo);

                    PipelineInfo& pi = getOrCreatePipeline(cfg, std::move(layouts));
                    pi.pipeline->bind(frameInfo.commandBuffer);

                    PushConst pc = static_cast<Derived*>(this)->buildPushConstant(obj, frameInfo, pi.pipelineLayout);
                    
                    vkCmdPushConstants(
                        frameInfo.commandBuffer,
                        pi.pipelineLayout,
                        Derived::PushConstStages,
                        0,
                        sizeof(PushConst),
                        &pc
                    );

                    vkCmdBindDescriptorSets(
                        frameInfo.commandBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pi.pipelineLayout,
                        0,
                        uint32_t(handles.size()),
                        handles.data(),
                        0, nullptr
                    );

                    obj->getModel()->bind(frameInfo.commandBuffer);
                    obj->getModel()->draw(frameInfo.commandBuffer);
                }
            }
        }
    };

}