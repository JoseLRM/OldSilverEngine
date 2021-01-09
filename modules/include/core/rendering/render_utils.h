#pragma once

#include "core/rendering/material_system.h"

namespace sv {

	// TODO: Move to SceneRenderer
	static constexpr u32 RENDERLAYER_COUNT = 16u;

	// TODO: Copy & Move operator
	struct GBuffer {

		static constexpr Format FORMAT_OFFSCREEN = Format_R16G16B16A16_FLOAT;
		static constexpr Format FORMAT_DIFFUSE = Format_R16G16B16A16_FLOAT;
		static constexpr Format FORMAT_NORMAL = Format_R16G16B16A16_FLOAT;
		static constexpr Format FORMAT_DEPTHSTENCIL = Format_D24_UNORM_S8_UINT;

		~GBuffer();

		GPUImage* offscreen		= nullptr;
		GPUImage* diffuse		= nullptr;
		GPUImage* normal		= nullptr;
		GPUImage* depthStencil	= nullptr;

		//TODO: setResolution();

		SV_INLINE v2_u32 getResolution() const noexcept { return (diffuse == nullptr) ? v2_u32{ 0u, 0u } : v2_u32{ graphics_image_get_width(diffuse), graphics_image_get_height(diffuse) }; }
		SV_INLINE v2_u32 getResolutionWidth() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_width(diffuse); }
		SV_INLINE v2_u32 getResolutionHeight() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_height(diffuse); }

		Viewport	getViewport() const noexcept;
		Scissor		getScissor() const noexcept;

		Result create(u32 width, u32 height);
		Result resize(u32 width, u32 height);
		Result destroy();

	};

	enum ProjectionType : u32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective,
	};

	struct CameraProjection {

		f32 width		= 1.f;
		f32 height		= 1.f;
		f32 near		= -1000.f;
		f32 far			= 1000.f;

		ProjectionType	projectionType		= ProjectionType_Orthographic;
		XMMATRIX		projectionMatrix	= XMMatrixIdentity();

		void adjust(u32 width, u32 height) noexcept;
		void adjust(float aspect) noexcept;

		f32		getProjectionLength() const noexcept;
		void	setProjectionLength(float length) noexcept;

		void updateMatrix();

	};

	enum LightType : u32 {
		LightType_Point,
		LightType_Direction
	};

	struct CameraData {

		v3_f32				position;
		v4_f32				rotation;
		CameraProjection*	pProjection;
		CameraBuffer*		pCameraBuffer;
		GBuffer*			pGBuffer;

	};

	struct LightInstance {

		LightType type;
		Color3f color;
		float intensity;

		union {
			struct {
				v3_f32 position;
				float range;
				float smoothness;
			} point;

			struct {
				v3_f32 direction;
			} direction;
		};

		LightInstance() = default;
		LightInstance(const LightInstance& other) = default;
		LightInstance(const v3_f32& pos, const Color3f& color, float range, float intensity, float smoothness) : type(LightType_Point), color(color), intensity(intensity), point({ pos, range, smoothness }) {}
	};

	// AUXILIAR IMAGES

	u64	auximg_id(u32 width, u32 height, Format format, GPUImageTypeFlags imageType);

	// Return the image and is index
	std::pair<GPUImage*, size_t>	auximg_push(u64 id, GPUImageLayout layout, CommandList cmd);
	void							auximg_pop(size_t index, GPUImageLayout layout, CommandList cmd);

}