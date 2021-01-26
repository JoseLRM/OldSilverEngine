#include "SilverEngine.h"

using namespace sv;

GPUImage* offscreen = nullptr;
GPUImage* zbuffer = nullptr;
v3_f32				camera_position;
v2_f32				camera_rotation;
CameraProjection	camera;
CameraBuffer		camera_buffer;
Mesh mesh;

GUI* gui;
Editor* editor;

Result init()
{
	svCheck(offscreen_create(1920u, 1080u, &offscreen));
	svCheck(zbuffer_create(1920u, 1080u, &zbuffer));

	camera.projection_type = ProjectionType_Perspective;
	camera.width = 0.3f;
	camera.height = 0.3f;
	camera.near = 0.2f;
	camera.far = 1000.f;

	svCheck(camerabuffer_create(&camera_buffer));

	gui = gui_create(window_width_get(engine.window), window_height_get(engine.window));
	editor = editor_create(gui);

	editor_runtime_create(editor);

	// TEMP: create cube mesh
	mesh_apply_cube(mesh);
	mesh_create_buffers(mesh);

	return Result_Success;
}

void update()
{
	editor_key_shortcuts(editor);

	gui_resize(gui, window_width_get(engine.window), window_height_get(engine.window));
	gui_update(gui);
	editor_update(editor);

	projection_adjust(camera, f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));

	editor_camera_controller3D(camera_position, camera_rotation, camera);

	projection_update_matrix(camera);
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	render_context[cmd].offscreen = offscreen;
	render_context[cmd].zbuffer = zbuffer;
	render_context[cmd].camera_buffer = &camera_buffer;

	camera_buffer.rotation = v4_f32(XMQuaternionRotationRollPitchYawFromVector(camera_rotation.getDX()));
	camera_buffer.view_matrix = math_matrix_view(camera_position, camera_buffer.rotation);
	camera_buffer.projection_matrix = camera.projection_matrix;
	camera_buffer.position = camera_position;
	camerabuffer_update(&camera_buffer, cmd);

	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Blue(), 1.f, 0u, cmd);
	graphics_image_clear(zbuffer, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::White(), 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	draw_mesh(&mesh, XMMatrixRotationY(cos(timer_now()) * 5.f) * XMMatrixTranslation(sin(timer_now()) * 2.f - 1.f, 0.f, 0.f), cmd);

	gui_render(gui, offscreen, cmd);
	
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result close()
{
	svCheck(graphics_destroy(offscreen));
	svCheck(graphics_destroy(zbuffer));

	mesh_clear(mesh);
	editor_destroy(editor);
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
	desc.windowDesc.state = WindowState_Fullscreen;
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