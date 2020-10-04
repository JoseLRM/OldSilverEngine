#pragma once

#include "scene.h"
#include "renderer.h"

namespace sv {

	// 3D Model

	struct MaterialNode {
		vec4f diffuseColor;
	};

	struct MeshNode {
		//Mesh		mesh;
		Material*	pMaterial;
		XMMATRIX	transformMatrix;
	};

	struct Model {
		std::vector<MeshNode>	nodes;
		std::vector<Material>	materials;
		std::string				filePath;
	};

	Result loader_model_import(const char* externalFilePath, Model& model);
	Result loader_model_serialize(const char* filePath, Model& model);
	Result loader_model_deserialize(const char* filePath, Model& model);

	Entity	loader_model_create_entities(Model& model);

}