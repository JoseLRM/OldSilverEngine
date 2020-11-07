#pragma once

#include "platform/graphics.h"

namespace sv {

	typedef void RendererDebugBatch;

	Result debug_renderer_batch_create(RendererDebugBatch** pBatch);
	Result debug_renderer_batch_destroy(RendererDebugBatch* batch);

	void debug_renderer_batch_reset(RendererDebugBatch* batch);
	void debug_renderer_batch_render(RendererDebugBatch* batch, GPUImage* renderTarget, const Viewport& viewport, const Scissor& scissor, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	// Draw calls

	void debug_renderer_draw_quad(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color);
	void debug_renderer_draw_line(RendererDebugBatch* batch, const vec3f& p0, const vec3f& p1, Color color);
	void debug_renderer_draw_ellipse(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color);
	void debug_renderer_draw_sprite(RendererDebugBatch* batch, const XMMATRIX& matrix, Color color, GPUImage* image);

	// Helper draw calls

	void debug_renderer_draw_quad(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color);
	void debug_renderer_draw_quad(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color);
	void debug_renderer_draw_quad(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color);

	void debug_renderer_draw_ellipse(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color);
	void debug_renderer_draw_ellipse(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color);
	void debug_renderer_draw_ellipse(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color);

	void debug_renderer_draw_sprite(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color, GPUImage* image);
	void debug_renderer_draw_sprite(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color, GPUImage* image);
	void debug_renderer_draw_sprite(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color, GPUImage* image);

	// Attributes

	// Line rasterization width (pixels)
	void	debug_renderer_linewidth_set(RendererDebugBatch* batch, float lineWidth);
	float	debug_renderer_linewidth_get(RendererDebugBatch* batch);

	// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
	void	debug_renderer_stroke_set(RendererDebugBatch* batch, float stroke);
	float	debug_renderer_stroke_get(RendererDebugBatch* batch);

	// Sprite texCoords
	void	debug_renderer_texcoord_set(RendererDebugBatch* batch, const vec4f& texCoord);
	vec4f	debug_renderer_texcoord_get(RendererDebugBatch* batch);

	// Sprite sampler
	void debug_renderer_sampler_set_default(RendererDebugBatch* batch);
	void debug_renderer_sampler_set(RendererDebugBatch* batch, Sampler* sampler);

	// High level draw calls

	void debug_renderer_draw_grid_orthographic(RendererDebugBatch* batch, const vec2f& position, const vec2f& size, float gridSize, Color color);

}