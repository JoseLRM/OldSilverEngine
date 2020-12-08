#pragma once

#include "rendering/material_system.h"
#include "simulation/model.h"

namespace sv {

	struct GBuffer {

		~GBuffer();

		GPUImage* diffuse = nullptr;
		GPUImage* normal = nullptr;
		GPUImage* depthStencil = nullptr;

		inline vec2u resolution() const noexcept { return (diffuse == nullptr) ? vec2u{ 0u, 0u } : vec2u{ graphics_image_get_width(diffuse), graphics_image_get_height(diffuse) }; }
		inline vec2u resolutionWidth() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_width(diffuse); }
		inline vec2u resolutionHeight() const noexcept { return (diffuse == nullptr) ? 0u : graphics_image_get_height(diffuse); }

		Result create(ui32 width, ui32 height);
		Result resize(ui32 width, ui32 height);
		Result destroy();

	};

	struct MeshInstance {

		XMMATRIX tm;
		Mesh* pMesh;
		Material* material;
		float toCameraDistance;

		MeshInstance() = default;
		MeshInstance(const XMMATRIX& tm, Mesh* pMesh, Material* material, float toCamera) : tm(tm), pMesh(pMesh), material(material), toCameraDistance(toCamera) {}

	};

	template<typename T>
	class FrameList;

	struct MeshInstanceGroup {

		FrameList<MeshInstance>*	pInstances;
		RasterizerCullMode			cullMode;
		bool						transparent;
		
	};

	enum LightType : ui32 {
		LightType_Point,
		LightType_Direction
	};

	struct LightInstance {

		LightType type;

		union {
			struct {
				vec3f position;
				float radius;
				float intensity;
				float smoothness;
			} point;

			struct {
				vec3f direction;
				float intensity;
			} direction;
		};

		LightInstance() = default;
		LightInstance(const vec3f& pos, float radius, float intensity, float smoothness) : type(LightType_Point), point({ pos, radius, intensity, smoothness }) {}
	};

	struct MeshRenderer {

		static void drawMeshes(GPUImage* renderTarget, GBuffer& gBuffer, CameraBuffer& cameraBuffer, FrameList<MeshInstance>& meshes, FrameList<LightInstance>& lights, bool optimize, CommandList cmd);

	};

}