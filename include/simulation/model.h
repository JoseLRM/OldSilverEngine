#pragma once

#include "platform/graphics.h"

namespace sv {

	typedef u32 MeshIndex;

	struct MeshVertex {
		vec3f position;
		vec3f normal;
	};

	struct Mesh {

		std::vector<vec3f> positions;
		std::vector<vec3f> normals;

		std::vector<MeshIndex> indices;

		GPUBuffer* vertexBuffer = nullptr;
		GPUBuffer* indexBuffer = nullptr;

		void applyPlane(const XMMATRIX& transform = XMMatrixIdentity());
		void applyCube(const XMMATRIX& transform = XMMatrixIdentity());
		void applySphere(const XMMATRIX& transform = XMMatrixIdentity());

		void optimize();
		void recalculateNormals();
		Result createGPUBuffers(ResourceUsage usage = ResourceUsage_Static);
		Result updateGPUBuffers(CommandList cmd);

		Result clear();

	};

	enum MeshAssetType : u32 {
		MeshAssetType_Cube,
		MeshAssetType_None = u32_max,
	};

	struct MeshAsset : public Asset {
		static AssetType assetType;
		inline Mesh* get() const noexcept { return reinterpret_cast<Mesh*>(m_Ref.get()); }
		inline Mesh* operator->() const noexcept { return get(); }
		inline Result loadFromID(MeshAssetType type = MeshAssetType_None) { return Asset::loadFromID(assetType, type); }
	};

}