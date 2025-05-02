#include "vk_pipeline.h"
#include "vk_model.h"
#include "../asset_utils/AssetManager.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace vk {

	Pipeline::Pipeline(Device& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo)
		: device{device} {
		createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
	}
	
	Pipeline::~Pipeline() {
		vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
		
		// Clean up tessellation shader modules if they exist
		if (tessControlShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device.device(), tessControlShaderModule, nullptr);
		}
		if (tessEvalShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device.device(), tessEvalShaderModule, nullptr);
		}
		
		vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
	}

	// Constructor with tessellation shaders
	Pipeline::Pipeline(Device& device,
		const std::string& vertFilepath,
		const std::string& tessControlFilepath,
		const std::string& tessEvalFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo)
		: device{device} {
		createTessellationPipeline(vertFilepath, tessControlFilepath, tessEvalFilepath, fragFilepath, configInfo);
	}

	std::vector<char> Pipeline::readFile(const std::string& filepath) {
		return AssetManager::getInstance().loadShader(filepath);
	}

	void Pipeline::createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo) {
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE &&
			   "Cannot create graphics pipeline::pipelineLayout is null");
		assert(configInfo.renderPass != VK_NULL_HANDLE &&
			   "Cannot create graphics pipeline::renderPass is null");
		auto vertCode = readFile(vertFilepath);
		auto fragCode = readFile(fragFilepath);

		createShaderModule(vertCode, &vertShaderModule);
		createShaderModule(fragCode, &fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
	
	void Pipeline::createTessellationPipeline(
		const std::string& vertFilepath,
		const std::string& tessControlFilepath,
		const std::string& tessEvalFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo) {
		
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE &&
			   "Cannot create tessellation pipeline::pipelineLayout is null");
		assert(configInfo.renderPass != VK_NULL_HANDLE &&
			   "Cannot create tessellation pipeline::renderPass is null");
			   
		// Load shader code
		auto vertCode = readFile(vertFilepath);
		auto tessControlCode = readFile(tessControlFilepath);
		auto tessEvalCode = readFile(tessEvalFilepath);
		auto fragCode = readFile(fragFilepath);

		// Create shader modules
		createShaderModule(vertCode, &vertShaderModule);
		createShaderModule(tessControlCode, &tessControlShaderModule);
		createShaderModule(tessEvalCode, &tessEvalShaderModule);
		createShaderModule(fragCode, &fragShaderModule);

		// Create shader stages
		VkPipelineShaderStageCreateInfo shaderStages[4];
		
		// Vertex shader
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		// Tessellation control shader
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		shaderStages[1].module = tessControlShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		// Tessellation evaluation shader
		shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[2].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		shaderStages[2].module = tessEvalShaderModule;
		shaderStages[2].pName = "main";
		shaderStages[2].flags = 0;
		shaderStages[2].pNext = nullptr;
		shaderStages[2].pSpecializationInfo = nullptr;

		// Fragment shader
		shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[3].module = fragShaderModule;
		shaderStages[3].pName = "main";
		shaderStages[3].flags = 0;
		shaderStages[3].pNext = nullptr;
		shaderStages[3].pSpecializationInfo = nullptr;

		// Vertex input state
		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		// Create pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 4;  // Vertex, TessControl, TessEval, Fragment
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
		pipelineInfo.pTessellationState = &configInfo.tessellationInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create tessellation pipeline!");
		}
	}

	void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}
	}

	void Pipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	void Pipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
		// Set default shader paths
		configInfo.vertShaderPath = "texture_shader.vert";
		configInfo.fragShaderPath = "texture_shader.frag";
		configInfo.tessControlShaderPath = "";
		configInfo.tessEvalShaderPath = "";
		configInfo.useTessellation = false;
		
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;			  // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;	  // Optional

		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;			  // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr;			  // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;		  // Optional

		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	 // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				 // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	 // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				 // Optional

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;	 // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;	 // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;	 // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;	 // Optional

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;	// Optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;	// Optional
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};	 // Optional
		configInfo.depthStencilInfo.back = {};	 // Optional

		configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		configInfo.bindingDescriptions = Model::Vertex::getBindingDescriptions();
		configInfo.attributeDescriptions = Model::Vertex::getAttributeDescriptions();
		
		// Initialize tessellation state (even though it's not used by default)
		configInfo.tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		configInfo.tessellationInfo.pNext = nullptr;
		configInfo.tessellationInfo.flags = 0;
		configInfo.tessellationInfo.patchControlPoints = 3;  // Default for triangles
	}
	
	void Pipeline::tessellationPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t patchControlPoints) {
		// Start with default configuration
		defaultPipelineConfigInfo(configInfo);
		
		// Enable tessellation
		configInfo.useTessellation = true;
		
		// Set default tessellation shader paths
		configInfo.vertShaderPath = "terrain_shader.vert";
		configInfo.fragShaderPath = "terrain_shader.frag";
		configInfo.tessControlShaderPath = "terrain_tess_control.tesc";
		configInfo.tessEvalShaderPath = "terrain_tess_eval.tese";
		
		// Set patch control points
		configInfo.patchControlPoints = patchControlPoints;
		configInfo.tessellationInfo.patchControlPoints = patchControlPoints;
		
		// Change input assembly topology to patch list
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	}

}