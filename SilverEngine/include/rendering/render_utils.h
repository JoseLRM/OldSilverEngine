#pragma once

#include "platform/graphics.h"

namespace sv {

	static constexpr Format OFFSCREEN_FORMAT = Format_R8G8B8A8_SRGB;
	static constexpr u32 RENDERLAYER_COUNT = 16u;

	struct GBuffer {

		static constexpr Format FORMAT_DIFFUSE = Format_R8G8B8A8_SRGB;
		static constexpr Format FORMAT_NORMAL = Format_R16G16B16A16_FLOAT;
		static constexpr Format FORMAT_DEPTHSTENCIL = Format_D24_UNORM_S8_UINT;

		~GBuffer();

		GPUImage* diffuse = nullptr;
		GPUImage* normal = nullptr;
		GPUImage* depthStencil = nullptr;

		inline vec2u resolution() const noexcept { return (diffuse == nullptr) ? vec2u{ 0u, 0u } : vec2u{ graphics_image_get_width(diffuse), graphics_image_get_height(diffuse) }; }
		inline vec2u resolutionWidth() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_width(diffuse); }
		inline vec2u resolutionHeight() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_height(diffuse); }

		Result create(u32 width, u32 height);
		Result resize(u32 width, u32 height);
		Result destroy();

	};

	enum LightType : u32 {
		LightType_Point,
		LightType_Direction
	};

	struct LightInstance {

		LightType type;
		Color3f color;
		float intensity;

		union {
			struct {
				vec3f position;
				float range;
				float smoothness;
			} point;

			struct {
				vec3f direction;
			} direction;
		};

		LightInstance() = default;
		LightInstance(const LightInstance& other) = default;
		LightInstance(const vec3f& pos, const Color3f& color, float range, float intensity, float smoothness) : type(LightType_Point), color(color), intensity(intensity), point({ pos, range, smoothness }) {}
	};

	// AUXILIAR IMAGES

	u64	auximg_id(u32 width, u32 height, Format format, GPUImageTypeFlags imageType);

	// Return the image and is index
	std::pair<GPUImage*, size_t>	auximg_push(u64 id, GPUImageLayout layout, CommandList cmd);
	void							auximg_pop(size_t index, GPUImageLayout layout, CommandList cmd);

}