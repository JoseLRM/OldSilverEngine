#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

using namespace sv;

void load_model(Scene* scene, const char* filepath, f32 scale = f32_max)
{
	ModelInfo info;
	if (result_fail(model_load(filepath, info))) {
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

Result command_hello(const char** args, u32 argc)
{
	console_notify("MIAU", "Hola que tal");
	return Result_Success;
}

Result app_validate_scene(const char* name)
{
	const char* valid_names[] = {
			"Sponza", "Goblin", "Dragon", "Test"
	};

	foreach(i, sizeof(valid_names) / sizeof(const char*)) {

		if (strcmp(name, valid_names[i]) == 0) {

			return Result_Success;
		}
	}

	return Result_NotFound;
}

Result app_init_scene(Scene* scene, ArchiveI* archive)
{
	Entity cam = create_entity(scene, SV_ENTITY_NULL, "Camera");
	engine.scene->main_camera = cam;
	CameraComponent* camera = add_component<CameraComponent>(scene, cam);
	camera->far = 10000.f;
	camera->near = 0.2f;
	camera->width = 0.5f;
	camera->height = 0.5f;
	camera->projection_type = ProjectionType_Perspective;

	LightComponent* light = add_component<LightComponent>(scene, create_entity(scene, SV_ENTITY_NULL, "Light"));
	Transform t = get_entity_transform(scene, light->entity);
	light->light_type = LightType_Point;
	t.setPosition({ 0.f, 0.f, -2.f });
	t.setEulerRotation({ PI * 0.4f, 0.f, 0.f });

	create_entity(scene, light->entity);

	if (strcmp(scene->name.c_str(), "Sponza") == 0) {

		load_model(scene, "assets/Sponza/sponza.obj");
	}
	if (strcmp(scene->name.c_str(), "Goblin") == 0) {

		load_model(scene, "assets/gobber/GoblinX.obj");
	}
	if (strcmp(scene->name.c_str(), "Dragon") == 0) {

		load_model(scene, "assets/dragon.obj");
	}

	return Result_Success;
}

Result app_close_scene(Scene* scene)
{

	return Result_Success;
}

Result init()
{
	set_active_scene("Goblin");

	register_command("hola", command_hello);

	return Result_Success;
}

void update()
{
	Transform t0 = get_entity_transform(engine.scene, 1u);
	Transform t1 = get_entity_transform(engine.scene, 2u);
	t1.setRotation(t0.getWorldRotation());
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	graphics_present(engine.window, engine.scene->offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result close()
{

	return Result_Success;
}

int main()
{
	InitializationDesc desc;
	desc.minThreadsCount = 2u;
	desc.callbacks.initialize = init;
	desc.callbacks.update = update;
	desc.callbacks.render = render;
	desc.callbacks.close = close;

	desc.callbacks.validate_scene = app_validate_scene;
	desc.callbacks.initialize_scene = app_init_scene;
	desc.callbacks.close_scene = app_close_scene;
	desc.callbacks.serialize_scene = nullptr;

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
