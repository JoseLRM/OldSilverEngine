#pragma once

#include "..//core.h"

//////////////////////////////////////////////// TYPES ////////////////////////////////////////////////

namespace _sv {
	struct Primitive_internal;
	struct Buffer_internal;
	struct Image_internal;
	struct Sampler_internal;
	struct RenderPass_internal;
	struct Shader_internal;
	struct GraphicsPipeline_internal;
}

namespace sv {
	struct Primitive;
	struct Buffer;
	struct Image;
	struct Sampler;
	struct RenderPass;
	struct Shader;
	struct GraphicsPipeline;

	typedef ui8  GfxFlags8;
	typedef ui16 GfxFlags16;
	typedef ui32 GfxFlags32;
	typedef ui64 GfxFlags64;
}

// Define Temporal Macros
#define SV_GFX_TYPEDEF(x) namespace sv { typedef x; }

enum SV_GFX_API {
	SV_GFX_API_INVALID,
	SV_GFX_API_DX11,
	SV_GFX_API_VULKAN,
};

//////////////////////////////////////////////// PROPERTIES ////////////////////////////////////////////////

enum SV_GFX_PRIMITIVE : ui8 {
	SV_GFX_PRIMITIVE_INVALID,
	SV_GFX_PRIMITIVE_IMAGE,
	SV_GFX_PRIMITIVE_SAMPLER,
	SV_GFX_PRIMITIVE_BUFFER,
	SV_GFX_PRIMITIVE_SHADER,
	SV_GFX_PRIMITIVE_RENDERPASS,
	SV_GFX_PRIMITIVE_FRAMEBUFFER,
	SV_GFX_PRIMITIVE_GRAPHICS_PIPELINE,
};

enum SV_GFX_PIPELINE_MODE : ui8 {
	SV_GFX_PIPELINE_MODE_GRAPHICS,
	SV_GFX_PIPELINE_MODE_COMPUTE,
};

enum SV_GFX_BUFFER_TYPE : ui8 {
	SV_GFX_BUFFER_TYPE_INVALID,
	SV_GFX_BUFFER_TYPE_VERTEX,
	SV_GFX_BUFFER_TYPE_INDEX,
	SV_GFX_BUFFER_TYPE_CONSTANT,
};

enum SV_GFX_SHADER_TYPE : ui8 {
	SV_GFX_SHADER_TYPE_VERTEX,
	SV_GFX_SHADER_TYPE_PIXEL,
	SV_GFX_SHADER_TYPE_GEOMETRY,
	SV_GFX_SHADER_TYPE_HULL,
	SV_GFX_SHADER_TYPE_DOMAIN,
	SV_GFX_SHADER_TYPE_GFX_COUNT = SV_GFX_SHADER_TYPE_DOMAIN,

	SV_GFX_SHADER_TYPE_COMPUTE,
};

enum SV_GFX_IMAGE_TYPE : ui8 {
	SV_GFX_IMAGE_TYPE_RENDER_TARGET		= SV_BIT(0),
	SV_GFX_IMAGE_TYPE_SHADER_RESOURCE	= SV_BIT(1),
	SV_GFX_IMAGE_TYPE_DEPTH_STENCIL		= SV_BIT(2),
};
SV_GFX_TYPEDEF(GfxFlags8 ImageTypeFlags)

