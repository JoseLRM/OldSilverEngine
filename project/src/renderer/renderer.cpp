#include "SilverEngine/core.h"

#include "renderer/renderer_internal.h"

#include "SilverEngine/mesh.h"
#include "SilverEngine/utils/allocators/FrameList.h"

namespace sv {

	GraphicsObjects		gfx = {};
	u8* batch_data[GraphicsLimit_CommandList] = {};
	Font				font_opensans;
	Font				font_console;

	// SHADER COMPILATION

#define COMPILE_SHADER(type, shader, path, alwais_compile) svCheck(graphics_shader_compile_fastbin_from_file(path, type, &shader, SV_SYS("system/shaders/") path, alwais_compile));
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)

	static Result compile_shaders()
	{
		COMPILE_VS_(gfx.vs_debug_solid_batch, "debug/solid_batch.hlsl");
		COMPILE_PS_(gfx.ps_debug_solid_batch, "debug/solid_batch.hlsl");

		COMPILE_VS(gfx.vs_text, "text.hlsl");
		COMPILE_PS(gfx.ps_text, "text.hlsl");

		COMPILE_VS(gfx.vs_sprite, "sprite/default.hlsl");
		COMPILE_PS(gfx.ps_sprite, "sprite/default.hlsl");

		COMPILE_VS(gfx.vs_mesh, "mesh/default.hlsl");
		COMPILE_PS(gfx.ps_mesh, "mesh/default.hlsl");

		COMPILE_VS(gfx.vs_sky, "skymapping.hlsl");
		COMPILE_PS(gfx.ps_sky, "skymapping.hlsl");

		return Result_Success;
	}

	// RENDERPASSES CREATION

