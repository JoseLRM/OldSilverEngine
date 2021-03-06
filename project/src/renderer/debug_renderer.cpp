#include "SilverEngine/core.h"

#include "SilverEngine/utils/allocators/FrameList.h"
#include "renderer/renderer_internal.h"

namespace sv {

	struct DebugQuad {
		XMMATRIX matrix;
		Color color;
		
		DebugQuad(const XMMATRIX& matrix, Color color) : matrix(matrix), color(color) {}
	};

	struct DebugLine {
		v3_f32 p0, p1;
		Color color;
		
		DebugLine(const v3_f32& p0, const v3_f32& p1, Color color) : p0(p0), p1(p1), color(color) {}
	};

	struct DebugTriangle {
		v3_f32 p0, p1, p2;
		Color color;

		DebugTriangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color) : p0(p0), p1(p1), p2(p2), color(color) {}
	};

	struct DebugMeshWireframe {
		Mesh* mesh;
		XMMATRIX transform;
		Color color;

		DebugMeshWireframe(Mesh* mesh, const XMMATRIX& transform, Color color) : mesh(mesh), transform(transform), color(color) {}
	};

	struct DebugRendererBatch {

		FrameList<DebugQuad> quads;
		FrameList<DebugLine> lines;
		FrameList<DebugTriangle> triangles;
		FrameList<DebugMeshWireframe> mesh_wireframes;
		
	};

