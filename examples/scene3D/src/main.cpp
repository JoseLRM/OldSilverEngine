#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

using namespace sv;

void load_model(Scene* scene, const char* filepath, f32 scale = f32_max)
{
	ModelInfo info;
	if (result_fail(load_model(filepath, info))) {
		SV_LOG_ERROR("Can't load the model");
	}
	else {

		for (u32 i = 0u; i < info.meshes.size(); ++i) {

			MeshInfo& m = info.meshes[i];

			Entity e = create_entity(scene);

			MeshComponent& comp = *add_component<MeshComponent>(scene, e);

			create_asset(comp.mesh, "Mesh");
			Mesh& mesh = *comp.mesh.get();

			mesh.positions = std::move(m.positions);
			mesh.normals = std::move(m.normals);
			mesh.tangents = std::move(m.tangents);
			mesh.bitangents = std::move(m.bitangents);
			mesh.texcoords = std::move(m.texcoords);
			mesh.indices = std::move(m.indices);

			if (scale != f32_max)
				mesh_set_scale(mesh, scale, true);
			mesh_create_buffers(mesh);

			if (m.name.size()) {

				set_entity_name(scene, e, m.name.c_str());
			}

			MaterialInfo& mat = info.materials[m.material_index];

			comp.material.diffuse_color = mat.diffuse_color;
			comp.material.specular_color = mat.specular_color;
			comp.material.emissive_color = mat.emissive_color;
			comp.material.emissive_color = Color3f::Blue();
			comp.material.shininess = std::max(mat.shininess, 0.1f);
			comp.material.diffuse_map = mat.diffuse_map;
			comp.material.normal_map = mat.normal_map;
			comp.material.specular_map = mat.specular_map;
			comp.material.emissive_map = mat.emissive_map;

			Transform trans = get_entity_transform(scene, e);
			trans.setMatrix(m.transform_matrix);
		}
	}
}

Result app_validate_scene(const char* name)
{
	const char* valid_names[] = {
			"Test"
	};

	foreach(i, sizeof(valid_names) / sizeof(const char*)) {

		if (strcmp(name, valid_names[i]) == 0) {

			return Result_Success;
		}
	}

	return Result_NotFound;
}

Result app_init_scene(Scene* scene, Archive* archive)
{
	if (archive == nullptr) {
	
		Entity cam = create_entity(scene, SV_ENTITY_NULL, "Camera");
		scene->main_camera = cam;
		CameraComponent* camera = add_component<CameraComponent>(scene, cam);
		camera->far = 10000.f;
		camera->near = 0.2f;
		camera->width = 0.5f;
		camera->height = 0.5f;
		camera->projection_type = ProjectionType_Perspective;
	}

	return Result_Success;
}

Result app_close_scene(Scene* scene)
{

	return Result_Success;
}

Result init()
{
	//set_active_scene("Test");

	return Result_Success;
}

void update()
{
}

void update_scene(Scene* scene)
{
}

Result close()
{

	return Result_Success;
}

std::string get_scene_filepath(const char* name)
{
	std::stringstream ss;
	ss << "scenes/";
	ss << name << ".scene";
	return ss.str();
}

int main()
{
	InitializationDesc desc;
	desc.minThreadsCount = 2u;
	desc.callbacks.initialize = init;
	desc.callbacks.update = update;
	desc.callbacks.close = close;

	desc.callbacks.validate_scene = app_validate_scene;
	desc.callbacks.initialize_scene = app_init_scene;
	desc.callbacks.update_scene = update_scene;
	desc.callbacks.close_scene = app_close_scene;
	desc.callbacks.serialize_scene = nullptr;
	desc.callbacks.get_scene_filepath = get_scene_filepath;

	desc.windowDesc.bounds = { 0u, 0u, 1080u, 720u };
	desc.windowDesc.flags = WindowFlag_Default;
	desc.windowDesc.iconFilePath = nullptr;
	//desc.windowDesc.state = WindowState_Windowed;
	desc.windowDesc.state = WindowState_Fullscreen;
	desc.windowDesc.title = L"Test";

	if (result_fail(engine_initialize(&desc))) {
		printf("Can't init");
	}
	else {

		while (result_okay(engine_loop()));

		engine_close();
	}

	system_pause();

}
