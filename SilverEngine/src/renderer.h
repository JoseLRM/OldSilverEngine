#pragma once

#include "core.h"

#include "graphics.h"
#include "renderer/objects/Mesh.h"
#include "renderer/objects/Material.h"
#include "renderer/objects/TextureAtlas.h"

namespace sv {
	
	enum CameraType : ui32 {
		CameraType_Clip,
		CameraType_Orthographic,
		CameraType_Perspective,
	};

	enum LightType : ui32 {
		LightType_None,
		LightType_Point,
		LightType_Spot,
		LightType_Directional,
	};

	struct CameraProjection {
		CameraType cameraType = CameraType_Orthographic;
		float width = 1.f;
		float height = 1.f;
		float near = -10000.0f;
		float far = 10000.f;
	};

	struct Offscreen {
		GPUImage renderTarget;
		GPUImage depthStencil;
		inline Viewport GetViewport() const noexcept { return graphics_image_get_viewport(renderTarget); }
		inline Scissor GetScissor() const noexcept { return graphics_image_get_scissor(renderTarget); }
		inline ui32 GetWidth() const noexcept { return graphics_image_get_width(renderTarget); }
		inline ui32 GetHeight() const noexcept { return graphics_image_get_height(renderTarget); }
	};

	struct SpriteInstance {
		XMMATRIX tm;
		vec4 texCoord;
		Texture* pTexture;
		Color color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, Sprite sprite, sv::Color color) : tm(m), texCoord(sprite.texCoord), pTexture(sprite.texture.Get()), color(color) {}
	};

	struct SpriteRenderingDesc {
		const SpriteInstance*	pInstances;
		ui32					count;
		const XMMATRIX*			pViewProjectionMatrix;
		GPUImage*				pRenderTarget;
	};

	struct MeshInstance {
		XMMATRIX	modelViewMatrix;
		Mesh*		pMesh;
		Material*	pMaterial;

		MeshInstance() = default;
		MeshInstance(XMMATRIX mvm, Mesh* mesh, Material* mat) : modelViewMatrix(mvm), pMesh(mesh), pMaterial(mat) {}
	};

	struct alignas(16) LightInstance {
		LightType	lightType;
		vec3		position;
		vec3		direction;
		float		intensity;
		float		range;
		Color3f		color;
		float		smoothness;
		vec3		padding;

		LightInstance() = default;
		LightInstance(LightType type, const vec3& position, const vec3& direction, float intensity, float range, float smoothness, const Color3f& color)
			: lightType(type), position(position), direction(direction), intensity(intensity), range(range), smoothness(smoothness), color(color) {}
	};

	struct ForwardRenderingDesc {
		const MeshInstance*		pInstances;
		ui32*					pIndices;
		ui32					count;
		bool					transparent;
		const XMMATRIX*			pViewMatrix;
		const XMMATRIX*			pProjectionMatrix;
		GPUImage*				pRenderTarget;
		GPUImage*				pDepthStencil;
		const LightInstance*	lights;
		ui32					lightCount;
	};

	// Renderer initialization

	struct InitializationRendererDesc {
		bool				presentOffscreen;
		ui32				resolutionWidth;
		ui32				resolutionHeight;
	};

	// Camera Projection

	XMMATRIX	renderer_projection_matrix(const CameraProjection& projection);
	float		renderer_projection_aspect_get(const CameraProjection& projection);
	void		renderer_projection_aspect_set(CameraProjection& projection, float aspect);
	vec2		renderer_projection_position(const CameraProjection& projection, const vec2& point); // The point must be in range { -0.5 - 0.5 }
	float		renderer_projection_zoom_get(const CameraProjection& projection);
	void		renderer_projection_zoom_set(CameraProjection& projection, float zoom);

	// Offscreen

	Result renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen);
	Result renderer_offscreen_destroy(Offscreen& offscreen);

	Offscreen&	renderer_offscreen_get();
	void		renderer_offscreen_set_present(bool enable);

	// MainOffscreen resolution

	void	renderer_resolution_set(ui32 width, ui32 height);
	uvec2	renderer_resolution_get() noexcept;
	ui32	renderer_resolution_get_width() noexcept;
	ui32	renderer_resolution_get_height() noexcept;
	float	renderer_resolution_get_aspect() noexcept;

	// Mesh rendering

	void renderer_mesh_forward_rendering(const ForwardRenderingDesc* desc, CommandList cmd);

	// Sprite rendering

	void renderer_sprite_rendering(const SpriteRenderingDesc* desc, CommandList cmd);

	// Postprocessing


}