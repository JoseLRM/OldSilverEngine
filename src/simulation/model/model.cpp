#include "core.h"

#include "model_internal.h"

namespace sv {

	AssetType MeshAsset::assetType = nullptr;

	Result loadIDMesh(size_t ID, void* pObject)
	{
		Mesh* mesh = new(pObject) Mesh();

		switch (ID)
		{
		case MeshAssetType_Cube:
			mesh->applyCube();
			svCheck(mesh->createGPUBuffers());
			break;
		}

		return Result_Success;
	}

	Result destroyMesh(void* pObject)
	{
		Mesh* mesh = reinterpret_cast<Mesh*>(pObject);
		svCheck(mesh->clear());
		mesh->~Mesh();
		return Result_Success;
	}

	Result model_initialize()
	{
		// Create Mesh Asset
		{
			AssetRegisterTypeDesc desc;
			desc.name = "Mesh";
			desc.pExtensions = nullptr;
			desc.extensionsCount = 0u;
			desc.loadFileFn = nullptr;
			desc.loadIDFn = loadIDMesh;
			desc.createFn = nullptr;
			desc.destroyFn = destroyMesh;
			desc.reloadFileFn = nullptr;
			desc.serializeFn = nullptr;
			desc.isUnusedFn = nullptr;
			desc.assetSize = sizeof(Mesh*);
			desc.unusedLifeTime = 7.f;			

			svCheck(asset_register_type(&desc, &MeshAsset::assetType));
		}

		return Result_Success;
	}

	Result model_close()
	{
		return Result_Success;
	}

}