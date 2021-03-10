#include "SilverEngine/core.h"

#include "SilverEngine/engine.h"
#include "SilverEngine/asset_system.h"
#include "SilverEngine/platform/impl.h"

#include "task_system/task_system_internal.h"
#include "event_system/event_system_internal.h"
#include "window/window_internal.h"
#include "graphics/graphics_internal.h"

#include "renderer/renderer_internal.h"

namespace sv {

	GlobalEngineData	engine;
	GlobalInputData		input;

#ifdef SV_DEV
	GlobalDevData dev;
#endif

	void update_assets();
	void close_assets();
	Result initialize_scene(Scene** pscene, const char* name);
	Result close_scene(Scene* scene);
	void update_scene(Scene* scene);

#ifdef SV_DEV
	Result initialize_editor();
	Result close_editor();
	void update_editor();
	void draw_editor(CommandList cmd);
	void initialize_console();
	void close_console();
	void update_console();
	void draw_console(CommandList cmd);
#endif

	// Asset functions

	static Result create_image_asset(void* asset)
	{
		GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
		image = nullptr;
		return Result_Success;
	}

	static Result load_image_asset(void* asset, const char* filepath)
	{
		GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);

		// Get file data
		void* data;
		u32 width;
		u32 height;
		svCheck(load_image(filepath, &data, &width, &height));

		// Create Image
		GPUImageDesc desc;

		desc.pData = data;
		desc.size = width * height * 4u;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.width = width;
		desc.height = height;

		Result res = graphics_image_create(&desc, &image);

		delete[] data;
		return res;
	}

	static Result destroy_image_asset(void* asset)
	{
		GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
		graphics_destroy(image);
		image = nullptr;
		return Result_Success;
	}

	static Result reload_image_asset(void* asset, const char* filepath)
	{
		svCheck(destroy_image_asset(asset));
		return load_image_asset(asset, filepath);
	}

	static Result create_mesh_asset(void* asset)
	{
		Mesh& mesh = *new(asset) Mesh();
		return Result_Success;
	}

	static Result load_mesh_asset(void* asset, const char* filepath)
	{
		Mesh& mesh = *new(asset) Mesh();
		svCheck(load_mesh(filepath, mesh));
		svCheck(mesh_create_buffers(mesh));
		return Result_Success;
	}

	static Result free_mesh_asset(void* asset)
	{
		Mesh& mesh = *reinterpret_cast<Mesh*>(asset);

		graphics_destroy(mesh.vbuffer);
		graphics_destroy(mesh.ibuffer);

		mesh.~Mesh();
		return Result_Success;
	}

	static Result create_material_asset(void* asset)
	{
		Material& material = *new(asset) Material();
		return Result_Success;
	}

	static Result load_material_asset(void* asset, const char* filepath)
	{
		Material& material = *new(asset) Material();
		svCheck(load_material(filepath, material));
		return Result_Success;
	}

	static Result free_material_asset(void* asset)
	{
		Material& mat = *reinterpret_cast<Material*>(asset);
		mat.~Material();
		return Result_Success;
	}

	// Initialization

