#include "core.h"

#define SV_VULKAN_IMPLEMENTATION
#include "graphics_vulkan.h"

namespace sv {

	size_t graphics_vulkan_pipeline_compute_hash(const GraphicsState& state)
	{
		Shader_vk* vs = reinterpret_cast<Shader_vk*>(state.vertexShader);
		Shader_vk* ps = reinterpret_cast<Shader_vk*>(state.pixelShader);
		Shader_vk* gs = reinterpret_cast<Shader_vk*>(state.geometryShader);
		InputLayoutState_vk& inputLayoutState = *reinterpret_cast<InputLayoutState_vk*>(state.inputLayoutState);
		BlendState_vk& blendState = *reinterpret_cast<BlendState_vk*>(state.blendState);
		DepthStencilState_vk& depthStencilState = *reinterpret_cast<DepthStencilState_vk*>(state.depthStencilState);
		RasterizerState_vk& rasterizerState = *reinterpret_cast<RasterizerState_vk*>(state.rasterizerState);

		// TODO: This is slow...
		size_t hash = 0u;
		if (vs) hash_combine(hash, vs->ID);
		if (ps) hash_combine(hash, ps->ID);
		if (gs) hash_combine(hash, gs->ID);
		hash_combine(hash, inputLayoutState.hash);
		hash_combine(hash, blendState.hash);
		hash_combine(hash, depthStencilState.hash);
		hash_combine(hash, rasterizerState.hash);
		hash_combine(hash, u64(state.topology));

		return hash;
	}

	bool graphics_vulkan_pipeline_create(VulkanPipeline& p, Shader_vk* pVertexShader, Shader_vk* pPixelShader, Shader_vk* pGeometryShader)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		// Create
		std::lock_guard<std::mutex> lock(p.creationMutex);

		// Check if it is created
		if (p.layout != VK_NULL_HANDLE) return true;

		// Get Semantic names
		if (pVertexShader) {
			p.semanticNames.insert(pVertexShader->semanticNames.begin(), pVertexShader->semanticNames.end());
		}
		if (pPixelShader) {
			p.semanticNames.insert(pPixelShader->semanticNames.begin(), pPixelShader->semanticNames.end());
		}
		if (pGeometryShader) {
			p.semanticNames.insert(pGeometryShader->semanticNames.begin(), pGeometryShader->semanticNames.end());
		}

		// Create Pipeline Layout
		{
			std::vector<VkDescriptorSetLayout> layouts;

			if (pVertexShader) {
				p.setLayout.layouts[ShaderType_Vertex] = pVertexShader->layout;
				layouts.push_back(pVertexShader->layout.setLayout);
			}

			if (pPixelShader) {
				p.setLayout.layouts[ShaderType_Pixel] = pPixelShader->layout;
				layouts.push_back(pPixelShader->layout.setLayout);
			}

			if (pGeometryShader) {
				p.setLayout.layouts[ShaderType_Geometry] = pGeometryShader->layout;
				layouts.push_back(pGeometryShader->layout.setLayout);
			}

			VkPipelineLayoutCreateInfo layout_info{};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = u32(layouts.size());
			layout_info.pSetLayouts = layouts.data();
			layout_info.pushConstantRangeCount = 0u;

			if (vkCreatePipelineLayout(gfx.device, &layout_info, nullptr, &p.layout) != VK_SUCCESS) return false;
		}

		p.lastUsage = timer_now();

