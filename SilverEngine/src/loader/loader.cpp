#include "core.h"

#include "loader_internal.h"

#include "external/assimp/Importer.hpp"
#include "external/assimp/postprocess.h"
#include "external/assimp/scene.h"

namespace sv {

	FILE* loader_file_open(const char* filePath, const char* mode)
	{
		FILE* res = nullptr;

#ifdef SV_SRC_PATH
		std::string filePathStr;
		filePathStr += SV_SRC_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		//res = fopen(filePath, mode);
		return res;
	}

	SerializationResult loader_file_open_read(const char* filePath, File& file)
	{
		loader_file_close(file);
		
		file.stream = loader_file_open(filePath, "r");
		// TODO:
		// Capture errors
		// Get size
		// Set pos

		return SerializationResult_Success;
	}

	SerializationResult loader_file_open_write(const char* filePath, bool append, File& file)
	{
		loader_file_close(file);

		file.stream = loader_file_open(filePath, append ? "a" : "w");

		return SerializationResult_Success;
	}

	void loader_file_see(size_t pos, File& file)
	{
	}

	void loader_file_read(void* dst, size_t size, File& file)
	{
		SV_ASSERT(loader_file_state_get(file) == FileState_OpenRead);
		fread(dst, 1u, size, file.stream);
	}

	void loader_file_write(const void* src, size_t size, File& file)
	{
		SV_ASSERT(loader_file_state_get(file) == FileState_OpenWrite);
		fwrite(src, 1u, size, file.stream);
	}

	constexpr FileState loader_file_state_get(File& file)
	{
		//if (file.inputStream.is_open()) return FileState_OpenRead;
		//if (file.outputStream.is_open()) return FileState_OpenWrite;
		return FileState_None;

	}

	void loader_file_close(File& file)
	{
		fclose(file.stream);
		svZeroMemory(&file, sizeof(File));
	}

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