#define INIT_SYSTEM(name, fn) res = fn; if (result_okay(res)) SV_LOG_INFO(name ": %s", result_str(res)); else { SV_LOG_ERROR(name ": %s", result_str(res)); return res; }

	static Result initialize(const InitializationDesc& desc)
	{
		Result res;

#ifdef SV_DEV
		initialize_console();
#else
#ifdef SV_PLATFORM_WIN
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
#endif

		SV_LOG_INFO("Initializing %s", engine.name.c_str());

		// CORE
		INIT_SYSTEM("TaskSystem",		task_initialize(desc.minThreadsCount));
		INIT_SYSTEM("EventSystem",		event_initialize());
		INIT_SYSTEM("Window",			window_initialize());
		INIT_SYSTEM("GraphicsAPI",		graphics_initialize());
		INIT_SYSTEM("Renderer",			renderer_initialize());

		// Open Main Window
		res = window_create(&desc.windowDesc, &engine.window);
		if (result_fail(res)) {
			SV_LOG_ERROR("Can't open the main window...");
			return res;
		}

		svCheck(create_offscreen(1920u, 1080u, &engine.offscreen));
		svCheck(create_depthstencil(1920u, 1080u, &engine.depthstencil));

		// Register components
		{
			register_component<SpriteComponent>("Sprite");
			register_component<CameraComponent>("Camera");
			register_component<MeshComponent>("Mesh");
			register_component<LightComponent>("Light");
		}

		// Register assets
		{
			AssetTypeDesc desc;
			const char* extensions[5u];
			desc.extensions = extensions;


			// Texture
			extensions[0] = "png";
			extensions[1] = "tga";
			extensions[2] = "jpg";
			extensions[3] = "jpeg";
			extensions[4] = "JPG";

			desc.name = "Texture";
			desc.asset_size = sizeof(GPUImage*);
			desc.extension_count = 5u;
			desc.create = create_image_asset;
			desc.load_file = load_image_asset;
			desc.free = destroy_image_asset;
			desc.reload_file = reload_image_asset;
			desc.unused_time = 3.f;

			svCheck(register_asset_type(&desc));

			// Mesh
			extensions[0] = "mesh";

			desc.name = "Mesh";
			desc.asset_size = sizeof(Mesh);
			desc.extension_count = 1u;
			desc.create = create_mesh_asset;
			desc.load_file = load_mesh_asset;
			desc.free = free_mesh_asset;
			desc.reload_file = nullptr;
			desc.unused_time = 5.f;

			svCheck(register_asset_type(&desc));

			// Material
			extensions[0] = "mat";

			desc.name = "Material";
			desc.asset_size = sizeof(Material);
			desc.extension_count = 1u;
			desc.create = create_material_asset;
			desc.load_file = load_material_asset;
			desc.free = free_material_asset;
			desc.reload_file = nullptr;
			desc.unused_time = 2.5f;

			svCheck(register_asset_type(&desc));
		}

#ifdef SV_DEV
		svCheck(initialize_editor());
#endif

		// APPLICATION
		INIT_SYSTEM("Application", engine.callbacks.initialize());

		return res;
	}

	v2_f32 next_mouse_position;
	bool new_mouse_position = false;

	static void update_input()
	{
		input.text.clear();
		input.text_commands.resize(1u);
		input.text_commands.back() = TextCommand_Null;

		// Reset unused input
		input.unused = true;

		// KEYS
		// reset pressed and released
		for (Key i = Key(0); i < Key_MaxEnum; ++(u32&)i) {

			if (input.keys[i] == InputState_Pressed) {
				input.keys[i] = InputState_Hold;
			}
			else if (input.keys[i] == InputState_Released) input.keys[i] = InputState_None;
		}

		// MOUSE BUTTONS
		// reset pressed and released
		for (MouseButton i = MouseButton(0); i < MouseButton_MaxEnum; ++(u32&)i) {

			if (input.mouse_buttons[i] == InputState_Pressed) {
				input.mouse_buttons[i] = InputState_Hold;
			}
			else if (input.mouse_buttons[i] == InputState_Released) input.mouse_buttons[i] = InputState_None;
		}

		input.mouse_last_pos = input.mouse_position;
		input.mouse_wheel = 0.f;

#ifdef SV_PLATFORM_WIN
		MSG msg;

		while (PeekMessageW(&msg, 0, 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
#endif

		input.mouse_dragged = input.mouse_position - input.mouse_last_pos;

		// Move mouse
		if (new_mouse_position) {

			Window* window = input.focused_window;

			if (window) {

				v2_u32 size = window_size_get(window);
				v2_u32 pos = window_position_get(window);
#ifdef SV_PLATFORM_WIN
				SetCursorPos(i32(pos.x + u32(f32(size.x) * next_mouse_position.x)), i32(pos.y + u32(f32(size.y) * (1.f - next_mouse_position.y))));

				MSG msg;
				PeekMessageW(&msg, 0, 0u, 0u, PM_REMOVE);
#endif

				input.mouse_position = next_mouse_position - 0.5f;
				input.mouse_last_pos = input.mouse_position;
			}

			new_mouse_position = false;
		}
	}

#define CATCH catch (std::exception e) {\
					SV_LOG_ERROR("STD Exception: %s", e.what()); \
					exception_catched = true;\
					sv::show_message(L"STD Exception", sv::parse_wstring(e.what()).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (int i) {\
					SV_LOG_ERROR("Unknown Exception %i", i); \
					exception_catched = true;\
					sv::show_message(L"Unknown Exception", std::to_wstring(i).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (...) {\
					SV_LOG_ERROR("Unknown Exception"); \
					exception_catched = true;\
					sv::show_message(L"Unknown Exception", L"", sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}


	Result engine_initialize(const InitializationDesc* d)
	{
		if (d == nullptr) return Result_InvalidUsage;

		bool exception_catched = false;

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		engine.callbacks = desc.callbacks;

		Time initTimeBegin = timer_now();

		// SYSTEMS
		Result res;
		try {
			res = initialize(desc);
		}
		CATCH;

		if (result_okay(res)) {
			SV_LOG_INFO("Initialized successfuly");
		}
		else {
			SV_LOG_ERROR("Initialization failed");
		}
		
		Time initTimeEnd = timer_now();

		SV_LOG_INFO("Time: %f", initTimeEnd - initTimeBegin);

		SV_LOG_SEPARATOR();

		engine.able_to_run = result_okay(res);
		
		return Result_Success;
	}

	Result engine_loop()
	{
		if (!engine.able_to_run) {
			printf("The engine is not able to run\n");
			return Result_CloseRequest;
		}

		if (engine.close_request)
		{
			return Result_CloseRequest;
		}

		static Time	lastTime = 0.f;
		bool exception_catched = false;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			engine.deltatime = f32(actualTime - lastTime);
			lastTime = actualTime;

			if (engine.deltatime > 0.3f) engine.deltatime = 0.3f;

			// Calculate FPS
			{
				static f32 fpsTimeCount = 0.f;
				static u32 fpsCount = 0u;

				fpsTimeCount += engine.deltatime;
				++fpsCount;
				if (fpsTimeCount > 0.25f) {
					fpsTimeCount -= 0.25f;
					engine.FPS = fpsCount * 4u;
					fpsCount = 0u;
				}
			}

			// Update Input
			update_input();

			if (!window_update(engine.window)) {
				engine.close_request = true;
				engine.window = nullptr;
				return Result_CloseRequest;
			}

			update_assets();

#ifdef SV_DEV
			update_console();
#endif

			// Scene management
			{
				if (engine.next_scene_name.size()) {

					// Close last scene
					if (engine.scene) {
						Result res = close_scene(engine.scene);
						// TOOD: handle error
					}

					Result res = initialize_scene(&engine.scene, engine.next_scene_name.c_str());
					engine.next_scene_name.clear();
					// Handle error

				}
			}

#ifdef SV_DEV
			update_editor();
#endif

			// Update user
			if (engine.callbacks.update)
				engine.callbacks.update();

			// Update scene
			if (engine.scene) update_scene(engine.scene);

			if (window_has_valid_surface(engine.window)) {

				// Begin Rendering
				graphics_begin();

				CommandList cmd = graphics_commandlist_begin();

				graphics_image_clear(engine.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Black(), 1.f, 0u, cmd);
				graphics_image_clear(engine.depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::Black(), 1.f, 0u, cmd);

				graphics_viewport_set(engine.offscreen, 0u, cmd);
				graphics_scissor_set(engine.offscreen, 0u, cmd);

				// Draw scene
				if (engine.scene) draw_scene(engine.scene);

				// Draw editor and the console and present to screen
				cmd = graphics_commandlist_get();

#ifdef SV_DEV
				draw_editor(cmd);
				draw_console(cmd);
#endif

				graphics_present(engine.window, engine.offscreen, GPUImageLayout_RenderTarget, cmd);

				// End frame
				graphics_end();
			}
			else std::this_thread::sleep_for(std::chrono::milliseconds(10u));
		}
		CATCH;

		++engine.frame_count;

		if (exception_catched) 
			engine.close_request = true;

		return Result_Success;
	}

	Result engine_close()
	{
		if (!engine.able_to_run) {
			printf("The engine is not able to close\n");
			return Result_InvalidUsage;
		}
		engine.able_to_run = false;

		bool exception_catched = false;

		SV_LOG_SEPARATOR();
		SV_LOG_INFO("Closing %s", engine.name.c_str());

		// APPLICATION
		try {
			if (engine.scene) svCheck(close_scene(engine.scene));
			svCheck(engine.callbacks.close());

			free_unused_assets();

#ifdef SV_DEV
			close_editor();
#endif

			graphics_destroy(engine.offscreen);
			graphics_destroy(engine.depthstencil);

			// Close Window
			svCheck(window_destroy(engine.window));

			// CORE
			if (result_fail(renderer_close())) { SV_LOG_ERROR("Can't close render utils"); }
			if (result_fail(graphics_close())) { SV_LOG_ERROR("Can't close graphicsAPI"); }
			if (result_fail(window_close())) { SV_LOG_ERROR("Can't close window"); }
			close_assets();
			if (result_fail(event_close())) { SV_LOG_ERROR("Can't close the event system"); }
			if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }

#ifdef SV_DEV
			close_console();
#endif
		}
		CATCH;

		return Result_Success;
	}

}
