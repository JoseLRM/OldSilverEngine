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

Entity select_mesh()
{
	v2_f32 mouse = input.mouse_position;

	// Screen to clip space
	mouse *= 2.f;

	// clip to world
	XMMATRIX vpm = XMMatrixIdentity(); // TODO
	vpm = XMMatrixInverse(nullptr, vpm);

	XMVECTOR mouse_world = XMVectorSet(mouse.x, mouse.y, 0.f, 1.f);
	mouse_world = XMVector3Transform(mouse_world, vpm);

	// Ray
	Transform trans = ecs_entity_transform_get(ecs, camera->entity);
	v3_f32 camera_position = trans.getWorldPosition();

	v3_f32 ray_origin = camera_position;
	v3_f32 ray_direction = v3_f32(mouse_world) - camera_position;
	ray_direction.normalize();

	Entity selected = SV_ENTITY_NULL;
	f32 distance = f32_max;

	EntityView<MeshComponent> meshes(ecs);

	for (MeshComponent& m : meshes) {

		Transform trans = ecs_entity_transform_get(ecs, m.entity);
		v3_f32 position = trans.getWorldPosition();
		v3_f32 scale = trans.getWorldScale();

		f32 radius = std::max(std::max(scale.x, scale.y), scale.z) * 0.5f;

		v3_f32 to_sphere = position - ray_origin;

		f32 dot = to_sphere.dot(ray_direction);

		if (dot <= 0.f) {

			if (abs(dot) > radius) continue;
			if (abs(dot) == radius) {

				f32 d = (position - ray_origin).length();
				if (d < distance) {

					distance = d;
					selected = m.entity;
				}
				continue;
			}
			else {

			}
		}
		else {

			v3_f32 projection = ray_origin + ray_direction * dot;

			f32 projection_to_center = (projection - position).length();

			if (projection_to_center > radius) continue;

			f32 dist = math_sqrt(radius * radius - projection_to_center * projection_to_center);

			f32 d0 = dot - dist;
			f32 d1 = dot + dist;
			f32 d = std::min(d0, d1);
			if (d < distance) {

				distance = d;
				selected = m.entity;
			}
			continue;
		}
	}

	return selected;
}


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
			trans.setMatrix(m.transform_matrix);
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

	//load_model(ecs, "assets/dragon.obj");
	//load_model(ecs, "assets/gobber/GoblinX.obj");
	load_model(ecs, "assets/Sponza/sponza.obj");

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

	if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {
		Entity selected = select_mesh();

		if (selected != SV_ENTITY_NULL)
			SV_LOG("Selected %u", selected);
	}

	if (update_camera)
		camera_controller3D(ecs, *camera, 0.1f);
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