// Resource Usage
enum SV_GFX_USAGE : ui8 {
	SV_GFX_USAGE_DEFAULT,
	SV_GFX_USAGE_STATIC,
	SV_GFX_USAGE_DYNAMIC,
	SV_GFX_USAGE_STAGING
};
// CPU Access
enum SV_GFX_CPU_ACCESS : ui8 {
	SV_GFX_CPU_ACCESS_NONE = 0u,
	SV_GFX_CPU_ACCESS_READ = SV_BIT(0),
	SV_GFX_CPU_ACCESS_WRITE = SV_BIT(1),
	SV_GFX_CPU_ACCESS_ALL = SV_GFX_CPU_ACCESS_READ | SV_GFX_CPU_ACCESS_WRITE,
};
SV_GFX_TYPEDEF(GfxFlags8 CPUAccessFlags)
// Image Layout
enum SV_GFX_IMAGE_LAYOUT {
	SV_GFX_IMAGE_LAYOUT_UNDEFINED,
	SV_GFX_IMAGE_LAYOUT_GENERAL,
	SV_GFX_IMAGE_LAYOUT_RENDER_TARGET,
	SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL,
	SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY,
	SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE,
};
// Format
enum SV_GFX_FORMAT {
	SV_GFX_FORMAT_UNKNOWN,
	SV_GFX_FORMAT_R32G32B32A32_FLOAT,
	SV_GFX_FORMAT_R32G32B32A32_UINT,
	SV_GFX_FORMAT_R32G32B32A32_SINT,
	SV_GFX_FORMAT_R32G32B32_FLOAT,
	SV_GFX_FORMAT_R32G32B32_UINT,
	SV_GFX_FORMAT_R32G32B32_SINT,
	SV_GFX_FORMAT_R16G16B16A16_FLOAT,
	SV_GFX_FORMAT_R16G16B16A16_UNORM,
	SV_GFX_FORMAT_R16G16B16A16_UINT,
	SV_GFX_FORMAT_R16G16B16A16_SNORM,
	SV_GFX_FORMAT_R16G16B16A16_SINT,
	SV_GFX_FORMAT_R32G32_FLOAT,
	SV_GFX_FORMAT_R32G32_UINT,
	SV_GFX_FORMAT_R32G32_SINT,
	SV_GFX_FORMAT_R8G8B8A8_UNORM,
	SV_GFX_FORMAT_R8G8B8A8_SRGB,
	SV_GFX_FORMAT_R8G8B8A8_UINT,
	SV_GFX_FORMAT_R8G8B8A8_SNORM,
	SV_GFX_FORMAT_R8G8B8A8_SINT,
	SV_GFX_FORMAT_R16G16_FLOAT,
	SV_GFX_FORMAT_R16G16_UNORM,
	SV_GFX_FORMAT_R16G16_UINT,
	SV_GFX_FORMAT_R16G16_SNORM,
	SV_GFX_FORMAT_R16G16_SINT,
	SV_GFX_FORMAT_D32_FLOAT,
	SV_GFX_FORMAT_R32_FLOAT,
	SV_GFX_FORMAT_R32_UINT,
	SV_GFX_FORMAT_R32_SINT,
	SV_GFX_FORMAT_D24_UNORM_S8_UINT,
	SV_GFX_FORMAT_R8G8_UNORM,
	SV_GFX_FORMAT_R8G8_UINT,
	SV_GFX_FORMAT_R8G8_SNORM,
	SV_GFX_FORMAT_R8G8_SINT,
	SV_GFX_FORMAT_R16_FLOAT,
	SV_GFX_FORMAT_D16_UNORM,
	SV_GFX_FORMAT_R16_UNORM,
	SV_GFX_FORMAT_R16_UINT,
	SV_GFX_FORMAT_R16_SNORM,
	SV_GFX_FORMAT_R16_SINT,
	SV_GFX_FORMAT_R8_UNORM,
	SV_GFX_FORMAT_R8_UINT,
	SV_GFX_FORMAT_R8_SNORM,
	SV_GFX_FORMAT_R8_SINT,
	SV_GFX_FORMAT_BC1_UNORM,
	SV_GFX_FORMAT_BC1_SRGB,
	SV_GFX_FORMAT_BC2_UNORM,
	SV_GFX_FORMAT_BC2_SRGB,
	SV_GFX_FORMAT_BC3_UNORM,
	SV_GFX_FORMAT_BC3_SRGB,
	SV_GFX_FORMAT_BC4_UNORM,
	SV_GFX_FORMAT_BC4_SNORM,
	SV_GFX_FORMAT_BC5_UNORM,
	SV_GFX_FORMAT_BC5_SNORM,
	SV_GFX_FORMAT_B8G8R8A8_UNORM,
	SV_GFX_FORMAT_B8G8R8A8_SRGB,

};
// Topology
enum SV_GFX_TOPOLOGY {
	SV_GFX_TOPOLOGY_POINTS,
	SV_GFX_TOPOLOGY_LINES,
	SV_GFX_TOPOLOGY_LINE_STRIP,
	SV_GFX_TOPOLOGY_TRIANGLES,
	SV_GFX_TOPOLOGY_TRIANGLE_STRIP,
};
// Index Type
enum SV_GFX_INDEX_TYPE : ui8 {
	SV_GFX_INDEX_TYPE_16_BITS,
	SV_GFX_INDEX_TYPE_32_BITS,
};
// Cull Mode
enum SV_GFX_CULL_MODE {
	SV_GFX_CULL_NONE,
	SV_GFX_CULL_FRONT,
	SV_GFX_CULL_BACK,
	SV_GFX_CULL_FRONT_AND_BACK,
};
// Load operations
enum SV_GFX_LOAD_OP {
	SV_GFX_LOAD_OP_DONT_CARE,
	SV_GFX_LOAD_OP_LOAD,
	SV_GFX_LOAD_OP_CLEAR
};
// Store operations
enum SV_GFX_STORE_OP {
	SV_GFX_STORE_OP_DONT_CARE,
	SV_GFX_STORE_OP_STORE
};
// Attachments Types
enum SV_GFX_ATTACHMENT_TYPE {
	SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET,
	SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL
};
// Compare Operations
enum SV_GFX_COMPARE_OP {
	SV_GFX_COMPARE_OP_NEVER,
	SV_GFX_COMPARE_OP_LESS,
	SV_GFX_COMPARE_OP_EQUAL,
	SV_GFX_COMPARE_OP_LESS_OR_EQUAL,
	SV_GFX_COMPARE_OP_GREATER,
	SV_GFX_COMPARE_OP_NOT_EQUAL,
	SV_GFX_COMPARE_OP_GREATER_OR_EQUAL,
	SV_GFX_COMPARE_OP_ALWAYS,
};
// Stencil Operations
enum SV_GFX_STENCIL_OP {
	SV_GFX_STENCIL_OP_KEEP = 0,
	SV_GFX_STENCIL_OP_ZERO = 1,
	SV_GFX_STENCIL_OP_REPLACE = 2,
	SV_GFX_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
	SV_GFX_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
	SV_GFX_STENCIL_OP_INVERT = 5,
	SV_GFX_STENCIL_OP_INCREMENT_AND_WRAP = 6,
	SV_GFX_STENCIL_OP_DECREMENT_AND_WRAP = 7,
};
// Blend Operations
enum SV_GFX_BLEND_OP {
	SV_GFX_BLEND_OP_ADD,
	SV_GFX_BLEND_OP_SUBTRACT,
	SV_GFX_BLEND_OP_REVERSE_SUBTRACT,
	SV_GFX_BLEND_OP_MIN,
	SV_GFX_BLEND_OP_MAX,
};
// Color Components
enum SV_GFX_COLOR_COMPONENT {
	SV_GFX_COLOR_COMPONENT_NONE = 0,
	SV_GFX_COLOR_COMPONENT_R	= SV_BIT(0),
	SV_GFX_COLOR_COMPONENT_G	= SV_BIT(0),
	SV_GFX_COLOR_COMPONENT_B	= SV_BIT(0),
	SV_GFX_COLOR_COMPONENT_A	= SV_BIT(0),
	SV_GFX_COLOR_COMPONENT_RGB	= (SV_GFX_COLOR_COMPONENT_R | SV_GFX_COLOR_COMPONENT_G | SV_GFX_COLOR_COMPONENT_B),
	SV_GFX_COLOR_COMPONENT_ALL  = (SV_GFX_COLOR_COMPONENT_RGB | SV_GFX_COLOR_COMPONENT_A),
};
SV_GFX_TYPEDEF(GfxFlags8 ColorComponentFlags)
// Blend Factors
enum SV_GFX_BLEND_FACTOR {
	SV_GFX_BLEND_FACTOR_ZERO,
	SV_GFX_BLEND_FACTOR_ONE,
	SV_GFX_BLEND_FACTOR_SRC_COLOR,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	SV_GFX_BLEND_FACTOR_DST_COLOR,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	SV_GFX_BLEND_FACTOR_SRC_ALPHA,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	SV_GFX_BLEND_FACTOR_DST_ALPHA,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	SV_GFX_BLEND_FACTOR_CONSTANT_COLOR,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	SV_GFX_BLEND_FACTOR_CONSTANT_ALPHA,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
	SV_GFX_BLEND_FACTOR_SRC_ALPHA_SATURATE,
	SV_GFX_BLEND_FACTOR_SRC1_COLOR,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
	SV_GFX_BLEND_FACTOR_SRC1_ALPHA,
	SV_GFX_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
};

