#include "SilverEngine.h"

using namespace sv;

GPUImage* offscreen = nullptr;
Window* win = nullptr;
DebugRenderer rend;
Font font;
v2_f32				camera_position;
CameraProjection	camera;

GUI* gui;
GuiWindow* gui_window;

void create_color_sliders(GuiContainer& cont, u64 begin_id, f32 y)
{
	GuiSlider* slider;
	slider = gui_slider_create(gui, &cont);
	slider->x.constraint = GuiConstraint_Relative;
	slider->x.value = 0.05f;
	slider->x.alignment = GuiCoordAlignment_Left;
	slider->y.value = y;
	slider->y.constraint = GuiConstraint_Relative;
	slider->y.alignment = GuiCoordAlignment_Bottom;
	slider->w.value = 0.95f / 3.5f;
	slider->w.constraint = GuiConstraint_Relative;
	slider->h.value = 0.05f;
	slider->h.constraint = GuiConstraint_Relative;
	slider->min = 0.f;
	slider->max = 1.f;
	slider->user_id = begin_id++;

	slider = gui_slider_create(gui, &cont);
	slider->x.constraint = GuiConstraint_Relative;
	slider->x.value = 0.5f;
	slider->x.alignment = GuiCoordAlignment_Center;
	slider->y.value = y;
	slider->y.constraint = GuiConstraint_Relative;
	slider->y.alignment = GuiCoordAlignment_Bottom;
	slider->w.value = 0.95f / 3.5f;
	slider->w.constraint = GuiConstraint_Relative;
	slider->h.value = 0.05f;
	slider->h.constraint = GuiConstraint_Relative;
	slider->min = 0.f;
	slider->max = 1.f;
	slider->user_id = begin_id++;

	slider = gui_slider_create(gui, &cont);
	slider->x.constraint = GuiConstraint_Relative;
	slider->x.value = 0.05f;
	slider->x.alignment = GuiCoordAlignment_InverseRight;
	slider->y.value = y;
	slider->y.constraint = GuiConstraint_Relative;
	slider->y.alignment = GuiCoordAlignment_Bottom;
	slider->w.value = 0.95f / 3.5f;
	slider->w.constraint = GuiConstraint_Relative;
	slider->h.value = 0.05f;
	slider->h.constraint = GuiConstraint_Relative;
	slider->min = 0.f;
	slider->max = 1.f;
	slider->user_id = begin_id;
}

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

	// Create gui
	{
		gui = gui_create(window_width_get(engine.window), window_height_get(engine.window));

		gui_window = gui_window_create(gui);
		GuiContainer& cont = *gui_window->container;
		cont.color = Color::Green();
		cont.x.value = 0.5f;
		cont.y.value = 0.5f;

		GuiSlider* slider = gui_slider_create(gui, &cont);
		slider->x.constraint = GuiConstraint_Center;
		slider->x.alignment = GuiCoordAlignment_Center;
		slider->y.value = 0.9f;
		slider->y.constraint = GuiConstraint_Relative;
		slider->y.alignment = GuiCoordAlignment_Bottom;
		slider->w.value = 0.95f;
		slider->w.constraint = GuiConstraint_Relative;
		slider->h.value = 0.05f;
		slider->h.constraint = GuiConstraint_Relative;
		slider->min = 0.f;
		slider->max = 0.05f;
		slider->user_id = 1u;

		slider = gui_slider_create(gui, &cont);
		slider->x.constraint = GuiConstraint_Center;
		slider->x.alignment = GuiCoordAlignment_Center;
		slider->y.value = 0.8f;
		slider->y.constraint = GuiConstraint_Relative;
		slider->y.alignment = GuiCoordAlignment_Bottom;
		slider->w.value = 0.95f;
		slider->w.constraint = GuiConstraint_Relative;
		slider->h.value = 0.05f;
		slider->h.constraint = GuiConstraint_Relative;
		slider->min = 0.f;
		slider->max = 0.1f;
		slider->user_id = 2u;

		create_color_sliders(cont, 3u, 0.7f);
		create_color_sliders(cont, 6u, 0.6f);
		create_color_sliders(cont, 9u, 0.5f);

		GuiButton* button = gui_button_create(gui, &cont);
		button->x.constraint = GuiConstraint_Center;
		button->x.alignment = GuiCoordAlignment_Center;
		button->y.value = 0.45f;
		button->y.constraint = GuiConstraint_Relative;
		button->y.alignment = GuiCoordAlignment_Bottom;
		button->w.value = 1.f;
		button->w.constraint = GuiConstraint_Aspect;
		button->h.value = 0.1f;
		button->h.constraint = GuiConstraint_Relative;
		button->user_id = 12u;
		
	}

	camera.width = 10.f;
	camera.height = 10.f;
	camera.updateMatrix();

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

	gui_resize(gui, window_width_get(engine.window), window_height_get(engine.window));
	gui_update(gui);

	camera.adjust(window_width_get(engine.window), window_height_get(engine.window));

	if (!gui_locked_input(gui).mouse_click) {
		editor_camera_controller2D(camera_position, camera);
	}

	if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		if (gui_locked_input(gui).mouse_click) {
			SV_LOG("Clicked inside");
		}
		else {
			SV_LOG("Clicked outside");
		}

	}

	GuiWidget* widget = gui_widget_hovered(gui);
	if (widget) {
	}

	widget = gui_widget_clicked(gui);
	if (widget) {
	}

	widget = gui_widget_focused(gui);

	if (widget) {

		switch (widget->type)
		{
		case GuiWidgetType_Slider:
		{
			GuiSlider& slider = *reinterpret_cast<GuiSlider*>(widget);
			
			switch (slider.user_id)
			{
			case 1:
				gui_window->outline_size = slider.value;
				break;
			case 2:
				gui_window->decoration_height = slider.value;
				break;

			case 3:
				gui_window->color.r = u8(slider.value * 255.f);
				break;

			case 4:
				gui_window->color.g = u8(slider.value * 255.f);
				slider.color.g = u8(slider.value * 255.f);
				break;

			case 5:
				gui_window->color.b = u8(slider.value * 255.f);
				slider.color.b = u8(slider.value * 255.f);
				break;

			case 6:
				gui_window->decoration_color.r = u8(slider.value * 255.f);
				slider.color.r = u8(slider.value * 255.f);
				break;

			case 7:
				gui_window->decoration_color.g = u8(slider.value * 255.f);
				slider.color.g = u8(slider.value * 255.f);
				break;

			case 8:
				gui_window->decoration_color.b = u8(slider.value * 255.f);
				slider.color.b = u8(slider.value * 255.f);
				break;

			case 9:
				gui_window->container->color.r = u8(slider.value * 255.f);
				slider.color.r = u8(slider.value * 255.f);
				break;

			case 10:
				gui_window->container->color.g = u8(slider.value * 255.f);
				slider.color.g = u8(slider.value * 255.f);
				break;

			case 11:
				gui_window->container->color.b = u8(slider.value * 255.f);
				slider.color.b = u8(slider.value * 255.f);
				break;
			}

		}
		break;

		}
	}
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Blue(), 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	rend.reset();
	rend.setTexcoord({ 0.f, 0.f, 1.f, 1.f });
	rend.drawSprite(v3_f32{ 0.f, 0.5f, 0.f }, v2_f32{ 0.5f, 0.5f }, Color::White(), font.image);
	rend.drawLine({ -1.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, Color::Red());
	rend.drawLine({ -1.f, -0.1f, 0.f }, { 1.f, -0.1f, 0.f }, Color::Black());
	rend.drawLine({ -1.f, -0.2f, 0.f }, { 1.f, -0.2f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.3f, 0.f }, { 1.f, -0.3f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.4f, 0.f }, { 1.f, -0.4f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.5f, 0.f }, { 1.f, -0.5f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.6f, 0.f }, { 1.f, -0.6f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.7f, 0.f }, { 1.f, -0.7f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.8f, 0.f }, { 1.f, -0.8f, 0.f }, Color::Orange());
	rend.drawLine({ -1.f, -0.9f, 0.f }, { 1.f, -0.9f, 0.f }, Color::Orange());

	static std::string text = "";

	const char* inp = input.text.c_str();
	if (inp)
		text += inp;

	for (const TextCommand& cmd : input.text_commands) {

		switch (cmd)
		{
		case TextCommand_DeleteLeft:
			if (text.size()) text.pop_back();
			break;
		case TextCommand_Enter:
			text += '\n';
			break;
		}
	}

	f32 x = 0.005f;
	f32 y = 0.55f;
	f32 size = 0.05f;
	f32 width = 0.2f;

	XMMATRIX vp_matrix = XMMatrixTranslation(camera_position.x, camera_position.y, 0.f) * camera.projectionMatrix;
	rend.render(offscreen, vp_matrix, cmd);
	
	f32 window_aspect = f32(window_width_get(engine.window)) / f32(window_height_get(engine.window));

	u32 num = draw_text(text.c_str(), x, y, width, 4u, size, window_aspect, TextSpace_Normal, TextAlignment_Center, &font, offscreen, cmd);
	text.resize(num);

	gui_render(gui, offscreen, cmd);

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

	system("pause");

}