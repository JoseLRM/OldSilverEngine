#include "SilverEngine.h"

using namespace sv;

GPUImage* offscreen = nullptr;
Window* win = nullptr;
DebugRenderer rend;
Font font;

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

	rend.create();

	return Result_Success;
}

void update()
{
	//if (input_key_pressed(' '))
	//	window_state_set(g_Engine.window, window_state_get(g_Engine.window) == WindowState_Fullscreen ? WindowState_Windowed : WindowState_Fullscreen);

	//static u32 lastFPS = 0u;
	//if (lastFPS != g_Engine.FPS) {
	//	SV_LOG("FPS: %u", g_Engine.FPS);
	//	lastFPS = g_Engine.FPS;
	//}

	if (win) {
		if (!window_update(win)) {
			win = nullptr;
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

	const char* input = input_text();
	if (input)
		text += input;

	const TextCommand* commands = input_text_commands();
	while (*commands != TextCommand_Null) {

		switch (*commands)
		{
		case TextCommand_DeleteLeft:
			if (text.size()) text.pop_back();
			break;
		case TextCommand_Enter:
			text += '\n';
			break;
		}

		++commands;
	}

	static TextSpace space = TextSpace_Clip;

	if (input_mouse_pressed(SV_MOUSE_LEFT)) {
		space = TextSpace((u32(space) + 1u) % (u32(TextSpace_Offscreen) + 1u));

		switch (space)
		{
		case TextSpace_Clip:
			SV_LOG("Clip space!!");
			break;
		case TextSpace_Normal:
			SV_LOG("Normal space!!");
			break;
		case TextSpace_Offscreen:
			SV_LOG("Offscreen space!!");
			break;
		}
	}

	f32 x = 0.005f;
	f32 y = 0.55f;
	f32 size = 0.05f;
	f32 width = 0.2f;

	switch (space)
	{
	case TextSpace_Clip:
		x = x * 2.f - 1.f;
		y = y * 2.f - 1.f;
		size *= 2.f;
		width *= 2.f;
		break;
	case TextSpace_Offscreen:
		x *= 1920.f;
		y *= 1080.f;
		size *= 1080.f;
		width *= 1920.f;
		break;
	}

	if (input_mouse(SV_MOUSE_CENTER)) {
		const v2_f32& p = input_mouse_position();
		x = p.x;
		y = p.y;

		switch (space)
		{
		case TextSpace_Clip:
			x *= 2.f;
			y *= 2.f;
			break;
		case TextSpace_Normal:
			x += 0.5f;
			y += 0.5f;
			break;
		case TextSpace_Offscreen:
			x = (x + 0.5f) * 1920.f;
			y = (y + 0.5f) * 1080.f;
			break;
		}
	}


	rend.drawQuad({ x + width * 0.5f, y - 1.f, 0.f }, { width, 2.f }, Color::Red(30u));
	rend.render(offscreen, XMMatrixIdentity(), cmd);
	
	f32 window_aspect = f32(window_width_get(g_Engine.window)) / f32(window_height_get(g_Engine.window));

	u32 num = draw_text(offscreen, text.c_str(), x, y, width, 4u, size, window_aspect, space, TextAlignment_Center, &font, cmd);
	text.resize(num);

	graphics_present(g_Engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);

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