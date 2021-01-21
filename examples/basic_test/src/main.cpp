#include "SilverEngine.h"

using namespace sv;

GPUImage* offscreen = nullptr;
Window* win = nullptr;
DebugRenderer rend;
Font font;
f32 p = 0.f;

GUI* gui;

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

		GuiContainer& cont = *gui_window_create(gui)->container;
		cont.color = Color::Green();
		cont.x.value = 0.5f;
		cont.y.value = 0.5f;

		GuiButton& button = gui_button_create(gui, &cont);
		button.x.constraint = GuiConstraint_Center;
		button.y.value = 1.f;
		button.y.constraint = GuiConstraint_Relative;
		button.y.alignment = GuiCoordAlignment_Top;

		button.w.constraint = GuiConstraint_Relative;
		button.w.value = 0.2f;
		button.h.value = 1.f;
		button.h.constraint = GuiConstraint_Aspect;

		button.color = Color::Orange();
		button.user_id = 69u;

		GuiSlider& slider = gui_slider_create(gui, &cont);
		slider.x.constraint = GuiConstraint_Center;
		slider.x.alignment = GuiCoordAlignment_Center;

		slider.y.value = 0.4f;
		slider.y.constraint = GuiConstraint_Relative;
		slider.y.alignment = GuiCoordAlignment_Bottom;

		slider.w.value = 0.95f;
		slider.w.constraint = GuiConstraint_Relative;

		slider.h.value = 0.08f;
		slider.h.constraint = GuiConstraint_Aspect;

		slider.min = 0.f;
		slider.max = 0.5f;
	}

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

	if (win) {
		if (!window_update(win)) {
			win = nullptr;
		}
	}

	gui_resize(gui, window_width_get(engine.window), window_height_get(engine.window));
	gui_update(gui);

	if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		if (gui_locked_input(gui).mouse_click) {
			SV_LOG("Clicked inside");
		}
		else {
			SV_LOG("Clicked outside");
		}

	}

	GuiWidget* widget = gui_widget_hovered(gui);
	if (widget && widget->user_id == 69u) {

		GuiButton& button = *reinterpret_cast<GuiButton*>(widget);
		if (button.hover_state == HoverState_Hover) {
			button.w.value = 0.2f + sin(timer_now() * 5.f) * 0.03f;
		}
		else if (button.hover_state == HoverState_Leave) {
			button.w.value = 0.2f;
		}
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
			p = slider.value;
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

	f32 x = 0.005f + p;
	f32 y = 0.55f;
	f32 size = 0.05f;
	f32 width = 0.2f;

	rend.drawQuad({ (x * 2.f - 1.f) + width * 2.f * 0.5f, y * 2.f - 1.f, 0.f }, { width * 2.f, 2.f }, Color::Red(30u));
	rend.render(offscreen, XMMatrixIdentity(), cmd);
	
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