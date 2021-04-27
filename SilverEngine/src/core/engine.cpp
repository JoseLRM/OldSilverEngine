#include "core/engine.h"
#include "core/scene.h"
#include "core/asset_system.h"
#include "core/renderer.h"
#include "core/event_system.h"

#include "platform/os.h"

#include "user.h"

#include "debug/editor.h"
#include "debug/console.h"

namespace sv {

    GlobalEngineData engine;
    GlobalInputData input;

#if SV_DEV
    GlobalDevData dev;
    static Date last_user_lib_write = {};
#endif

    ////////////////////////////////////////////////////////////////// UPDATE DLL ////////////////////////////////////////////////////////////
    
    SV_AUX void recive_user_callbacks(bool init)
    {
	const char* filepath = "system/game_bin/Game.dll";
	
#if SV_DEV
	
	_user_close(false);

	event_unregister_flags(EventFlag_User);
	_os_free_user_callbacks();
	
	if (!file_copy(filepath, "system/game_bin/GameTemp.dll")) {
	    SV_LOG_ERROR("Can't create temporal game dll");
	    return;
	}
	
	_os_update_user_callbacks("system/game_bin/GameTemp.dll");
	
	if (!init)
	    _user_initialize(false);

	Date date;
	if (file_date(filepath, nullptr, &date, nullptr)) {
	    last_user_lib_write = date;
	}
#else
	_os_update_user_callbacks(filepath);
#endif
    }
    
#if SV_DEV
    
    SV_INTERNAL void update_user_callbacks()
    {
	static f64 last_update = 0.0;
	f64 now = timer_now();
	
	if (now - last_update > 1.0) {

	    last_update = now;
	    const char* filepath = "system/game_bin/Game.dll";
	    const char* respath = "system/build_output.txt";

	    // Get compilation result
	    if (file_exists(respath)) {

		char* str;
		size_t size;
		if (file_read_text(respath, &str, &size)) {

		    SV_LOG_INFO("Compilation result:\n");
		    SV_LOG("%s\n", str);
		    SV_FREE_MEMORY(str);

		    if (!file_remove(respath)) {
			SV_LOG_ERROR("Can't delete the compilation result file");
		    }
		}
	    }
	    
	    // Check if the file is modified
	    Date date;
	    if (file_date(filepath, nullptr, &date, nullptr)) {
		
		if (date <= last_user_lib_write)
		    return;
	    }
	    else return;
	}
	else return;

	// Reset the last update to get the compilation output in the next frame
	last_update = 0.0;

	recive_user_callbacks(false);
    }

#endif

    /////////////////////////////////////////////////////////////////// PROCESS INPUT /////////////////////////////////////////////////////////////

    SV_AUX void process_input()
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

	_os_recive_input();