// Filters
enum SV_GFX_FILTER {
	SV_GFX_FILTER_NEAREST,
	SV_GFX_FILTER_LINEAR,
};
// Address Mode
enum SV_GFX_ADDRESS_MODE {
	SV_GFX_ADDRESS_MODE_WRAP,
	SV_GFX_ADDRESS_MODE_MIRROR,
	SV_GFX_ADDRESS_MODE_CLAMP,
	SV_GFX_ADDRESS_MODE_BORDER
};
// Border Color
enum SV_GFX_BORDER_COLOR {

};
// barriers types
enum SV_GFX_BARRIER_TYPE : ui8 {
	SV_GFX_BARRIER_TYPE_IMAGE
};

////////////////////////////////////////////////// GRAPHICS STRUCTS ////////////////////////////////////////////////

namespace sv {

	// Viewport
	struct Viewport {
		float x, y, width, height, minDepth, maxDepth;
	};
	// Scissor
	struct Scissor {
		float x, y, width, height;
	};
	// Barriers
	struct GPUImageBarrier {
		Image*				pImage;
		SV_GFX_IMAGE_LAYOUT oldLayout;
		SV_GFX_IMAGE_LAYOUT newLayout;
	};
	struct GPUBarrier {
		union {
			GPUImageBarrier image;
		};
		SV_GFX_BARRIER_TYPE type;

