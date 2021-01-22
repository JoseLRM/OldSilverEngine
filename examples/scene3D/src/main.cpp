#include "SilverEngine.h"

using namespace sv;

GPUImage* offscreen = nullptr;
Window* win = nullptr;
DebugRenderer rend;
Font font;
v3_f32				camera_position;
v2_f32				camera_rotation;
CameraProjection	camera;

Result init()
{
	{
		svCheck(graphics_offscreen_create(1920u, 1080u, &offscreen));
	}
	{
		WindowDesc desc;
		desc.bounds = { 0u, 0u, 1080u, 720u };
		desc.flags = WindowFlag_Default;
		desc.iconFilePath = nullptr;
		desc.state = WindowState_Windowed;
		desc.title = L"Test2!!";
		//svCheck(window_create(&desc, &win));
	}

	// Init font
	{
		//svCheck(font_create(font, "C:/Windows/Fonts/arial.ttf", 128.f, 0));
		//svCheck(font_create(font, "C:/Windows/Fonts/SourceCodePro-Black.ttf", 128.f, 0));
		svCheck(font_create(font, "C:/Windows/Fonts/VIVALDII.TTF", 300.f, 0));
		//svCheck(font_create(font, "C:/Windows/Fonts/ROCCB___.TTF", 200.f, 0));
		//svCheck(font_create(font, "C:/Windows/Fonts/LiberationMono-BoldItalic.ttf", 200.f, 0));
		//svCheck(font_create(font, "C:/Windows/Fonts/consola.ttf", 128.f, 0));
	}

	camera.projectionType = ProjectionType_Perspective;
	camera.width = 0.1f;
	camera.height = 0.1f;
	camera.near = 0.1f;
	camera.far = 1000.f;

	rend.create();

	return Result_Success;
}

void update()
{
	if (input.keys[Key_F11] == InputState_Pressed) {
		engine.close_request = true;
	}
	if (input.keys[Key_F10] == InputState_Pressed) {
		if (window_state_get(engine.window) == WindowState_Fullscreen) {
			window_state_set(engine.window, WindowState_Windowed);
		}
		else window_state_set(engine.window, WindowState_Fullscreen);
	}

	camera.adjust(window_width_get(engine.window), window_height_get(engine.window));

	editor_camera_controller3D(camera_position, camera_rotation, camera);

	camera.updateMatrix();
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Blue(), 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	rend.reset();
	rend.setTexcoord({ 0.f, 0.f, 1.f, 1.f });

	for (f32 x = 0.f; x < 10.f; x += 2.f)
	for (f32 y = 0.f; y < 10.f; y += 2.f)
	for (f32 z = 0.f; z < 10.f; z += 2.f)
		rend.drawSprite(v3_f32{ x, y, z }, v2_f32{ 0.5f, 0.5f }, Color::White(), font.image);

	XMMATRIX vp_matrix = math_matrix_view(camera_position, v4_f32(XMQuaternionRotationRollPitchYawFromVector(camera_rotation.getDX()))) * camera.projectionMatrix;
	rend.render(offscreen, vp_matrix, cmd);
	
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);

	if (win) {
		graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Blue(), 1.f, 0u, cmd);
		graphics_present(win, offscreen, GPUImageLayout_RenderTarget, cmd);
	}
}

Result close()
{
	svCheck(graphics_destroy(offscreen));
	svCheck(window_destroy(win));

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

	system("pause");

}