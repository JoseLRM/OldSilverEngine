#pragma once

#include "rendering/material_system.h"
#include "simulation/model.h"

namespace sv {

	struct GBuffer {

		~GBuffer();

		GPUImage* diffuse = nullptr;
		GPUImage* normal = nullptr;

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
		Material* pMaterial;

	};

	struct MeshRendererDrawDesc {

		const MeshInstance*		pInstances;
		ui32					count;
		GPUImage*				renderTarget;
		GPUImage*				depthStencil;
		GBuffer*				pGBuffer;
		CameraBuffer*			cameraBuffer;

	};

	void mesh_renderer_draw(const MeshRendererDrawDesc* desc, CommandList cmd);

}