		inline static GPUBarrier Image(Image& image, SV_GFX_IMAGE_LAYOUT oldLayout, SV_GFX_IMAGE_LAYOUT newLayout)
		{
			GPUBarrier barrier;
			barrier.image.pImage = &image;
			barrier.image.oldLayout = oldLayout;
			barrier.image.newLayout = newLayout;
			barrier.type = SV_GFX_BARRIER_TYPE_IMAGE;
			return barrier;
		}
	};

}

////////////////////////////////////////////////// PRIMITIVE CREATION STRUCTS ////////////////////////////////////////////////

// Buffer Struct
struct SV_GFX_BUFFER_DESC {
	SV_GFX_BUFFER_TYPE	bufferType;
	SV_GFX_USAGE		usage;
	sv::CPUAccessFlags	CPUAccess;
	ui32				size;
	SV_GFX_INDEX_TYPE	indexType;
	void*				pData;
};
// Image Struct
struct SV_GFX_IMAGE_DESC {
	void*					pData;
	ui32					size;
	SV_GFX_FORMAT			format;
	SV_GFX_IMAGE_LAYOUT		layout;
	sv::ImageTypeFlags		type;
	SV_GFX_USAGE			usage;
	sv::CPUAccessFlags		CPUAccess;
	ui8						dimension;
	ui32					width;
	ui32					height;
	ui32					depth;
	ui32					layers;
};
// Sampler Struct
struct SV_GFX_SAMPLER_DESC {
	SV_GFX_ADDRESS_MODE addressModeU;
	SV_GFX_ADDRESS_MODE addressModeV;
	SV_GFX_ADDRESS_MODE addressModeW;
	SV_GFX_FILTER		minFilter;
	SV_GFX_FILTER		magFilter;
};
// Shader Struct
struct SV_GFX_SHADER_DESC {
	SV_GFX_SHADER_TYPE	shaderType;
	const char*			filePath;
};
// RenderPass Structs
struct SV_GFX_ATTACHMENT_DESC {
	SV_GFX_LOAD_OP			loadOp;
	SV_GFX_STORE_OP			storeOp;
	SV_GFX_LOAD_OP			stencilLoadOp;
	SV_GFX_STORE_OP			stencilStoreOp;
	SV_GFX_FORMAT			format;
	SV_GFX_IMAGE_LAYOUT		initialLayout;
	SV_GFX_IMAGE_LAYOUT		layout;
	SV_GFX_IMAGE_LAYOUT		finalLayout;
	SV_GFX_ATTACHMENT_TYPE	type;
};
struct SV_GFX_RENDERPASS_DESC {
	std::vector<SV_GFX_ATTACHMENT_DESC> attachments;
};
// GraphicsPipeline Struct
struct SV_GFX_INPUT_ELEMENT_DESC {
	ui32			inputSlot;
	const char*		name;
	ui32			index;
	ui32			offset;
	SV_GFX_FORMAT	format;
};
struct SV_GFX_INPUT_SLOT_DESC {
	ui32 slot;
	ui32 stride;
	bool instanced;
};
struct SV_GFX_INPUT_LAYOUT_DESC {
	std::vector<SV_GFX_INPUT_SLOT_DESC> slots;
	std::vector<SV_GFX_INPUT_ELEMENT_DESC> elements;
};
struct SV_GFX_BLEND_ATTACHMENT_DESC {
	bool blendEnabled;
	SV_GFX_BLEND_FACTOR		srcColorBlendFactor;
	SV_GFX_BLEND_FACTOR		dstColorBlendFactor;
	SV_GFX_BLEND_OP			colorBlendOp;
	SV_GFX_BLEND_FACTOR		srcAlphaBlendFactor;
	SV_GFX_BLEND_FACTOR		dstAlphaBlendFactor;
	SV_GFX_BLEND_OP			alphaBlendOp;
	sv::ColorComponentFlags colorWriteMask;
};
struct SV_GFX_BLEND_STATE_DESC {
	std::vector<SV_GFX_BLEND_ATTACHMENT_DESC>	attachments;
	sv::vec4									blendConstants;
};
struct SV_GFX_RASTERIZER_STATE_DESC {
	bool				wireframe;
	float				lineWidth;
	SV_GFX_CULL_MODE	cullMode;
	bool				clockwise;
};
struct SV_GFX_STENCIL_STATE {
	SV_GFX_STENCIL_OP   failOp;
	SV_GFX_STENCIL_OP   passOp;
	SV_GFX_STENCIL_OP   depthFailOp;
	SV_GFX_COMPARE_OP   compareOp;
	ui32				compareMask;
	ui32				writeMask;
	ui32				reference;
};
struct SV_GFX_DEPTHSTENCIL_STATE_DESC {
	bool					depthTestEnabled;
	bool					depthWriteEnabled;
	SV_GFX_COMPARE_OP		depthCompareOp;
	bool					stencilTestEnabled;
	SV_GFX_STENCIL_STATE	front;
	SV_GFX_STENCIL_STATE	back;
};
struct SV_GFX_GRAPHICS_PIPELINE_DESC {
	sv::Shader*									pVertexShader;
	sv::Shader*									pPixelShader;
	sv::Shader*									pGeometryShader;
	const SV_GFX_INPUT_LAYOUT_DESC*				pInputLayout;
	const SV_GFX_BLEND_STATE_DESC*				pBlendState;
	const SV_GFX_RASTERIZER_STATE_DESC*			pRasterizerState;
	const SV_GFX_DEPTHSTENCIL_STATE_DESC*		pDepthStencilState;
	SV_GFX_TOPOLOGY								topology;
};

////////////////////////////////////////////////// FUNCTIONS ////////////////////////////////////////////////

namespace sv {
	inline bool graphics_format_has_stencil(SV_GFX_FORMAT format)
	{
		if (format == SV_GFX_FORMAT_D24_UNORM_S8_UINT)
			return true;
		else return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Undefine Temporal Macros
#undef SV_GFX_TYPEDEF