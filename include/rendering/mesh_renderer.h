#pragma once

#include "rendering/material_system.h"
#include "simulation/model.h"
#include "render_utils.h"

namespace sv {

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
		
		MeshInstanceGroup() = default;
		MeshInstanceGroup(FrameList<MeshInstance>* pInstances, RasterizerCullMode cullMode, bool transparent) :
			pInstances(pInstances), cullMode(cullMode), transparent(transparent) {}

	};

	struct MeshRenderer {

		static void drawMeshes(GPUImage* renderTarget, GBuffer& gBuffer, CameraBuffer& cameraBuffer, FrameList<MeshInstanceGroup>& meshes, FrameList<LightInstance>& lights, bool optimize, bool depthTest, CommandList cmd);

	};

}