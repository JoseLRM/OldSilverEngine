#pragma once

#include "renderer/objects/Texture.h"

namespace sv {

	typedef void RendererDebugBatch;

	Result renderer_debug_batch_create(RendererDebugBatch** pBatch);
	Result renderer_debug_batch_destroy(RendererDebugBatch* batch);

	void renderer_debug_batch_reset(RendererDebugBatch* batch);
	void renderer_debug_batch_render(RendererDebugBatch* batch, GPUImage& renderTarget, const Viewport& viewport, const Scissor& scissor, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	// Draw calls

	void renderer_debug_draw_quad(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color);
	void renderer_debug_draw_line(RendererDebugBatch* batch, const vec3& p0, const vec3& p1, Color color);
	void renderer_debug_draw_ellipse(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color);
	void renderer_debug_draw_sprite(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color, GPUImage& image);

	// Helper draw calls

	void renderer_debug_draw_quad(RendererDebugBatch* batch, const vec3& position, const vec2& size, Color color);
	void renderer_debug_draw_quad(RendererDebugBatch* batch, const vec3& position, const vec2& size, const vec3& rotation, Color color);

	void renderer_debug_draw_ellipse(RendererDebugBatch* batch, const vec3& position, const vec2& size, Color color);
	void renderer_debug_draw_ellipse(RendererDebugBatch* batch, const vec3& position, const vec2& size, const vec3& rotation, Color color);

	void renderer_debug_draw_sprite(RendererDebugBatch* batch, const vec3& position, const vec2& size, Color color, GPUImage& image);
	void renderer_debug_draw_sprite(RendererDebugBatch* batch, const vec3& position, const vec2& size, const vec3& rotation, Color color, GPUImage& image);

	// Attributes

	// Line rasterization width (pixels)
	void	renderer_debug_linewidth_set(RendererDebugBatch* batch, float lineWidth);
	float	renderer_debug_linewidth_get(RendererDebugBatch* batch);

	// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
	void	renderer_debug_stroke_set(RendererDebugBatch* batch, float stroke);
	float	renderer_debug_stroke_get(RendererDebugBatch* batch);

	// Sprite texCoords
	void renderer_debug_texcoord_set(RendererDebugBatch* batch, const vec4& texCoord);
	vec4 renderer_debug_texcoord_get(RendererDebugBatch* batch);

	// Sprite sampler
	void renderer_debug_sampler_set_default(RendererDebugBatch* batch);
	void renderer_debug_sampler_set(RendererDebugBatch* batch, Sampler& sampler);

}