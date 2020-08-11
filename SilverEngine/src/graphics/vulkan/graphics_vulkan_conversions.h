#pragma once

namespace sv {

	constexpr VkFormat graphics_vulkan_parse_format(Format format)
	{
		switch (format)
		{
		case Format_Unknown:
			return VK_FORMAT_UNDEFINED;

		case Format_R32G32B32A32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case Format_R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;

		case Format_R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;

		case Format_R32G32B32_FLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case Format_R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;

		case Format_R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;

		case Format_R16G16B16A16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case Format_R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;

		case Format_R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;

		case Format_R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;

		case Format_R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;

		case Format_R32G32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;

		case Format_R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;

		case Format_R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;

		case Format_R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;

		case Format_R8G8B8A8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;

		case Format_R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;

		case Format_R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;

		case Format_R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;

		case Format_R16G16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;

		case Format_R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;

		case Format_R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;

		case Format_R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;

		case Format_R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;

		case Format_D32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;

		case Format_R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;

		case Format_R32_UINT:
			return VK_FORMAT_R32_UINT;

		case Format_R32_SINT:
			return VK_FORMAT_R32_SINT;

		case Format_D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;

		case Format_R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;

		case Format_R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;

		case Format_R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;

		case Format_R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;

		case Format_R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;

		case Format_D16_UNORM:
			return VK_FORMAT_D16_UNORM;

		case Format_R16_UNORM:
			return VK_FORMAT_R16_UNORM;

		case Format_R16_UINT:
			return VK_FORMAT_R16_UINT;

		case Format_R16_SNORM:
			return VK_FORMAT_R16_SNORM;

		case Format_R16_SINT:
			return VK_FORMAT_R16_SINT;

		case Format_R8_UNORM:
			return VK_FORMAT_R8_UNORM;

		case Format_R8_UINT:
			return VK_FORMAT_R8_UINT;

		case Format_R8_SNORM:
			return VK_FORMAT_R8_SNORM;

		case Format_R8_SINT:
			return VK_FORMAT_R8_SINT;

		case Format_BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

		case Format_BC1_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

		case Format_BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;

		case Format_BC2_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;

		case Format_BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;

		case Format_BC3_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;

		case Format_BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;

		case Format_BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;

		case Format_BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;

		case Format_BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;

		case Format_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;

		case Format_B8G8R8A8_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;

		default:
			sv::log_warning("Invalid Format");
			return VK_FORMAT_UNDEFINED;
		}
	}
	constexpr Format graphics_vulkan_parse_format(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_UNDEFINED:
			return Format_Unknown;

		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return Format_R32G32B32A32_FLOAT;

		case VK_FORMAT_R32G32B32A32_UINT:
			return Format_R32G32B32A32_UINT;

		case VK_FORMAT_R32G32B32A32_SINT:
			return Format_R32G32B32A32_SINT;

		case VK_FORMAT_R32G32B32_SFLOAT:
			return Format_R32G32B32_FLOAT;

		case VK_FORMAT_R32G32B32_UINT:
			return Format_R32G32B32_UINT;

		case VK_FORMAT_R32G32B32_SINT:
			return Format_R32G32B32_SINT;

		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return Format_R16G16B16A16_FLOAT;

		case VK_FORMAT_R16G16B16A16_UNORM:
			return Format_R16G16B16A16_UNORM;

		case VK_FORMAT_R16G16B16A16_UINT:
			return Format_R16G16B16A16_UINT;

		case VK_FORMAT_R16G16B16A16_SNORM:
			return Format_R16G16B16A16_SNORM;

		case VK_FORMAT_R16G16B16A16_SINT:
			return Format_R16G16B16A16_SINT;

		case VK_FORMAT_R32G32_SFLOAT:
			return Format_R32G32_FLOAT;

		case VK_FORMAT_R32G32_UINT:
			return Format_R32G32_UINT;

		case VK_FORMAT_R32G32_SINT:
			return Format_R32G32_SINT;

		case VK_FORMAT_R8G8B8A8_UNORM:
			return Format_R8G8B8A8_UNORM;

		case VK_FORMAT_R8G8B8A8_SRGB:
			return Format_R8G8B8A8_SRGB;

		case VK_FORMAT_R8G8B8A8_UINT:
			return Format_R8G8B8A8_UINT;

		case VK_FORMAT_R8G8B8A8_SNORM:
			return Format_R8G8B8A8_SNORM;

		case VK_FORMAT_R8G8B8A8_SINT:
			return Format_R8G8B8A8_SINT;

		case VK_FORMAT_R16G16_SFLOAT:
			return Format_R16G16_FLOAT;

		case VK_FORMAT_R16G16_UNORM:
			return Format_R16G16_UNORM;

		case VK_FORMAT_R16G16_UINT:
			return Format_R16G16_UINT;

		case VK_FORMAT_R16G16_SNORM:
			return Format_R16G16_SNORM;

		case VK_FORMAT_R16G16_SINT:
			return Format_R16G16_SINT;

		case VK_FORMAT_D32_SFLOAT:
			return Format_D32_FLOAT;

		case VK_FORMAT_R32_SFLOAT:
			return Format_R32_FLOAT;

		case VK_FORMAT_R32_UINT:
			return Format_R32_UINT;

		case VK_FORMAT_R32_SINT:
			return Format_R32_SINT;

		case VK_FORMAT_D24_UNORM_S8_UINT:
			return Format_D24_UNORM_S8_UINT;

		case VK_FORMAT_R8G8_UNORM:
			return Format_R8G8_UNORM;

		case VK_FORMAT_R8G8_UINT:
			return Format_R8G8_UINT;

		case VK_FORMAT_R8G8_SNORM:
			return Format_R8G8_SNORM;

		case VK_FORMAT_R8G8_SINT:
			return Format_R8G8_SINT;

		case VK_FORMAT_R16_SFLOAT:
			return Format_R16_FLOAT;

		case VK_FORMAT_D16_UNORM:
			return Format_D16_UNORM;

		case VK_FORMAT_R16_UNORM:
			return Format_R16_UNORM;

		case VK_FORMAT_R16_UINT:
			return Format_R16_UINT;

		case VK_FORMAT_R16_SNORM:
			return Format_R16_SNORM;

		case VK_FORMAT_R16_SINT:
			return Format_R16_SINT;

		case VK_FORMAT_R8_UNORM:
			return Format_R8_UNORM;

		case VK_FORMAT_R8_UINT:
			return Format_R8_UINT;

		case VK_FORMAT_R8_SNORM:
			return Format_R8_SNORM;

		case VK_FORMAT_R8_SINT:
			return Format_R8_SINT;

		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return Format_BC1_UNORM;

		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			return Format_BC1_SRGB;

		case VK_FORMAT_BC2_UNORM_BLOCK:
			return Format_BC2_UNORM;

		case VK_FORMAT_BC2_SRGB_BLOCK:
			return Format_BC2_SRGB;

		case VK_FORMAT_BC3_UNORM_BLOCK:
			return Format_BC3_UNORM;

		case VK_FORMAT_BC3_SRGB_BLOCK:
			return Format_BC3_SRGB;

		case VK_FORMAT_BC4_UNORM_BLOCK:
			return Format_BC4_UNORM;

		case VK_FORMAT_BC4_SNORM_BLOCK:
			return Format_BC4_SNORM;

		case VK_FORMAT_BC5_UNORM_BLOCK:
			return Format_BC5_UNORM;

		case VK_FORMAT_BC5_SNORM_BLOCK:
			return Format_BC5_SNORM;

		case VK_FORMAT_B8G8R8A8_UNORM:
			return Format_B8G8R8A8_UNORM;

		case VK_FORMAT_B8G8R8A8_SRGB:
			return Format_B8G8R8A8_SRGB;

		default:
			sv::log_warning("Invalid Format");
			return Format_Unknown;
		}
	}
	constexpr VkPrimitiveTopology graphics_vulkan_parse_topology(GraphicsTopology topology)
	{
		switch (topology)
		{
		case GraphicsTopology_Points:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case GraphicsTopology_Lines:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case GraphicsTopology_LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case GraphicsTopology_Triangles:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case GraphicsTopology_TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:
			sv::log_warning("Unknown Topology");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}
	constexpr VkCullModeFlags graphics_vulkan_parse_cullmode(RasterizerCullMode mode)
	{
		switch (mode)
		{
		case RasterizerCullMode_None:
			return VK_CULL_MODE_NONE;
		case RasterizerCullMode_Front:
			return VK_CULL_MODE_FRONT_BIT;
		case RasterizerCullMode_Back:
			return VK_CULL_MODE_BACK_BIT;
		case RasterizerCullMode_FrontAndBack:
			return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			return VK_CULL_MODE_BACK_BIT;
		}
	}
	constexpr VkAttachmentLoadOp graphics_vulkan_parse_attachment_loadop(AttachmentOperation op)
	{
		switch (op)
		{
		case AttachmentOperation_DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case AttachmentOperation_Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentOperation_Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		default:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		}
	}
	constexpr VkAttachmentStoreOp graphics_vulkan_parse_attachment_storeop(AttachmentOperation op)
	{
		switch (op)
		{
		case AttachmentOperation_DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case AttachmentOperation_Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
		default:
			return VK_ATTACHMENT_STORE_OP_STORE;
		}
	}
	constexpr VkImageLayout graphics_vulkan_parse_image_layout(GPUImageLayout l, Format format = Format_Unknown)
	{
		switch (l)
		{
		case GPUImageLayout_Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;

		case GPUImageLayout_DepthStencil:
			if (format == Format_Unknown)	return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			else {

				if (sv::graphics_format_has_stencil(format)) {
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}

		case GPUImageLayout_DepthStencilReadOnly:
			if (format == Format_Unknown) return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			else {
				if (sv::graphics_format_has_stencil(format)) {
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}

		case GPUImageLayout_ShaderResource:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case GPUImageLayout_RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		default:
			sv::log_warning("Unknown Image Layout");
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}

	constexpr VkCompareOp graphics_vulkan_parse_compareop(CompareOperation op)
	{
		switch (op)
		{
		case CompareOperation_Never:
			return VK_COMPARE_OP_NEVER;

		case CompareOperation_Less:
			return VK_COMPARE_OP_LESS;

		case CompareOperation_Equal:
			return VK_COMPARE_OP_EQUAL;

		case CompareOperation_LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;

		case CompareOperation_Greater:
			return VK_COMPARE_OP_GREATER;

		case CompareOperation_NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;

		case CompareOperation_GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;

		case CompareOperation_Always:
			return VK_COMPARE_OP_ALWAYS;

		default:
			return VK_COMPARE_OP_NEVER;
		}
	}

	constexpr VkStencilOp graphics_vulkan_parse_stencilop(StencilOperation op)
	{
		switch (op)
		{
		case StencilOperation_Keep:
			return VK_STENCIL_OP_KEEP;

		case StencilOperation_Zero:
			return VK_STENCIL_OP_ZERO;

		case StencilOperation_Replace:
			return VK_STENCIL_OP_REPLACE;

		case StencilOperation_IncrementAndClamp:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;

		case StencilOperation_DecrementAndClamp:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;

		case StencilOperation_Invert:
			return VK_STENCIL_OP_INVERT;

		case StencilOperation_IncrementAndWrap:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;

		case StencilOperation_DecrementAndWrap:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;

		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	constexpr VkBlendOp graphics_vulkan_parse_blendop(BlendOperation op)
	{
		switch (op)
		{
		case BlendOperation_Add:
			return VK_BLEND_OP_ADD;

		case BlendOperation_Substract:
			return VK_BLEND_OP_SUBTRACT;

		case BlendOperation_ReverseSubstract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;

		case BlendOperation_Min:
			return VK_BLEND_OP_MIN;

		case BlendOperation_Max:
			return VK_BLEND_OP_MAX;

		default:
			return VK_BLEND_OP_ADD;
		}
	}

	inline VkColorComponentFlags graphics_vulkan_parse_colorcomponent(sv::ColorComponentFlags f)
	{
		VkColorComponentFlags res = 0u;
		if (f & ColorComponent_R) res |= VK_COLOR_COMPONENT_R_BIT;
		if (f & ColorComponent_G) res |= VK_COLOR_COMPONENT_G_BIT;
		if (f & ColorComponent_B) res |= VK_COLOR_COMPONENT_B_BIT;
		if (f & ColorComponent_A) res |= VK_COLOR_COMPONENT_A_BIT;
		return res;
	}

	constexpr VkBlendFactor graphics_vulkan_parse_blendfactor(BlendFactor bf)
	{
		switch (bf)
		{
		case BlendFactor_Zero:
			return VK_BLEND_FACTOR_ZERO;

		case BlendFactor_One:
			return VK_BLEND_FACTOR_ONE;

		case BlendFactor_SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;

		case BlendFactor_OneMinusSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

		case BlendFactor_DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;

		case BlendFactor_OneMinusDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

		case BlendFactor_SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;

		case BlendFactor_OneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		case BlendFactor_DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;

		case BlendFactor_OneMinusDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

		case BlendFactor_ConstantColor:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;

		case BlendFactor_OneMinusConstantColor:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

		case BlendFactor_ConstantAlpha:
			return VK_BLEND_FACTOR_CONSTANT_ALPHA;

		case BlendFactor_OneMinusConstantAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;

		case BlendFactor_SrcAlphaSaturate:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

		case BlendFactor_Src1Color:
			return VK_BLEND_FACTOR_SRC1_COLOR;

		case BlendFactor_OneMinusSrc1Color:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

		case BlendFactor_Src1Alpha:
			return VK_BLEND_FACTOR_SRC1_ALPHA;

		case BlendFactor_OneMinusSrc1Alpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	constexpr VkIndexType graphics_vulkan_parse_indextype(IndexType t)
	{
		switch (t)
		{
		case IndexType_16:
			return VK_INDEX_TYPE_UINT16;
		case IndexType_32:
			return VK_INDEX_TYPE_UINT32;
		default:
			return VK_INDEX_TYPE_UINT16;
		}
	}

	constexpr VkShaderStageFlags graphics_vulkan_parse_shadertype(ShaderType t)
	{
		switch (t)
		{
		case ShaderType_Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType_Pixel:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType_Geometry:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType_Hull:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderType_Domain:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderType_Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	constexpr ShaderType graphics_vulkan_parse_shadertype(VkShaderStageFlags t)
	{
		switch (t)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return ShaderType_Vertex;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return ShaderType_Pixel;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return ShaderType_Geometry;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return ShaderType_Hull;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return ShaderType_Domain;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return ShaderType_Compute;
		default:
			return ShaderType_Vertex;
		}
	}

	constexpr VkFilter graphics_vulkan_parse_filter(SamplerFilter f)
	{
		switch (f)
		{
		case SamplerFilter_Nearest:
			return VK_FILTER_NEAREST;
		case SamplerFilter_Linear:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_NEAREST;
		}
	}

	constexpr VkSamplerAddressMode graphics_vulkan_parse_addressmode(SamplerAddressMode m)
	{
		switch (m)
		{
		case SamplerAddressMode_Wrap:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case SamplerAddressMode_Mirror:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case SamplerAddressMode_Clamp:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case SamplerAddressMode_Border:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	inline VkAccessFlags graphics_vulkan_access_from_image_layout(GPUImageLayout layout)
	{
		VkAccessFlags flags = 0u;

		switch (layout)
		{
		case GPUImageLayout_Undefined:
			break;
		case GPUImageLayout_General:
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			flags |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case GPUImageLayout_RenderTarget:
			flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case GPUImageLayout_DepthStencil:
			flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case GPUImageLayout_DepthStencilReadOnly:
			flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case GPUImageLayout_ShaderResource:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			sv::log_warning("Undefined Image Layout");
			break;
		}

		return flags;
	}

	inline VkPipelineStageFlags graphics_vulkan_stage_from_image_layout(GPUImageLayout layout)
	{
		VkPipelineStageFlags flags = 0u;

		switch (layout)
		{
		case GPUImageLayout_Undefined:
			flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case GPUImageLayout_RenderTarget:
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case GPUImageLayout_DepthStencil:
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		case GPUImageLayout_General:
		case GPUImageLayout_DepthStencilReadOnly:
		case GPUImageLayout_ShaderResource:
			flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			break;
		default:
			sv::log_warning("Undefined Image Layout");
			break;
		}

		return flags;
	}

	inline VkPipelineStageFlags graphics_vulkan_aspect_from_image_layout(GPUImageLayout layout, Format format = Format_Unknown)
	{
		VkImageAspectFlags flags = 0u;

		switch (layout)
		{
		case GPUImageLayout_Undefined:
		case GPUImageLayout_General:
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		case GPUImageLayout_RenderTarget:
		case GPUImageLayout_ShaderResource:
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		case GPUImageLayout_DepthStencil:
		case GPUImageLayout_DepthStencilReadOnly:

			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;

			if (format == Format_Unknown || sv::graphics_format_has_stencil(format)) {
				flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			break;
		default:
			sv::log_warning("Undefined Image Layout");
			break;
		}

		return flags;
	}

}