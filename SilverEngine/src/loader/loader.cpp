#include "core.h"

#include "loader_internal.h"

#include "external/assimp/Importer.hpp"
#include "external/assimp/postprocess.h"
#include "external/assimp/scene.h"

namespace sv {

	SerializationResult loader_model_import(const char* externalFilePath, Model& model)
	{
		Assimp::Importer importer;

		std::string filePath = externalFilePath;
#ifdef SV_SRC_PATH
		filePath = SV_SRC_PATH + filePath;
#endif

		const aiScene const* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes);
		if (scene == nullptr || !scene->HasMeshes()) return SerializationResult_InvalidFormat;

		// Create Materials
		model.materials.resize(scene->mNumMaterials);
		for (ui32 i = 0; i < scene->mNumMaterials; ++i) {

			Material& mat = model.materials[i];
			aiMaterial& aiMat = *scene->mMaterials[i];

			// TODO:
			MaterialData matData;
			matData.diffuseColor = { 0.f, 1.f, 0.f, 1.f };

			if (!mat.CreateBuffers(matData)) {
				return SerializationResult_Unknown;
			}

		}

		// Create Meshes
		// TEMP:
		model.nodes.resize(1u);
		{
			auto& mesh = model.nodes[0];
			auto& aiMesh = *scene->mMeshes[0];
			mesh.mesh.SetVertexCount(aiMesh.mNumVertices);
			mesh.mesh.SetIndexCount(aiMesh.mNumFaces * 3u);

			mesh.pMaterial = &model.materials[aiMesh.mMaterialIndex];
			mesh.transformMatrix = XMMatrixIdentity();

			// Vertices
			for (ui32 i = 0; i < aiMesh.mNumVertices; ++i) {
				aiMesh.mVertices[i].y *= -1.f;
			}
			mesh.mesh.SetPositions(aiMesh.mVertices, 0u, aiMesh.mNumVertices);

			mesh.mesh.SetNormals(aiMesh.mNormals, 0u, aiMesh.mNumVertices);
			mesh.mesh.SetTexCoords(aiMesh.mTextureCoords[0u], 0u, aiMesh.mNumVertices);

			// Indices
			for (ui32 i = 0; i < aiMesh.mNumFaces; ++i) {
				auto& face = aiMesh.mFaces[i];
				SV_ASSERT(face.mNumIndices == 3u);
				mesh.mesh.SetIndices(face.mIndices, i * 3u, 3u);
			}

			if (!mesh.mesh.CreateBuffers()) {
				return SerializationResult_Unknown;
			}
		}

		

		return SerializationResult_Success;
	}

}