	input.mouse_dragged = input.mouse_position - input.mouse_last_pos;
    }
    
    //////////////////////////////////////////////////////////////////// ASSET FUNCTIONS //////////////////////////////////////////////////////////

    SV_INTERNAL bool create_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	image = nullptr;
	return true;
    }

    SV_INTERNAL bool load_image_asset(void* asset, const char* filepath)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);

	// Get file data
	void* data;
	u32 width;
	u32 height;
	if (!load_image(filepath, &data, &width, &height)) return false;

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

	bool res = graphics_image_create(&desc, &image);

	delete[] data;
	return res;
    }

    SV_INTERNAL bool destroy_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	graphics_destroy(image);
	image = nullptr;
	return true;
    }

    SV_INTERNAL bool reload_image_asset(void* asset, const char* filepath)
    {
	SV_CHECK(destroy_image_asset(asset));
	return load_image_asset(asset, filepath);
    }

    SV_INTERNAL bool create_mesh_asset(void* asset)
    {
	new(asset) Mesh();
	return true;
    }

    SV_INTERNAL bool load_mesh_asset(void* asset, const char* filepath)
    {
	Mesh& mesh = *new(asset) Mesh();
	SV_CHECK(load_mesh(filepath, mesh));
	SV_CHECK(mesh_create_buffers(mesh));
	return true;
    }

    SV_INTERNAL bool free_mesh_asset(void* asset)
    {
	Mesh& mesh = *reinterpret_cast<Mesh*>(asset);

	graphics_destroy(mesh.vbuffer);
	graphics_destroy(mesh.ibuffer);

	mesh.~Mesh();
	return true;
    }

    SV_INTERNAL bool create_material_asset(void* asset)
    {
	new(asset) Material();
	return true;
    }

    SV_INTERNAL bool load_material_asset(void* asset, const char* filepath)
    {
	Material& material = *new(asset) Material();
	SV_CHECK(load_material(filepath, material));
	return true;
    }

    SV_INTERNAL bool free_material_asset(void* asset)
    {
	Material& mat = *reinterpret_cast<Material*>(asset);
	mat.~Material();
	return true;
    }

    SV_AUX bool register_assets()
    {
	// Register assets
	
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

	SV_CHECK(register_asset_type(&desc));

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

	SV_CHECK(register_asset_type(&desc));

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

	SV_CHECK(register_asset_type(&desc));

	return true;
    }
    
    void engine_main()
    {
#if SV_DEV	
	_console_initialize();
#endif

	SV_LOG_INFO("Initializing %s", engine.name);

	if (!_event_initialize()) {
	    SV_LOG_ERROR("Can't initialize event system");
	    return;
	}
	
	if (_os_startup()) {
	    SV_LOG_INFO("OS layer initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize OS layer");
	    return;
	}

	// TODO initialize_audio());
	// TODO task_initialize();

	// Initialize Graphics API
	if (_graphics_initialize()) {
	    SV_LOG_INFO("Graphics API initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize graphics API");
	    return;
	}

	// Initialize Renderer
	if (_renderer_initialize()) {
	    SV_LOG_INFO("Renderer initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize the renderer");
	    return;
	}

	_initialize_scene();

	// Register components
	{
	    register_component<SpriteComponent>("Sprite");
	    register_component<CameraComponent>("Camera");
	    register_component<MeshComponent>("Mesh");
	    register_component<LightComponent>("Light");

	    register_component<BodyComponent>("Body");
	}

	if (!register_assets()) {
	    SV_LOG_ERROR("Can't register default assets");
	    return;
	}

#if SV_DEV
	_editor_initialize();
#endif
	
	engine.running = true;

	static f64 lastTime = 0.f;

	recive_user_callbacks(true);

	// User init
	if (!_user_initialize(true)) {
	    engine.running = false;
	}
	
	while (engine.running) {

	    // Calculate DeltaTime
	    f64 actualTime = timer_now();
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

#if SV_DEV
	    update_user_callbacks();
#endif
	    
	    process_input(); 

	    if (engine.close_request) {
		break;
	    }

	    _update_assets();
	    
	    _graphics_begin();
	    _renderer_begin();

	    _manage_scenes();
	    
#if SV_DEV
	    _console_update();
	    _editor_update();
	    
	    _update_scene();
#else
	    _update_scene();
#endif
    
	    
            // Draw scene
	    _draw_scene();
	    
	    // Draw editor and the console	    
#if SV_DEV
	    _editor_draw();
	    _console_draw();
#endif

	    _renderer_end();
	    _graphics_end();
	}

	SV_LOG_INFO("Closing %s", engine.name);

	// User close
	if (!_user_close(true)) {
	    
	    SV_LOG_ERROR("User can't close successfully");
	}
	
	_close_scene();

	free_unused_assets();

#if SV_DEV
        _editor_close();
#endif

	if (!_renderer_close()) { SV_LOG_ERROR("Can't close render utils"); }
	if (!_graphics_close()) { SV_LOG_ERROR("Can't close graphicsAPI"); }
	if (!_os_shutdown()) { SV_LOG_ERROR("Can't shutdown OS layer properly"); }
	_close_assets();
	// if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }

	_event_close();

#if SV_DEV
	_console_close();
#endif

    }

}
