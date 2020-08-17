#pragma once

#include "scene.h"
#include "renderer.h"

namespace sv {

	enum SerializationResult {
		SerializationResult_Success,
		SerializationResult_Unknown,
		SerializationResult_NotFound,
		SerializationResult_InvalidFormat,
	};

	// Files

	enum FileState {
		FileState_None,
		FileState_OpenWrite,
		FileState_OpenRead,
	};

	struct File {
		size_t	size;
		size_t	pos;
		FILE*	stream;
	};

	SerializationResult loader_file_open_read(const char* filePath, File& file);
	SerializationResult loader_file_open_write(const char* filePath, bool append, File& file);

	void loader_file_see(size_t pos, File& file);
	void loader_file_read(void* dst, size_t size, File& file);
	void loader_file_write(const void* src, size_t size, File& file);

	constexpr FileState loader_file_state_get(File& file);

	void loader_file_close(File& file);

	// 3D Model

	struct MaterialNode {
		vec4 diffuseColor;
	};

	struct MeshNode {
		Mesh		mesh;
		Material*	pMaterial;
		XMMATRIX	transformMatrix;
	};

	struct Model {
		std::vector<MeshNode>	nodes;
		std::vector<Material>	materials;
		std::string				filePath;
	};

	SerializationResult loader_model_import(const char* externalFilePath, Model& model);
	SerializationResult loader_model_serialize(const char* filePath, Model& model);
	SerializationResult loader_model_deserialize(const char* filePath, Model& model);

	Entity				loader_model_create_entities(Model& model);

}