#define CREATE_RENDERPASS(name, renderpass) svCheck(graphics_renderpass_create(&desc, &renderpass));

	static Result create_renderpasses()
	{
		RenderPassDesc desc;
		AttachmentDesc att[GraphicsLimit_Attachment];
		desc.pAttachments = att;

		// Offscreen pass
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		desc.attachmentCount = 1u;
		CREATE_RENDERPASS("OffscreenRenderpass", gfx.renderpass_off);
		
		// World
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		att[1].loadOp = AttachmentOperation_Load;
		att[1].storeOp = AttachmentOperation_Store;
		att[1].stencilLoadOp = AttachmentOperation_Load;
		att[1].stencilStoreOp = AttachmentOperation_Store;
		att[1].format = ZBUFFER_FORMAT;
		att[1].initialLayout = GPUImageLayout_DepthStencil;
		att[1].layout = GPUImageLayout_DepthStencil;
		att[1].finalLayout = GPUImageLayout_DepthStencil;
		att[1].type = AttachmentType_DepthStencil;

		desc.attachmentCount = 2u;
		CREATE_RENDERPASS("WorldRenderPass", gfx.renderpass_world);

		return Result_Success;
	}

	// BUFFER CREATION

	static Result create_buffers()
	{
		GPUBufferDesc desc;

		// Camera buffer
		{
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(CameraBuffer_GPU);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_camera));
		}

		// set text index data
		{
			constexpr u32 index_count = TEXT_BATCH_COUNT * 6u;
			u16* index_data = new u16[index_count];

			for (u32 i = 0u; i < TEXT_BATCH_COUNT; ++i) {

				u32 currenti = i * 6u;
				u32 currentv = i * 4u;

				index_data[currenti + 0u] = 0 + currentv;
				index_data[currenti + 1u] = 1 + currentv;
				index_data[currenti + 2u] = 2 + currentv;

				index_data[currenti + 3u] = 1 + currentv;
				index_data[currenti + 4u] = 3 + currentv;
				index_data[currenti + 5u] = 2 + currentv;
			}

			// 10.922 characters
			desc.indexType = IndexType_16;
			desc.bufferType = GPUBufferType_Index;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.size = sizeof(u16) * index_count;
			desc.pData = index_data;

			svCheck(graphics_buffer_create(&desc, &gfx.ibuffer_text));
			graphics_name_set(gfx.ibuffer_text, "Text_IndexBuffer");

			delete[] index_data;
		}

		// Sprite index buffer
		{
			u32* index_data = new u32[SPRITE_BATCH_COUNT * 6u];
			{
				u32 indexCount = SPRITE_BATCH_COUNT * 6u;
				u32 index = 0u;
				for (u32 i = 0; i < indexCount; ) {

					index_data[i++] = index;
					index_data[i++] = index + 1;
					index_data[i++] = index + 2;
					index_data[i++] = index + 1;
					index_data[i++] = index + 3;
					index_data[i++] = index + 2;

					index += 4u;
				}
			}

			desc.bufferType = GPUBufferType_Index;
			desc.CPUAccess = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.pData = index_data;
			desc.size = SPRITE_BATCH_COUNT * 6u * sizeof(u32);
			desc.indexType = IndexType_32;

			svCheck(graphics_buffer_create(&desc, &gfx.ibuffer_sprite));
			delete[] index_data;

			graphics_name_set(gfx.ibuffer_sprite, "Sprite_IndexBuffer");
		}

		// Mesh
		{
			desc.pData = nullptr;

			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(MeshData);
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance));

			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(Material);
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_material));

			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(LightData) * LIGHT_COUNT;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_light_instances));
		}

		// Sky box
		{
			f32 box[] = {
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};


			desc.pData = box;
			desc.bufferType = GPUBufferType_Vertex;
			desc.size = sizeof(v3_f32) * 36u;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;

			svCheck(graphics_buffer_create(&desc, &gfx.vbuffer_skybox));

			desc.pData = nullptr;
			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(XMMATRIX);
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_skybox));
		}

		return Result_Success;
	}

	// IMAGE CREATION

	static Result create_images()
	{
		GPUImageDesc desc;

		// White Image
		{
			u8 bytes[4];
			for (u32 i = 0; i < 4; ++i) bytes[i] = 255u;

			desc.pData = bytes;
			desc.size = sizeof(u8) * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.width = 1u;
			desc.height = 1u;

			svCheck(graphics_image_create(&desc, &gfx.image_white));
		}

		Color* data;
		u32 w, h;
		svCheck(load_image(SV_SYS("system/skymap.jpg"), (void**)& data, &w, &h));


		u32 image_width = w / 4u;
		u32 image_height = h / 3u;

		Color* images[6u] = {};
		Color* mem = (Color*)malloc(image_width * image_height * 4u * 6u);

		foreach(i, 6u) {

			images[i] = mem + image_width * image_height * i;

			u32 xoff;
			u32 yoff;

			switch (i)
			{
			case 0:
				xoff = image_width;
				yoff = image_height;
				break;
			case 1:
				xoff = image_width * 3u;
				yoff = image_height;
				break;
			case 2:
				xoff = image_width;
				yoff = 0u;
				break;
			case 3:
				xoff = image_width;
				yoff = image_height * 2u;
				break;
			case 4:
				xoff = image_width * 2u;
				yoff = image_height;
				break;
			default:
				xoff = 0u;
				yoff = image_height;
				break;
			}

			for (u32 y = yoff; y < yoff + image_height; ++y) {

				Color* src = data + xoff + y * w;

				Color* dst = images[i] + (y - yoff) * image_width;
				Color* dst_end = dst + image_width;

				while (dst != dst_end) {

					*dst = *src;

					++src;
					++dst;
				}
			}
		}

		desc.pData = images;
		desc.size = image_width * image_height * 4u;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource | GPUImageType_CubeMap;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.width = image_width;
		desc.height = image_height;

		svCheck(graphics_image_create(&desc, &gfx.image_sky));

		free(mem);
		delete[] data;

		return Result_Success;
	}

	// STATE CREATION

	static Result create_states()
	{
		// Create InputlayoutStates
		{
			InputSlotDesc slots[GraphicsLimit_InputSlot];
			InputElementDesc elements[GraphicsLimit_InputElement];
			InputLayoutStateDesc desc;
			desc.pSlots = slots;
			desc.pElements = elements;

			// DEBUG
			slots[0].instanced = false;
			slots[0].slot = 0u;
			slots[0].stride = sizeof(DebugVertex_Solid);

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "Color", 0u, 0u, 4u * sizeof(f32), Format_R8G8B8A8_UNORM };

			desc.elementCount = 2u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_debug_solid_batch));

			// TEXT
			slots[0] = { 0u, sizeof(TextVertex), false };

			elements[0] = { "Position", 0u, 0u, sizeof(f32) * 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "TexCoord", 0u, 0u, sizeof(f32) * 4u, Format_R32G32_FLOAT };
			elements[2] = { "Color", 0u, 0u, sizeof(f32) * 6u, Format_R8G8B8A8_UNORM };

			desc.elementCount = 3u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_text));

			// SPRITE
			slots[0] = { 0u, sizeof(SpriteVertex), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "TexCoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT };
			elements[2] = { "Color", 0u, 0u, 6u * sizeof(f32), Format_R8G8B8A8_UNORM };

			desc.elementCount = 3u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_sprite));

			// MESH
			slots[0] = { 0u, sizeof(MeshVertex), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
			elements[1] = { "Normal", 0u, 0u, 3u * sizeof(f32), Format_R32G32B32_FLOAT };
			elements[2] = { "Tangent", 0u, 0u, 6u * sizeof(f32), Format_R32G32B32_FLOAT };
			elements[3] = { "Bitangent", 0u, 0u, 9u * sizeof(f32), Format_R32G32B32_FLOAT };
			elements[4] = { "Texcoord", 0u, 0u, 12u * sizeof(f32), Format_R32G32_FLOAT };

			desc.elementCount = 5u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_mesh));

			// SKY BOX
			slots[0] = { 0u, sizeof(v3_f32), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };

			desc.elementCount = 1u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_sky));
		}

		// Create BlendStates
		{
			BlendAttachmentDesc att[GraphicsLimit_AttachmentRT];
			BlendStateDesc desc;
			desc.pAttachments = att;

			// TRANSPARENT
			att[0].blendEnabled = true;
			att[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			att[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			att[0].colorBlendOp = BlendOperation_Add;
			att[0].srcAlphaBlendFactor = BlendFactor_One;
			att[0].dstAlphaBlendFactor = BlendFactor_One;
			att[0].alphaBlendOp = BlendOperation_Add;
			att[0].colorWriteMask = ColorComponent_All;

			desc.attachmentCount = 1u;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			svCheck(graphics_blendstate_create(&desc, &gfx.bs_transparent));

			// MESH
			att[0].blendEnabled = false;
			att[0].colorWriteMask = ColorComponent_All;

			desc.attachmentCount = 1u;

			svCheck(graphics_blendstate_create(&desc, &gfx.bs_mesh));
		}

		// Create DepthStencilStates
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = true;
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_Less;
			desc.stencilTestEnabled = false;

			graphics_depthstencilstate_create(&desc, &gfx.dss_default_depth);
		}

		// Create RasterizerStates
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.cullMode = RasterizerCullMode_Back;
			desc.clockwise = true;

			graphics_rasterizerstate_create(&desc, &gfx.rs_back_culling);
		}

		return Result_Success;
	}

	// SAMPLER CREATION

	static Result create_samplers()
	{
		SamplerDesc desc;

		desc.addressModeU = SamplerAddressMode_Wrap;
		desc.addressModeV = SamplerAddressMode_Wrap;
		desc.addressModeW = SamplerAddressMode_Wrap;
		desc.minFilter = SamplerFilter_Linear;
		desc.magFilter = SamplerFilter_Linear;
		svCheck(graphics_sampler_create(&desc, &gfx.sampler_def_linear));

		desc.addressModeU = SamplerAddressMode_Wrap;
		desc.addressModeV = SamplerAddressMode_Wrap;
		desc.addressModeW = SamplerAddressMode_Wrap;
		desc.minFilter = SamplerFilter_Nearest;
		desc.magFilter = SamplerFilter_Nearest;
		svCheck(graphics_sampler_create(&desc, &gfx.sampler_def_nearest));

		return Result_Success;
	}

	// RENDERER RUNTIME

	Result renderer_initialize()
	{
		svCheck(compile_shaders());
		svCheck(create_renderpasses());
		svCheck(create_samplers());
		svCheck(create_buffers());
		svCheck(create_images());
		svCheck(create_states());

		// Create default fonts
		svCheck(font_create(font_opensans, "$system/fonts/OpenSans-Regular.ttf", 228.f, 0u));
		font_create(font_console, "C:/Windows/Fonts/consola.ttf", 228.f, 0u);

		return Result_Success;
	}

	Result renderer_close()
	{
		// Free graphics objects
		graphics_destroy_struct(&gfx, sizeof(gfx));

		// Deallocte batch memory
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				if (batch_data[i]) {
					free(batch_data[i]);
					batch_data[i] = nullptr;
				}
			}
		}

		font_destroy(font_opensans);
		font_destroy(font_console);

		return Result_Success;
	}

	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texcoord;
		GPUImage* image;
		Color		color;
		u32			layer;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color, u32 layer)
			: tm(m), texcoord(texcoord), image(image), color(color), layer(layer) {}
	};

	struct MeshInstance {

		XMMATRIX	transform_matrix;
		Mesh* mesh;
		Material* material;

		MeshInstance(const XMMATRIX& transform_matrix, Mesh* mesh, Material* material)
			: transform_matrix(transform_matrix), mesh(mesh), material(material) {}

	};

	struct LightInstance {

		Color3f		color;
		LightType	type;
		v3_f32		position;
		f32			range;
		f32			intensity;
		f32			smoothness;

		// None
		LightInstance() = default;

		// Point
		LightInstance(const Color3f& color, const v3_f32& position, f32 range, f32 intensity, f32 smoothness)
			: color(color), type(LightType_Point), position(position), range(range), intensity(intensity), smoothness(smoothness) {}

		// Direction
		LightInstance(const Color3f& color, const v3_f32& direction, f32 intensity)
			: color(color), type(LightType_Direction), position(direction), intensity(intensity) {}
	};

	// Temp data
	static FrameList<SpriteInstance> sprite_instances;
	static FrameList<MeshInstance> mesh_instances;
	static FrameList<LightInstance> light_instances;

	void draw_scene(Scene* scene)
	{
		mesh_instances.reset();
		light_instances.reset();
		sprite_instances.reset();

		ECS* ecs = scene->ecs;
		CommandList cmd = graphics_commandlist_begin();

		graphics_image_clear(scene->offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Black(), 1.f, 0u, cmd);
		graphics_image_clear(scene->depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::Black(), 1.f, 0u, cmd);

		graphics_viewport_set(scene->offscreen, 0u, cmd);
		graphics_scissor_set(scene->offscreen, 0u, cmd);

		// Get lights
		{
			EntityView<LightComponent> lights(ecs);

			for (const LightComponent& l : lights) {

				Transform trans = ecs_entity_transform_get(ecs, l.entity);

				switch (l.light_type)
				{
				case LightType_Point:
					light_instances.emplace_back(l.color, trans.getWorldPosition(), l.range, l.intensity, l.smoothness);
					break;

				case LightType_Direction:
				{
					XMVECTOR direction = XMVectorSet(0.f, 0.f, 1.f, 1.f);

					direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(trans.getWorldRotation().get_dx()));

					light_instances.emplace_back(l.color, v3_f32(direction), l.intensity);
				}
				break;

				}

			}
		}

		// Draw cameras
		{
			EntityView<CameraComponent> cameras(ecs);

			for (CameraComponent& camera : cameras) {

				Transform camera_trans = ecs_entity_transform_get(ecs, camera.entity);

				CameraBuffer_GPU camera_data;
				{
					camera_data.projection_matrix = camera_projection_matrix(camera);
					camera_data.position = camera_trans.getWorldPosition().getVec4(0.f);
					camera_data.rotation = camera_trans.getWorldRotation();
					camera_data.view_matrix = camera_view_matrix(camera_data.position.get_vec3(), camera_data.rotation, camera);
					camera_data.view_projection_matrix = camera_data.view_matrix * camera_data.projection_matrix;
				}

				draw_sky(scene->offscreen, camera_data.view_matrix, camera_data.projection_matrix, cmd);

				graphics_buffer_update(gfx.cbuffer_camera, &camera_data, sizeof(CameraBuffer_GPU), 0u, cmd);

				// DRAW SPRITES
				{
					{
						EntityView<SpriteComponent> sprites(ecs);

						for (SpriteComponent& spr : sprites) {

							Transform trans = ecs_entity_transform_get(ecs, spr.entity);
							sprite_instances.emplace_back(trans.getWorldMatrix(), spr.texcoord, spr.texture.get(), spr.color, spr.layer);
						}
					}

					if (sprite_instances.size()) {

						// Sort sprites
						std::sort(sprite_instances.data(), sprite_instances.data() + sprite_instances.size(), [](const SpriteInstance& s0, const SpriteInstance& s1)
						{
							return s0.layer < s1.layer;
						});

						GPUBuffer* batch_buffer = get_batch_buffer(sizeof(SpriteData), cmd);
						SpriteData& data = *(SpriteData*)batch_data[cmd];

						// Prepare
						graphics_event_begin("Sprite_GeometryPass", cmd);

						graphics_topology_set(GraphicsTopology_Triangles, cmd);
						graphics_vertexbuffer_bind(batch_buffer, 0u, 0u, cmd);
						graphics_indexbuffer_bind(gfx.ibuffer_sprite, 0u, cmd);
						graphics_inputlayoutstate_bind(gfx.ils_sprite, cmd);
						graphics_sampler_bind(gfx.sampler_def_nearest, 0u, ShaderType_Pixel, cmd);
						graphics_shader_bind(gfx.vs_sprite, cmd);
						graphics_shader_bind(gfx.ps_sprite, cmd);
						graphics_blendstate_bind(gfx.bs_transparent, cmd);
						graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);

						GPUImage* att[2];
						att[0] = scene->offscreen;
						att[1] = scene->depthstencil;

						XMMATRIX matrix;
						XMVECTOR pos0, pos1, pos2, pos3;

						// Batch data, used to update the vertex buffer
						SpriteVertex* batchIt = data.data;

						// Used to draw
						u32 instanceCount;

						const SpriteInstance* it = sprite_instances.data();
						const SpriteInstance* end = it + sprite_instances.size();

						while (it != end) {

							// Compute the end ptr of the vertex data
							SpriteVertex* batchEnd;
							{
								size_t batchCount = batchIt - data.data + SPRITE_BATCH_COUNT * 4u;
								instanceCount = std::min(batchCount / 4u, size_t(end - it));
								batchEnd = batchIt + instanceCount * 4u;
							}

							// Fill batch buffer
							while (batchIt != batchEnd) {

								const SpriteInstance& spr = *it++;

								matrix = XMMatrixMultiply(spr.tm, camera_data.view_projection_matrix);

								pos0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
								pos1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
								pos2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
								pos3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

								pos0 = XMVector4Transform(pos0, matrix);
								pos1 = XMVector4Transform(pos1, matrix);
								pos2 = XMVector4Transform(pos2, matrix);
								pos3 = XMVector4Transform(pos3, matrix);

								*batchIt = { v4_f32(pos0), {spr.texcoord.x, spr.texcoord.y}, spr.color };
								++batchIt;

								*batchIt = { v4_f32(pos1), {spr.texcoord.z, spr.texcoord.y}, spr.color };
								++batchIt;

								*batchIt = { v4_f32(pos2), {spr.texcoord.x, spr.texcoord.w}, spr.color };
								++batchIt;

								*batchIt = { v4_f32(pos3), {spr.texcoord.z, spr.texcoord.w}, spr.color };
								++batchIt;
							}

							// Draw

							graphics_buffer_update(batch_buffer, data.data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

							graphics_renderpass_begin(gfx.renderpass_world, att, nullptr, 1.f, 0u, cmd);

							end = it;
							it -= instanceCount;

							const SpriteInstance* begin = it;
							const SpriteInstance* last = it;

							graphics_image_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);

							while (it != end) {

								if (it->image != last->image) {

									u32 spriteCount = u32(it - last);
									u32 startVertex = u32(last - begin) * 4u;

									graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

									if (it->image != last->image) {
										graphics_image_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);
									}

									last = it;
								}

								++it;
							}

							// Last draw call
							{
								u32 spriteCount = u32(it - last);
								u32 startVertex = u32(last - begin) * 4u;

								graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);
							}

							graphics_renderpass_end(cmd);

							end = sprite_instances.data() + sprite_instances.size();
							batchIt = data.data;
						}
					}
				}

				// DRAW MESHES
				{
					{
						EntityView<MeshComponent> meshes(ecs);

						for (MeshComponent& mesh : meshes) {

							Transform trans = ecs_entity_transform_get(ecs, mesh.entity);
							mesh_instances.emplace_back(trans.getWorldMatrix(), mesh.mesh.get(), &mesh.material);
						}
					}

					/* TODO LIST:
					- Undefined lights
					- Begin render pass once
					- Dinamic material and instance buffer
					*/
					if (mesh_instances.size()) {
						// Prepare state
						graphics_shader_bind(gfx.vs_mesh, cmd);
						graphics_shader_bind(gfx.ps_mesh, cmd);
						graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
						graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
						graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
						graphics_blendstate_bind(gfx.bs_mesh, cmd);
						graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

						// Bind resources
						GPUBuffer* instance_buffer = gfx.cbuffer_mesh_instance;
						GPUBuffer* material_buffer = gfx.cbuffer_material;
						GPUBuffer* light_instances_buffer = gfx.cbuffer_light_instances;

						graphics_constantbuffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
						graphics_constantbuffer_bind(gfx.cbuffer_camera, 1u, ShaderType_Vertex, cmd);
						graphics_constantbuffer_bind(material_buffer, 0u, ShaderType_Pixel, cmd);
						graphics_constantbuffer_bind(light_instances_buffer, 1u, ShaderType_Pixel, cmd);

						u32 light_count = std::min(LIGHT_COUNT, u32(light_instances.size()));

						// Send light data
						if (light_count) {
							LightData light_data[LIGHT_COUNT];

							XMMATRIX rotation_view_matrix;
							{
								XMFLOAT4X4 vm;
								XMStoreFloat4x4(&vm, camera_data.view_matrix);
								vm._41 = 0.f;
								vm._42 = 0.f;
								vm._43 = 0.f;
								vm._44 = 1.f;

								rotation_view_matrix = XMLoadFloat4x4(&vm);
							}

							foreach(i, light_count) {

								LightData& l0 = light_data[i];
								const LightInstance& l1 = light_instances[i];

								l0.type = l1.type;
								l0.color = l1.color;
								l0.intensity = l1.intensity;

								switch (l1.type)
								{
								case LightType_Point:
									l0.position = v3_f32(XMVector4Transform(l1.position.getDX(1.f), camera_data.view_matrix));
									l0.range = l1.range;
									l0.smoothness = l1.smoothness;
									break;

								case LightType_Direction:
									l0.position = v3_f32(XMVector3Transform(l1.position.getDX(1.f), rotation_view_matrix));
									break;
								}
							}

							graphics_buffer_update(light_instances_buffer, light_data, sizeof(LightData) * light_count, 0u, cmd);

						}

						foreach(i, mesh_instances.size()) {

							const MeshInstance& inst = mesh_instances[i];

							graphics_vertexbuffer_bind(inst.mesh->vbuffer, 0u, 0u, cmd);
							graphics_indexbuffer_bind(inst.mesh->ibuffer, 0u, cmd);

							// Update material data
							{
								MaterialData material_data;
								material_data.flags = 0u;

								GPUImage* diffuse_map = inst.material->diffuse_map.get();
								GPUImage* normal_map = inst.material->normal_map.get();
								GPUImage* specular_map = inst.material->specular_map.get();
								GPUImage* emissive_map = inst.material->emissive_map.get();

								graphics_image_bind(diffuse_map ? diffuse_map : gfx.image_white, 0u, ShaderType_Pixel, cmd);
								if (normal_map) { graphics_image_bind(normal_map, 1u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_NORMAL_MAPPING; }
								// TODO: I don't know why i need to do this. The shader shouldn't sample this texture without the flag...
								else graphics_image_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);

								if (specular_map) { graphics_image_bind(specular_map, 2u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_SPECULAR_MAPPING; }
								// TODO: I don't know why i need to do this. The shader shouldn't sample this texture without the flag...
								else graphics_image_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);

								if (emissive_map) { graphics_image_bind(emissive_map, 3u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_EMISSIVE_MAPPING; }
								// TODO: I don't know why i need to do this. The shader shouldn't sample this texture without the flag...
								else graphics_image_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

								material_data.diffuse_color = inst.material->diffuse_color;
								material_data.specular_color = inst.material->specular_color;
								material_data.emissive_color = inst.material->emissive_color;
								material_data.shininess = inst.material->shininess;

								graphics_buffer_update(material_buffer, &material_data, sizeof(MaterialData), 0u, cmd);
							}

							// Update instance data
							{
								MeshData mesh_data;
								mesh_data.model_view_matrix = inst.transform_matrix * camera_data.view_matrix;
								mesh_data.inv_model_view_matrix = XMMatrixInverse(nullptr, mesh_data.model_view_matrix);

								graphics_buffer_update(instance_buffer, &mesh_data, sizeof(MeshData), 0u, cmd);
							}

							// Begin renderpass
							GPUImage* att[] = { scene->offscreen, scene->depthstencil };
							graphics_renderpass_begin(gfx.renderpass_world, att, cmd);

							// Draw
							graphics_draw_indexed(u32(inst.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);

							graphics_renderpass_end(cmd);
						}
					}
				}
			}
		}

		gui_render(scene->gui, scene->offscreen, cmd);
	}

	void draw_sky(GPUImage* offscreen, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd)
	{
		graphics_depthstencilstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_inputlayoutstate_bind(gfx.ils_sky, cmd);
		graphics_rasterizerstate_unbind(cmd);

		graphics_vertexbuffer_bind(gfx.vbuffer_skybox, 0u, 0u, cmd);
		graphics_constantbuffer_bind(gfx.cbuffer_skybox, 0u, ShaderType_Vertex, cmd);
		graphics_image_bind(gfx.image_sky, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		XMFLOAT4X4 vm;
		XMStoreFloat4x4(&vm, view_matrix);
		vm._41 = 0.f;
		vm._42 = 0.f;
		vm._43 = 0.f;
		vm._44 = 1.f;

		view_matrix = XMLoadFloat4x4(&vm);
		view_matrix = view_matrix * projection_matrix;

		graphics_buffer_update(gfx.cbuffer_skybox, &view_matrix, sizeof(XMMATRIX), 0u, cmd);

		graphics_shader_bind(gfx.vs_sky, cmd);
		graphics_shader_bind(gfx.ps_sky, cmd);

		GPUImage* att[] = { offscreen };
		graphics_renderpass_begin(gfx.renderpass_off, att, 0, 1.f, 0u, cmd);
		graphics_draw(36u, 1u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);
	}

	XMMATRIX camera_view_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera)
	{
		return math_matrix_view(position, rotation);
	}

	XMMATRIX camera_projection_matrix(CameraComponent& camera)
	{
#ifndef SV_DIST
		if (camera.near >= camera.far) {
			SV_LOG_WARNING("Computing the projection matrix. The far must be grater than near");
		}

		switch (camera.projection_type)
		{
		case ProjectionType_Orthographic:
			break;

		case ProjectionType_Perspective:
			if (camera.near <= 0.f) {
				SV_LOG_WARNING("In perspective projection, near must be greater to 0");
			}
			break;
		}
#endif
		XMMATRIX projection_matrix;

		switch (camera.projection_type)
		{
		case ProjectionType_Orthographic:
			projection_matrix = XMMatrixOrthographicLH(camera.width, camera.height, camera.near, camera.far);
			break;

		case ProjectionType_Perspective:
			projection_matrix = XMMatrixPerspectiveLH(camera.width, camera.height, camera.near, camera.far);
			break;

		default:
			projection_matrix = XMMatrixIdentity();
			break;
		}

		return projection_matrix;
	}

	SV_INLINE static void text_draw_call(GPUImage* offscreen, GPUBuffer* buffer, TextData& data, u32 vertex_count, CommandList cmd)
	{
		graphics_buffer_update(buffer, data.vertices, vertex_count * sizeof(TextVertex), 0u, cmd);

		GPUImage* att[1];
		att[0] = offscreen;

		graphics_renderpass_begin(gfx.renderpass_off, att, nullptr, 1.f, 0u, cmd);
		graphics_draw_indexed(vertex_count / 4u * 6u, 1u, 0u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);
	}

	u32 draw_text(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd)
	{
		if (text == nullptr) return 0u;
		if (text_size == 0u) return 0u;

		// Select font
		Font& font = pFont ? *pFont : font_opensans;
		
		// Prepare
		graphics_rasterizerstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_topology_set(GraphicsTopology_Triangles, cmd);

		graphics_shader_bind(gfx.vs_text, cmd);
		graphics_shader_bind(gfx.ps_text, cmd);

		GPUBuffer* buffer = get_batch_buffer(sizeof(TextData), cmd);
		graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(gfx.ibuffer_text, 0u, cmd);
		graphics_image_bind(font.image, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		graphics_inputlayoutstate_bind(gfx.ils_text, cmd);

		TextData& data = *reinterpret_cast<TextData*>(batch_data[cmd]);

		// Text space transformation
		f32 xmult = font_size / aspect;
		f32 ymult = font_size;

		u32 vertex_count = 0u;
		f32 line_height = ymult;

		const char* it = text;
		const char* end = it + text_size;

		f32 xoff;
		f32 yoff = 0.f;

		u32 number_of_chars = 0u;

		while (it != end && max_lines--) {

			xoff = 0.f;
			yoff -= line_height;
			u32 line_begin_index = vertex_count;

			// TODO: add first char offset to xoff
			const char* line_end = process_text_line(it, text_size - (it - text), max_line_width / xmult, font);
			SV_ASSERT(line_end != it);

			if (vertex_count + (line_end - it) > TEXT_BATCH_COUNT) {

				text_draw_call(offscreen, buffer, data, vertex_count, cmd);
				vertex_count = 0u;
			}

			if (yoff + y > 1.f || yoff + line_height + y < -1.f) {
				it = line_end;
				continue;
			}

			// Fill batch
			while (it != line_end) {

				Glyph* g_ = font.get(*it);

				if (g_) {

					Glyph& g = *g_;

					f32 advance = g.advance * xmult;

					if (*it != ' ') {
						f32 xpos = xoff + g.xoff * xmult + x;
						f32 ypos = yoff + g.yoff * ymult + y;
						f32 width = g.w * xmult;
						f32 height = g.h * ymult;

						data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.w }, color };
						data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.w }, color };
						data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.y }, color };
						data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.y }, color };
					}

					xoff += advance;
				}

				++it;
			}

			// Alignment
			if (vertex_count != line_begin_index) {

				switch (alignment)
				{
				case TextAlignment_Center:
				{
					f32 line_width = (data.vertices[vertex_count - 1u].position.x - x);
					f32 displacement = (max_line_width - line_width) * 0.5f;

					for (u32 i = line_begin_index; i < vertex_count; ++i) {

						data.vertices[i].position.x += displacement;
					}
				} break;

				case TextAlignment_Right:
					break;
				case TextAlignment_Justified:
					SV_LOG_ERROR("TODO-> Justified text rendering");
					break;
				}
			}
		}

		number_of_chars = it - text;

		text_draw_call(offscreen, buffer, data, vertex_count, cmd);

		return number_of_chars;
	}

	void draw_text_area(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd)
	{
		if (text == nullptr) return;
		if (text_size == 0u) return;

		// Select font
		Font& font = pFont ? *pFont : font_opensans;

		f32 transformed_max_width = max_line_width / (font_size / aspect);

		// Select text to render
		if (bottom_top) {
			
			// Count lines
			u32 lines = 0u;

			const char* it = text;
			const char* end = text + text_size;

			while (it != end) {

				it = process_text_line(it, text_size - (it - text), transformed_max_width, font);
				++lines;
			}

			u32 begin_line_index = lines - std::min(max_lines, lines);
			begin_line_index -= std::min(begin_line_index, line_offset);

			it = text;
			lines = 0u;

			while (it != end && lines != begin_line_index) {

				it = process_text_line(it, text_size - (it - text), transformed_max_width, font);
				++lines;
			}

			text_size = text_size - (it - text);
			text = it;
		}
		else if (line_offset) {

			const char* it = text;
			const char* end = text + text_size;

			foreach (i, line_offset) {

				it = process_text_line(it, text_size - (it - text), max_line_width / font_size / aspect, font);
				if (it == end) return;
			}

			text_size = text_size - (it - text);
			text = it;
		}

		draw_text(offscreen, text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, &font, cmd);
	}

	XMMATRIX camera_view_projection_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera)
	{
		return camera_view_matrix(position, rotation, camera) * camera_projection_matrix(camera);
	}

}
