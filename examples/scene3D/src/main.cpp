#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

using namespace sv;

GPUImage* offscreen = nullptr;
GPUImage* zbuffer = nullptr;

bool update_camera = false;
CameraComponent* camera;
PointLightComponent* light;
ECS* ecs;
GUI* gui;

GuiWindow* window;
Editor_ECS* editor_ecs;

void load_model(ECS* ecs, const char* filepath, f32 scale = f32_max)
{
	ModelInfo info;
	if (result_fail(model_load(filepath, info))) {
		SV_LOG_ERROR("Can't load the model");
	}
	else {

		for (u32 i = 0u; i < info.meshes.size(); ++i) {
		    
			MeshInfo& m = info.meshes[i];

			Entity e = ecs_entity_create(ecs);

			MeshComponent& comp = *ecs_component_add<MeshComponent>(ecs, e);
			
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

				ecs_component_add<NameComponent>(ecs, e)->name = m.name;
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

			Transform trans = ecs_entity_transform_get(ecs, e);
			//trans.setScale({ 0.01f, 0.01f, 0.01f });
			//trans.setEulerRotation({ -PI * 0.5f, 0.f, 0.f });
		}
	}
}


Result init()
{
	svCheck(offscreen_create(1920u, 1080u, &offscreen));
	svCheck(depthstencil_create(1920u, 1080u, &zbuffer));

	ecs_create(&ecs);

	Entity cam = ecs_entity_create(ecs);
	camera = ecs_component_add<CameraComponent>(ecs, cam);
	camera->far = 10000.f;
	camera->near = 0.2f;
	camera->width = 0.5f;
	camera->height = 0.5f;
	camera->projection_type = ProjectionType_Perspective;

	light = ecs_component_add<PointLightComponent>(ecs, ecs_entity_create(ecs));
	Transform t = ecs_entity_transform_get(ecs, light->entity);
	t.setPosition({ 0.f, 0.f, -2.f });

	gui = gui_create();

	load_model(ecs, "assets/dragon.obj");
	//load_model(ecs, "assets/gobber/GoblinX.obj");
	//load_model(ecs, "assets/Sponza/sponza.obj");

	// Editor stuff
	window = gui_window_create(gui);
	editor_ecs = create_editor_ecs(ecs, gui, window->container);

	return Result_Success;
}

void update()
{
	key_shortcuts();

	gui_update(gui, window_width_get(engine.window), window_height_get(engine.window));

	update_editor_ecs(editor_ecs);

	camera->adjust(f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));

	Transform l = ecs_entity_transform_get(ecs, light->entity);
	static f32 t = 0.f;
	if (input.keys[Key_Left]) t -= engine.deltatime;
	if (input.keys[Key_Right]) t += engine.deltatime;
	l.setPosition({ sin(t) * 7.f, 0.f, cos(t) * 7.f });

	if (input.keys[Key_C] == InputState_Released)
		update_camera = !update_camera;

	if (update_camera)
		camera_controller3D(ecs, *camera);
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	draw_scene(ecs, offscreen, zbuffer);

	gui_render(gui, offscreen, cmd);
	
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result close()
{
	svCheck(graphics_destroy(offscreen));
	svCheck(graphics_destroy(zbuffer));

	ecs_destroy(ecs);

	destroy_editor_ecs(editor_ecs);

	gui_destroy(gui);

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
	desc.windowDesc.bounds = { 0u, 0u, 1080u, 720u };
	desc.windowDesc.flags = WindowFlag_Default;
	desc.windowDesc.iconFilePath = nullptr;
	desc.windowDesc.state = WindowState_Windowed;
	//desc.windowDesc.state = WindowState_Fullscreen;
	desc.windowDesc.title = L"Test";

	if (result_fail(engine_initialize(&desc))) {
		printf("Can't init");
	}
	else {

		while (result_okay(engine_loop()));

		engine_close();
	}

#ifdef SV_ENABLE_LOGGING
	system("pause");
#endif

}