		return true;
	}

	bool graphics_vulkan_pipeline_destroy(VulkanPipeline& pipeline)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		vkDestroyPipelineLayout(gfx.device, pipeline.layout, nullptr);

		for (auto& it : pipeline.pipelines) {
			vkDestroyPipeline(gfx.device, it.second, nullptr);
		}
		return true;
	}

	VkPipeline graphics_vulkan_pipeline_get(VulkanPipeline& pipeline, GraphicsState& state, size_t hash)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();
		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		VkPipeline res = VK_NULL_HANDLE;

		hash_combine(hash, renderPass.renderPass);

		std::lock_guard<std::mutex> lock(pipeline.mutex);

		auto it = pipeline.pipelines.find(hash);
		if (it == pipeline.pipelines.end()) {

			// Shader Stages
			VkPipelineShaderStageCreateInfo shaderStages[ShaderType_GraphicsCount] = {};
			u32 shaderStagesCount = 0u;

			Shader_vk* vertexShader = reinterpret_cast<Shader_vk*>(state.vertexShader);
			Shader_vk* pixelShader = reinterpret_cast<Shader_vk*>(state.pixelShader);
			Shader_vk* geometryShader = reinterpret_cast<Shader_vk*>(state.geometryShader);

			if (vertexShader) {
				VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.flags = 0u;
				stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
				stage.module = vertexShader->module;
				stage.pName = "main";
				stage.pSpecializationInfo = nullptr;
			}
			if (pixelShader) {
				VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.flags = 0u;
				stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				stage.module = pixelShader->module;
				stage.pName = "main";
				stage.pSpecializationInfo = nullptr;
			}
			if (geometryShader) {
				VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
				stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.flags = 0u;
				stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				stage.module = geometryShader->module;
				stage.pName = "main";
				stage.pSpecializationInfo = nullptr;
			}

			// Input Layout
			VkPipelineVertexInputStateCreateInfo vertexInput{};
			vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription bindings[GraphicsLimit_InputSlot];
			VkVertexInputAttributeDescription attributes[GraphicsLimit_InputElement];
			{
				const InputLayoutStateInfo& il = state.inputLayoutState->info;
				for (u32 i = 0; i < il.slots.size(); ++i) {
					bindings[i].binding = il.slots[i].slot;
					bindings[i].inputRate = il.slots[i].instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
					bindings[i].stride = il.slots[i].stride;
				}
				for (u32 i = 0; i < il.elements.size(); ++i) {
					attributes[i].binding = il.elements[i].inputSlot;
					attributes[i].format = graphics_vulkan_parse_format(il.elements[i].format);
					attributes[i].location = pipeline.semanticNames[il.elements[i].name] + il.elements[i].index;
					attributes[i].offset = il.elements[i].offset;
				}
				vertexInput.vertexBindingDescriptionCount = u32(il.slots.size());
				vertexInput.pVertexBindingDescriptions = bindings;
				vertexInput.pVertexAttributeDescriptions = attributes;
				vertexInput.vertexAttributeDescriptionCount = u32(il.elements.size());
			}

			// Rasterizer State
			VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{};
			{
				const RasterizerStateInfo& rDesc = state.rasterizerState->info;

				rasterizerStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizerStateInfo.flags = 0u;
				rasterizerStateInfo.depthClampEnable = VK_FALSE;
				rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
				rasterizerStateInfo.polygonMode = rDesc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
				rasterizerStateInfo.cullMode = graphics_vulkan_parse_cullmode(rDesc.cullMode);
				// Is inverted beacuse the scene is rendered inverse and is flipped at presentation!!!
				rasterizerStateInfo.frontFace = rDesc.clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
				rasterizerStateInfo.depthBiasEnable = VK_FALSE;
				rasterizerStateInfo.depthBiasConstantFactor = 0;
				rasterizerStateInfo.depthBiasClamp = 0;
				rasterizerStateInfo.depthBiasSlopeFactor = 0;
				rasterizerStateInfo.lineWidth = 1.f;
			}

			// Blend State
			VkPipelineColorBlendStateCreateInfo blendStateInfo{};
			VkPipelineColorBlendAttachmentState attachments[GraphicsLimit_AttachmentRT];
			{
				const BlendStateInfo& bDesc = state.blendState->info;

				for (u32 i = 0; i < bDesc.attachments.size(); ++i)
				{
					const BlendAttachmentDesc& b = bDesc.attachments[i];

					attachments[i].blendEnable = b.blendEnabled ? VK_TRUE : VK_FALSE;
					attachments[i].srcColorBlendFactor = graphics_vulkan_parse_blendfactor(b.srcColorBlendFactor);
					attachments[i].dstColorBlendFactor = graphics_vulkan_parse_blendfactor(b.dstColorBlendFactor);;
					attachments[i].colorBlendOp = graphics_vulkan_parse_blendop(b.colorBlendOp);
					attachments[i].srcAlphaBlendFactor = graphics_vulkan_parse_blendfactor(b.srcAlphaBlendFactor);;
					attachments[i].dstAlphaBlendFactor = graphics_vulkan_parse_blendfactor(b.dstAlphaBlendFactor);;
					attachments[i].alphaBlendOp = graphics_vulkan_parse_blendop(b.alphaBlendOp);;
					attachments[i].colorWriteMask = graphics_vulkan_parse_colorcomponent(b.colorWriteMask);
				}

				blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				blendStateInfo.flags = 0u;
				blendStateInfo.logicOpEnable = VK_FALSE;
				blendStateInfo.logicOp;
				blendStateInfo.attachmentCount = u32(bDesc.attachments.size());
				blendStateInfo.pAttachments = attachments;
				blendStateInfo.blendConstants[0] = bDesc.blendConstants.x;
				blendStateInfo.blendConstants[1] = bDesc.blendConstants.y;
				blendStateInfo.blendConstants[2] = bDesc.blendConstants.z;
				blendStateInfo.blendConstants[3] = bDesc.blendConstants.w;
			}

			// DepthStencilState
			VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
			{
				const DepthStencilStateInfo& dDesc = state.depthStencilState->info;
				depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depthStencilStateInfo.flags = 0u;
				depthStencilStateInfo.depthTestEnable = dDesc.depthTestEnabled;
				depthStencilStateInfo.depthWriteEnable = dDesc.depthWriteEnabled;
				depthStencilStateInfo.depthCompareOp = graphics_vulkan_parse_compareop(dDesc.depthCompareOp);

				depthStencilStateInfo.stencilTestEnable = dDesc.stencilTestEnabled;
				depthStencilStateInfo.front.failOp = graphics_vulkan_parse_stencilop(dDesc.front.failOp);
				depthStencilStateInfo.front.passOp = graphics_vulkan_parse_stencilop(dDesc.front.passOp);
				depthStencilStateInfo.front.depthFailOp = graphics_vulkan_parse_stencilop(dDesc.front.depthFailOp);
				depthStencilStateInfo.front.compareOp = graphics_vulkan_parse_compareop(dDesc.front.compareOp);
				depthStencilStateInfo.front.compareMask = dDesc.readMask;
				depthStencilStateInfo.front.writeMask = dDesc.writeMask;
				depthStencilStateInfo.front.reference = 0u;
				depthStencilStateInfo.back.failOp = graphics_vulkan_parse_stencilop(dDesc.back.failOp);
				depthStencilStateInfo.back.passOp = graphics_vulkan_parse_stencilop(dDesc.back.passOp);
				depthStencilStateInfo.back.depthFailOp = graphics_vulkan_parse_stencilop(dDesc.back.depthFailOp);
				depthStencilStateInfo.back.compareOp = graphics_vulkan_parse_compareop(dDesc.back.compareOp);
				depthStencilStateInfo.back.compareMask = dDesc.readMask;
				depthStencilStateInfo.back.writeMask = dDesc.writeMask;
				depthStencilStateInfo.back.reference = 0u;

				depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
				depthStencilStateInfo.minDepthBounds = 0.f;
				depthStencilStateInfo.maxDepthBounds = 1.f;
			}

			// Topology
			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = graphics_vulkan_parse_topology(state.topology);

			// ViewportState
			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.flags = 0u;
			viewportState.viewportCount = GraphicsLimit_Viewport;
			viewportState.pViewports = nullptr;
			viewportState.scissorCount = GraphicsLimit_Scissor;
			viewportState.pScissors = nullptr;

			// MultisampleState
			VkPipelineMultisampleStateCreateInfo multisampleState{};
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.flags = 0u;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleState.sampleShadingEnable = VK_FALSE;
			multisampleState.minSampleShading = 1.f;
			multisampleState.pSampleMask = nullptr;
			multisampleState.alphaToCoverageEnable = VK_FALSE;
			multisampleState.alphaToOneEnable = VK_FALSE;

			// Dynamic States
			VkDynamicState dynamicStates[] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
				VK_DYNAMIC_STATE_STENCIL_REFERENCE,
				VK_DYNAMIC_STATE_LINE_WIDTH
			};

			VkPipelineDynamicStateCreateInfo dynamicStatesInfo{};
			dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStatesInfo.dynamicStateCount = 4u;
			dynamicStatesInfo.pDynamicStates = dynamicStates;

			// Creation
			VkGraphicsPipelineCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			create_info.flags = 0u;
			create_info.stageCount = shaderStagesCount;
			create_info.pStages = shaderStages;
			create_info.pVertexInputState = &vertexInput;
			create_info.pInputAssemblyState = &inputAssembly;
			create_info.pTessellationState = nullptr;
			create_info.pViewportState = &viewportState;
			create_info.pRasterizationState = &rasterizerStateInfo;
			create_info.pMultisampleState = &multisampleState;
			create_info.pDepthStencilState = &depthStencilStateInfo;
			create_info.pColorBlendState = &blendStateInfo;
			create_info.pDynamicState = &dynamicStatesInfo;
			create_info.layout = pipeline.layout;
			create_info.renderPass = renderPass.renderPass;
			create_info.subpass = 0u;

			vkAssert(vkCreateGraphicsPipelines(gfx.device, VK_NULL_HANDLE, 1u, &create_info, nullptr, &res));
			pipeline.pipelines[hash] = res;
		}
		else {
			res = it->second;
		}

		pipeline.lastUsage = timer_now();

		return res;
	}

}