#define SV_PARSE_BATCH() sv::DebugRendererBatch& batch = sv::debug_batches[cmd]
	
	static DebugRendererBatch debug_batches[GraphicsLimit_CommandList];

	void begin_debug_batch(CommandList cmd)
	{
		SV_PARSE_BATCH();

		batch.quads.reset();
		batch.lines.reset();
		batch.triangles.reset();
		batch.mesh_wireframes.reset();
	}

	constexpr u32 QUAD_COUNT = 100u;
	constexpr u32 LINE_COUNT = 100u;
	constexpr u32 TRIANGLE_COUNT = 100u;
	
	void end_debug_batch(GPUImage* offscreen, GPUImage* depthstencil, const XMMATRIX& view_projection_matrix, CommandList cmd)
	{
		SV_PARSE_BATCH();

		GPUImage* att[2u];
		att[0] = offscreen;
		att[1] = depthstencil;

		RenderPass* renderpass;
		
		if (depthstencil == nullptr) {

			renderpass = gfx.renderpass_off;
			graphics_depthstencilstate_unbind(cmd);
			graphics_blendstate_bind(gfx.bs_transparent, cmd);
		}
		else {

			renderpass = gfx.renderpass_world;
			graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
			graphics_blendstate_unbind(cmd);
		}

		graphics_rasterizerstate_unbind(cmd);
			
		// DRAW QUADS
		if (batch.quads.size()) {

			GPUBuffer* buffer = get_batch_buffer(QUAD_COUNT * sizeof(DebugVertex_Solid) * 6u, cmd);

			graphics_inputlayoutstate_bind(gfx.ils_debug_solid_batch, cmd);
			graphics_shader_bind(gfx.vs_debug_solid_batch, cmd);
			graphics_shader_bind(gfx.ps_debug_solid, cmd);
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);

			DebugVertex_Solid* data = (DebugVertex_Solid*)batch_data[cmd];
			DebugVertex_Solid* it = data;
			DebugVertex_Solid* end = data + QUAD_COUNT * 6u;

			XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
			XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
			XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
			XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

			XMVECTOR v0;
			XMVECTOR v1;
			XMVECTOR v2;
			XMVECTOR v3;

			XMMATRIX mvp_matrix;

			for (const DebugQuad& q : batch.quads) {
				
				mvp_matrix = q.matrix * view_projection_matrix;

				v0 = XMVector4Transform(p0, mvp_matrix);
				v1 = XMVector4Transform(p1, mvp_matrix);
				v2 = XMVector4Transform(p2, mvp_matrix);
				v3 = XMVector4Transform(p3, mvp_matrix);

				*it = { v4_f32(v0), q.color }; ++it;
				*it = { v4_f32(v1), q.color }; ++it;
				*it = { v4_f32(v2), q.color }; ++it;

				*it = { v4_f32(v1), q.color }; ++it;
				*it = { v4_f32(v3), q.color }; ++it;
				*it = { v4_f32(v2), q.color }; ++it;

				if (it == end) {

					graphics_buffer_update(buffer, data, QUAD_COUNT * sizeof(DebugVertex_Solid) * 6u, 0u, cmd);

					graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
					graphics_draw(QUAD_COUNT * 6u, 1u, 0u, 0u, cmd);
					graphics_renderpass_end(cmd);

					it = data;
				}
			}

			if (it != data) {
				graphics_buffer_update(buffer, data, (it - data) * sizeof(DebugVertex_Solid), 0u, cmd);

				graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
				graphics_draw(it - data, 1u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}
		}

		// DRAW LINES
		if (batch.lines.size()) {

			GPUBuffer* buffer = get_batch_buffer(LINE_COUNT * sizeof(DebugVertex_Solid) * 2u, cmd);

			graphics_inputlayoutstate_bind(gfx.ils_debug_solid_batch, cmd);
			graphics_shader_bind(gfx.vs_debug_mesh_wireframe, cmd);
			graphics_shader_bind(gfx.ps_debug_solid, cmd);
			graphics_topology_set(GraphicsTopology_Lines, cmd);
			graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);

			DebugVertex_Solid* data = (DebugVertex_Solid*)batch_data[cmd];
			DebugVertex_Solid* it = data;
			DebugVertex_Solid* end = data + LINE_COUNT * 2u;

			XMVECTOR v0;
			XMVECTOR v1;

			for (const DebugLine& l : batch.lines) {

				v0 = XMVector4Transform(l.p0.getDX(1.f), view_projection_matrix);
				v1 = XMVector4Transform(l.p1.getDX(1.f), view_projection_matrix);

				*it = { v4_f32(v0), l.color }; ++it;
				*it = { v4_f32(v1), l.color }; ++it;

				if (it == end) {

					graphics_buffer_update(buffer, data, LINE_COUNT * sizeof(DebugVertex_Solid) * 2u, 0u, cmd);

					graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
					graphics_draw(LINE_COUNT * 2u, 1u, 0u, 0u, cmd);
					graphics_renderpass_end(cmd);

					it = data;
				}
			}

			if (it != data) {
				graphics_buffer_update(buffer, data, (it - data) * sizeof(DebugVertex_Solid), 0u, cmd);

				graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
				graphics_draw(it - data, 1u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}
		}

		// DRAW TRIANGLES
		if (batch.triangles.size()) {

			GPUBuffer* buffer = get_batch_buffer(TRIANGLE_COUNT * sizeof(DebugVertex_Solid) * 3u, cmd);

			graphics_inputlayoutstate_bind(gfx.ils_debug_solid_batch, cmd);
			graphics_shader_bind(gfx.vs_debug_solid_batch, cmd);
			graphics_shader_bind(gfx.ps_debug_solid, cmd);
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);

			DebugVertex_Solid* data = (DebugVertex_Solid*)batch_data[cmd];
			DebugVertex_Solid* it = data;
			DebugVertex_Solid* end = data + TRIANGLE_COUNT * 3u;

			XMVECTOR v0;
			XMVECTOR v1;
			XMVECTOR v2;

			for (const DebugTriangle& t : batch.triangles) {

				v0 = XMVector4Transform(t.p0.getDX(1.f), view_projection_matrix);
				v1 = XMVector4Transform(t.p1.getDX(1.f), view_projection_matrix);
				v2 = XMVector4Transform(t.p2.getDX(1.f), view_projection_matrix);

				*it = { v4_f32(v0), t.color }; ++it;
				*it = { v4_f32(v1), t.color }; ++it;
				*it = { v4_f32(v2), t.color }; ++it;

				if (it == end) {

					graphics_buffer_update(buffer, data, TRIANGLE_COUNT * sizeof(DebugVertex_Solid) * 3u, 0u, cmd);

					graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
					graphics_draw(TRIANGLE_COUNT * 3u, 1u, 0u, 0u, cmd);
					graphics_renderpass_end(cmd);

					it = data;
				}
			}

			if (it != data) {
				graphics_buffer_update(buffer, data, (it - data) * sizeof(DebugVertex_Solid), 0u, cmd);

				graphics_renderpass_begin(renderpass, att, nullptr, 1.f, 0u, cmd);
				graphics_draw(it - data, 1u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}
		}

		// DRAW MESH WIREFRAMES
		if (batch.mesh_wireframes.size()) {

			graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
			graphics_shader_bind(gfx.vs_debug_mesh_wireframe, cmd);
			graphics_shader_bind(gfx.ps_debug_solid, cmd);
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_debug_mesh, 0u, ShaderType_Vertex, cmd);
			graphics_rasterizerstate_bind(gfx.rs_wireframe, cmd);

			for (const DebugMeshWireframe& t : batch.mesh_wireframes) {

				graphics_vertexbuffer_bind(t.mesh->vbuffer, 0u, 0u, cmd);
				graphics_indexbuffer_bind(t.mesh->ibuffer, 0u, cmd);

				struct {
					XMMATRIX matrix;
					Color4f color;
				} data;
				
				data.matrix = t.transform * view_projection_matrix;
				data.color = { f32(t.color.r) / 255.f, f32(t.color.g) / 255.f, f32(t.color.b) / 255.f, f32(t.color.a) / 255.f };

				graphics_buffer_update(gfx.cbuffer_debug_mesh, &data, sizeof(data), 0u, cmd);

				graphics_renderpass_begin(renderpass, att, cmd);
				graphics_draw_indexed(t.mesh->indices.size(), 1u, 0u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}
		}
	}

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.quads.emplace_back(matrix, color);
	}
	
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.lines.emplace_back(p0, p1, color);
	}

	void draw_debug_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.triangles.emplace_back(p0, p1, p2, color);
	}

	void draw_debug_mesh_wireframe(Mesh* mesh, const XMMATRIX& transform, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.mesh_wireframes.emplace_back(mesh, transform, color);
	}

	// HELPER
	
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationQuaternion(rotationQuat.get_dx()) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}
	
	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd)
	{
		v2_f32 begin = position - offset - size / 2.f;
		v2_f32 end = begin + size;

		for (f32 y = i32(begin.y / gridSize.y) * gridSize.y; y < end.y; y += gridSize.y) {
			draw_debug_line({ begin.x + offset.x, y + offset.y, 0.f }, { end.x + offset.x, y + offset.y, 0.f }, color, cmd);
		}
		for (f32 x = i32(begin.x / gridSize.x) * gridSize.x; x < end.x; x += gridSize.x) {
			draw_debug_line({ x + offset.x, begin.y + offset.y, 0.f }, { x + offset.x, end.y + offset.y, 0.f }, color, cmd);
		}
	}
}
