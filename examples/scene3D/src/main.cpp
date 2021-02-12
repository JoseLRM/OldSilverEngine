#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

using namespace sv;

GPUImage* offscreen = nullptr;
GPUImage* zbuffer = nullptr;
v3_f32				camera_position;
v2_f32				camera_rotation;
CameraProjection	camera;
CameraBuffer		camera_buffer;
Mesh plane;
Mesh model;
Material mat;

GUI* gui;

Result init()
{
	svCheck(offscreen_create(1920u, 1080u, &offscreen));
	svCheck(zbuffer_create(1920u, 1080u, &zbuffer));

	svCheck(camerabuffer_create(&camera_buffer));

	gui = gui_create();

	// TEMP: create meshes
	mesh_apply_plane(plane, XMMatrixScaling(30.f, 0.f, 30.f));
	mesh_create_buffers(plane);

	ModelInfo info;
	if (result_fail(model_load("assets/MP44/MP44.obj", info))) {
		SV_LOG_ERROR("Can't load the model");
	}
	else {

		model.positions = std::move(info.meshes.back().positions);
		model.normals = std::move(info.meshes.back().normals);
		model.texcoords = std::move(info.meshes.back().texcoords);
		model.indices = std::move(info.meshes.back().indices);
		
		mesh_set_scale(model, 1.f, true);

		mesh_create_buffers(model);

		mat.diffuse_color = info.materials.back().diffuse_color;
	}

	return Result_Success;
}

void update()
{
	key_shortcuts();

	gui_update(gui, window_width_get(engine.window), window_height_get(engine.window));

	projection_adjust(camera, f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));

	camera_controller3D(camera_position, camera_rotation, camera);
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	render_context[cmd].offscreen = offscreen;
	render_context[cmd].zbuffer = zbuffer;
	render_context[cmd].camera_buffer = &camera_buffer;

	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Black(), 1.f, 0u, cmd);
	graphics_image_clear(zbuffer, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::White(), 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	// Process lighting

	LightInstance lights[] = { LightInstance(Color3f::White(), v3_f32{ camera_position.x, camera_position.y, camera_position.z }, 3.f, 1.f, 0.5f) };

	// Mesh rendering

	static FrameList<MeshInstance> meshes;

	// Draw gun

	meshes.reset();

	camera.projection_type = ProjectionType_Perspective;
	camera.width = 0.3f;
	camera.height = 0.3f;
	camera.near = 0.1f;
	camera.far = 1000.f;
	projection_update_matrix(camera);

	camera_buffer.rotation = v4_f32(XMQuaternionRotationRollPitchYaw(0.f, 0.f, 0.f));
	camera_buffer.view_matrix = XMMatrixIdentity();
	camera_buffer.projection_matrix = camera.projection_matrix;
	camera_buffer.position = {};
	camerabuffer_update(&camera_buffer, cmd);

	XMMATRIX gun_matrix = XMMatrixRotationY(ToRadians(180.f))
		* XMMatrixTranslation(0.2f, -0.2f, 0.35f);

	meshes.emplace_back(gun_matrix, &model, &mat);

	lights[0] = { LightInstance(Color3f::White(), v3_f32{ }, 3.f, 1.f, 0.5f) };

	draw_meshes(meshes.data(), u32(meshes.size()), lights, 1u, cmd);

	// Draw rest of the scene

	meshes.reset();

	camera.projection_type = ProjectionType_Perspective;
	camera.width = 0.3f;
	camera.height = 0.3f;
	camera.near = 0.3f;
	camera.far = 1000.f;
	projection_update_matrix(camera);

	camera_buffer.rotation = v4_f32(XMQuaternionRotationRollPitchYawFromVector(camera_rotation.getDX()));
	camera_buffer.view_matrix = math_matrix_view(camera_position, camera_buffer.rotation);
	camera_buffer.projection_matrix = camera.projection_matrix;
	camera_buffer.position = camera_position;
	camerabuffer_update(&camera_buffer, cmd);

	
	Material plane_mat;
	plane_mat.diffuse_color = { 0.05f, 0.5f, 0.8f };

	meshes.emplace_back(XMMatrixTranslation(0.f, 0.f, 0.f), &plane, &plane_mat);

	lights[0] = { LightInstance(Color3f::White(), v3_f32{ camera_position.x, camera_position.y, camera_position.z }, 3.f, 1.f, 0.5f) };

	draw_meshes(meshes.data(), u32(meshes.size()), lights, 1u, cmd);

	gui_render(gui, cmd);
	
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result close()
{
	svCheck(graphics_destroy(offscreen));
	svCheck(graphics_destroy(zbuffer));

	camerabuffer_destroy(&camera_buffer);
	mesh_clear(model);
	mesh_clear(plane);
	gui_destroy(gui);

	return Result_Success;
}

int main()
{
	InitializationDesc desc;
	desc.assetsFolderPath = "assets/";
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
