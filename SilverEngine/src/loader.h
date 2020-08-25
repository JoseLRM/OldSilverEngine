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