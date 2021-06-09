#pragma once

#include "platform/graphics.h"
#include "utils/string.h"

namespace sv {

    constexpr u32 MESH_NAME_SIZE = 100u;
    constexpr u32 MATERIAL_NAME_SIZE = 100u;

    typedef u32 MeshIndex;
    
    struct MeshVertex {
		v3_f32 position;
		v3_f32 normal;
		v4_f32 tangents;
		v2_f32 texcoord;
    };

    struct Mesh {

		List<v3_f32> positions;
		List<v3_f32> normals;
		List<v4_f32> tangents;
		List<v2_f32> texcoords;

		List<MeshIndex> indices;

		GPUBuffer* vbuffer = nullptr;
		GPUBuffer* ibuffer = nullptr;

		// Info about the importation of the 3D Model

		XMMATRIX model_transform_matrix = XMMatrixIdentity();
		String model_material_filepath;

    };

    struct Material {

		// Pipeline settings
		bool transparent = false;
		RasterizerCullMode culling = RasterizerCullMode_Back;
	
		// Values
		Color ambient_color;
		Color diffuse_color;
		Color specular_color;
		Color emissive_color;
		f32   shininess = 0.1f;

		// Textures
	
		TextureAsset diffuse_map;
		TextureAsset normal_map;
		TextureAsset specular_map;
		TextureAsset emissive_map;
    };

	typedef u32 TerrainIndex;

	struct TerrainVertex {
		f32 height;
		v3_f32 normal;
		v2_f32 texcoord;
		Color color;
	};

	struct Terrain {

		GPUBuffer* vbuffer = NULL;
		GPUBuffer* ibuffer = NULL;

		// Thats the resolution of the height samples
		u32 width = 0u;
		u32 height = 0u;
		
		List<f32> heights;
		List<v3_f32> normals;
		List<v2_f32> texcoords;
		List<Color>  colors;

		List<TerrainIndex> indices;
		
	};

	struct TerrainMaterial {

		TextureAsset albedo_map;
		
	};

    SV_DEFINE_ASSET(MeshAsset, Mesh);
    SV_DEFINE_ASSET(MaterialAsset, Material);

    SV_API void mesh_apply_plane(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
    SV_API void mesh_apply_cube(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
    SV_API void mesh_apply_sphere(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());

    SV_API void mesh_set_scale(Mesh& mesh, f32 scale, bool center = false);
    SV_API void mesh_optimize(Mesh& mesh);
    SV_API void mesh_calculate_normals(Mesh& mesh);
	SV_API void mesh_calculate_tangents(Mesh& mesh);
	SV_API void mesh_recalculate_normals_and_tangents(Mesh& mesh);

    SV_API bool mesh_create_buffers(Mesh& mesh, ResourceUsage usage = ResourceUsage_Static);
    SV_API bool mesh_update_buffers(Mesh& mesh, CommandList cmd);
	SV_API void mesh_destroy_buffers(Mesh& mesh);
    SV_API void mesh_clear(Mesh& mesh);

	
	SV_API void terrain_apply_heightmap_u8(Terrain& terrain, const u8* heights, u32 width, u32 height, u32 stride);
	SV_API bool terrain_apply_heightmap_image(Terrain& terrain, const char* filepath);
	
	SV_API bool terrain_create_buffers(Terrain& terrain, ResourceUsage usage = ResourceUsage_Static);
	SV_API bool terrain_update_buffers(Terrain& terrain, CommandList cmd);
	SV_API void terrain_destroy_buffers(Terrain& terrain);
	SV_API void terrain_clear(Terrain& terrain);

    // Model loading

    struct MeshInfo {

		String name;

		List<v3_f32> positions;
		List<v3_f32> normals;
		List<v2_f32> texcoords;

		List<MeshIndex> indices;

		XMMATRIX transform_matrix;
		u32 material_index = u32_max;

    };

    struct MaterialInfo {

		String name;
	
		// Pipeline settings
		bool transparent = false;
		RasterizerCullMode culling = RasterizerCullMode_Back;

		// Values
	
		Color ambient_color;
		Color diffuse_color;
		Color specular_color;
		Color emissive_color;
		f32 shininess;

		// Textures
	
		String diffuse_map_path;
		String normal_map_path;
		String specular_map_path;
		String emissive_map_path;
    };

    struct ModelInfo {
		char folderpath[FILEPATH_SIZE + 1u];
		List<MeshInfo> meshes;
		List<MaterialInfo> materials;
    };

    // Load external model format
    SV_API bool load_model(const char* filepath, ModelInfo& model_info);

    // Create asset files for the engine
    SV_API bool import_model(const char* filepath, const ModelInfo& model_info);

    SV_API bool load_mesh(const char* filepath, Mesh& mesh);
    SV_API bool load_material(const char* filepath, Material& material);

}
