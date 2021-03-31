#include "os.h"

#define NOMINMAX

#include "Windows.h"

#undef near
#undef far

#include <iostream>

#include "engine.h"

namespace sv {

    struct GlobalPlatformData {
	HINSTANCE hinstance;
	HINSTANCE user_lib;
	HWND handle;
	v2_u32 size;
	v2_u32 position;
	bool resize;
	WindowState state;
    };

    GlobalPlatformData platform;

    SV_AUX Key wparam_to_key(WPARAM wParam)
    {
	Key key;

	if (wParam >= 'A' && wParam <= 'Z') {

	    key = Key(u32(Key_A) + u32(wParam) - 'A');
	}
	else if (wParam >= '0' && wParam <= '9') {

	    key = Key(u32(Key_Num0) + u32(wParam) - '0');
	}
	else if (wParam >= VK_F1 && wParam <= VK_F24) {

	    key = Key(u32(Key_F1) + u32(wParam) - VK_F1);
	}
	else {
	    switch (wParam)
	    {

	    case VK_INSERT:
		key = Key_Insert;
		break;

	    case VK_SPACE:
		key = Key_Space;
		break;

	    case VK_SHIFT:
		key = Key_Shift;
		break;

	    case VK_CONTROL:
		key = Key_Control;
		break;

	    case VK_ESCAPE:
		key = Key_Escape;
		break;

	    case VK_LEFT:
		key = Key_Left;
		break;
			
	    case VK_RIGHT:
		key = Key_Right;
		break;

	    case VK_UP:
		key = Key_Up;
		break;

	    case VK_DOWN:
		key = Key_Down;
		break;

	    case 13:
		key = Key_Enter;
		break;

	    case 8:
		key = Key_Delete;
		break;
			
	    case 46:
		key = Key_Supr;
		break;

	    case VK_TAB:
		key = Key_Tab;
		break;

	    case VK_CAPITAL:
		key = Key_Capital;
		break;

	    case VK_MENU:
		key = Key_Alt;
		break;

	    default:
		SV_LOG_WARNING("Unknown keycode: %u", (u32)wParam);
		key = Key_None;
		break;
	    }
	}

	return key;
    }
    
    LRESULT CALLBACK window_proc (
	    _In_ HWND   wnd,
	    _In_ UINT   msg,
	    _In_ WPARAM wParam,
	    _In_ LPARAM lParam
	)
    {
	LRESULT result = 0;
    
	switch (msg)
	{
    
	case WM_DESTROY:
	{
	    PostQuitMessage(0);
	    return 0;
	}
	break;
    
	case WM_CLOSE:
	{
	    engine.close_request = true;
	    return 0;
	}
	break;
	
	case WM_ACTIVATEAPP:
	{
	}
	break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
	    if (~lParam & (1 << 30)) {

		Key key = wparam_to_key(wParam);

		if (key != Key_None) {

		    input.keys[key] = InputState_Pressed;
		}
	    }

	    break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
	    Key key = wparam_to_key(wParam);

	    if (key != Key_None) {

		input.keys[key] = InputState_Released;
	    }

	    break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
	    MouseButton button = MouseButton_Left;

	    switch (msg)
	    {
	    case WM_RBUTTONDOWN:
	    case WM_RBUTTONUP:
		button = MouseButton_Right;
		break;

	    case WM_MBUTTONDOWN:
	    case WM_MBUTTONUP:
		button = MouseButton_Center;
		break;
	    }

	    switch (msg)
	    {
	    case WM_LBUTTONDOWN:
	    case WM_RBUTTONDOWN:
	    case WM_MBUTTONDOWN:
		input.mouse_buttons[button] = InputState_Pressed;
		break;

	    case WM_LBUTTONUP:
	    case WM_RBUTTONUP:
	    case WM_MBUTTONUP:
		input.mouse_buttons[button] = InputState_Released;
		break;
	    }
	}
	break;

	case WM_MOUSEMOVE:
	{
	    u16 _x = LOWORD(lParam);
	    u16 _y = HIWORD(lParam);

	    f32 w = f32(platform.size.x);
	    f32 h = f32(platform.size.y);

	    input.mouse_position.x = (f32(_x) / w) - 0.5f;
	    input.mouse_position.y = -(f32(_y) / h) + 0.5f;

	    break;
	}
	case WM_CHAR:
	{
	    switch (wParam)
	    {
	    case 0x08:
		input.text_commands.push_back(TextCommand_DeleteLeft);
		break;

	    case 0x0A:

		// Process a linefeed. 

		break;

	    case 0x1B:
		input.text_commands.push_back(TextCommand_Escape);
		break;

	    case 0x09:

		// TODO: Tabulations input
		input.text.push_back(' ');
		input.text.push_back(' ');
		input.text.push_back(' ');
		input.text.push_back(' ');

		break;

	    case 0x0D:
		input.text_commands.push_back(TextCommand_Enter);
		break;

	    default:
		input.text.push_back(char(wParam));
		break;
	    }

	    break;
	}

	case WM_SIZE:
	{
	    platform.size.x = LOWORD(lParam);
	    platform.size.y = HIWORD(lParam);

	    switch (wParam)
	    {
	    case SIZE_MAXIMIZED:
		platform.state = WindowState_Maximized;
		break;
	    case SIZE_MINIMIZED:
		platform.state = WindowState_Minimized;
		break;
	    default:
		platform.state = WindowState_Windowed;
		break;
	    }

	    platform.resize = true;

	    break;
	}
	case WM_MOVE:
	{
	    //wnd.bounds.x = LOWORD(lParam);
	    //wnd.bounds.y = HIWORD(lParam);

	    break;
	}
	case WM_MOUSEWHEEL:
	{
	    input.mouse_wheel = f32(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
	    break;
	}
		
	default:
	{
	    result = DefWindowProc(wnd, msg, wParam, lParam);
	}
	break;
    
	}
    
	return result;
    }

    void engine_main();

    extern "C" SV_API int entry_point(HINSTANCE hInstance)
    {
	{
	    WNDCLASSA c{};
	    c.hCursor = LoadCursorA(0, IDC_ARROW);
	    c.lpfnWndProc = window_proc;
	    c.lpszClassName = "SilverWindow";
	    c.hInstance = hInstance;
	    
	    if (!RegisterClassA(&c)) {
		sv::print("Can't register window class");
		return 1;
	    }
	}
	
	platform.hinstance = hInstance;
	platform.handle = 0;
	platform.resize = false;
	platform.state = WindowState_Windowed;

	engine_main();

	if (platform.user_lib)
	    FreeLibrary(platform.user_lib);

	return 0;
    }

    void print(const char* str)
    {
	printf(str);
	OutputDebugString(str);
    }
    
    void show_message(const char* title, const char* content, bool error)
    {
	MessageBox(0, content, title, MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
    }

    // Memory
    
    void* allocate_memory(size_t size)
    {
	void* ptr = nullptr;
	while (ptr == nullptr) ptr = malloc(size);
	return ptr;
    }
    
    void free_memory(void* ptr)
    {
	free(ptr);
    }

    // Window

    u64 os_window_handle()
    {
	return u64(platform.handle);
    }
    
    v2_u32 os_window_size()
    {
	return platform.size;
    }

    f32 os_window_aspect()
    {
	return f32(platform.size.x) / f32(platform.size.y);
    }

    void os_window_set_fullscreen(bool fullscreen)
    {
	if (fullscreen) {

	    if (platform.state == WindowState_Fullscreen)
		return;

	    platform.state = WindowState_Fullscreen;

	    DWORD style = WS_POPUP | WS_VISIBLE;
	    SetWindowLongPtrW(platform.handle, GWL_STYLE, (LONG_PTR)style);

	    v2_u32 size = os_desktop_size();

	    SetWindowPos(platform.handle, 0, 0, 0, size.x, size.y, 0);
	}
	else {

	    if (platform.state != WindowState_Fullscreen)
		return;
	    
	    DWORD style = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
	    SetWindowLongPtrW(platform.handle, GWL_STYLE, (LONG_PTR)style);
	}
    }
    
    WindowState os_window_state()
    {
	return platform.state;
    }

    v2_u32 os_desktop_size()
    {
	HWND desktop = GetDesktopWindow();
	RECT rect;
	GetWindowRect(desktop, &rect);
	return { u32(rect.right), u32(rect.bottom) };
    }

    // File Management

    bool path_is_absolute(const char* path)
    {
	SV_ASSERT(path);
	size_t size = strlen(path);
	if (size < 2u) return false;
	return path[1] == ':';
    }
    
    void path_clear(char* path)
    {
	while (*path != '\0') {

	    if (*path == '\\') {
		*path = '/';
	    }
	    
	    ++path;
	}
    }

    Result file_read_binary(const char* filepath, u8** pdata, size_t* psize)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return Result_NotFound;
	}

	DWORD size;
	size = GetFileSize(file, NULL);
	*psize = (size_t)size;

	if (*psize == 0u) return Result_UnknownError;
	
	*pdata = (u8*)allocate_memory(*psize);
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, (void*)*pdata, size, NULL, NULL);
	
	CloseHandle(file);
	return Result_Success;
    }

    Result file_read_binary(const char* filepath, List<u8>& data)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return Result_NotFound;
	}

	DWORD size;
	size = GetFileSize(file, NULL);

	if (size == 0u) return Result_UnknownError;
	data.resize(size_t(size));
	
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, data.data(), size, NULL, NULL);
	
	CloseHandle(file);
	return Result_Success;
    }
    
    Result file_read_text(const char* filepath, std::string& str)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return Result_NotFound;
	}

	DWORD size;
	size = GetFileSize(file, NULL);

	if (size == 0u) return Result_UnknownError;
	
	str.resize(size);
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, &str.front(), size, NULL, NULL);
	
	CloseHandle(file);
	return Result_Success;
    }
    
    Result file_write_binary(const char* filepath, const u8* data, size_t size, bool append)
    {
	HANDLE file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE) {
	    return Result_NotFound;
	}

	WriteFile(file, data, (DWORD)size, NULL, NULL);
	
	CloseHandle(file);
	return Result_Success;
    }
    
    Result file_write_text(const char* filepath, const char* str, size_t size, bool append)
    {
	HANDLE file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE) {
	    return Result_NotFound;
	}

	WriteFile(file, str, (DWORD)size, NULL, NULL);
	
	CloseHandle(file);
	return Result_Success;
    }

    Result file_remove(const char* filepath)
    {
	return DeleteFile(filepath) ? Result_Success : Result_NotFound;
    }
    
    Result file_copy(const char* srcpath, const char* dstpath)
    {
	return Result_TODO;
    }

    Result load_image(const char* filePath, void** pdata, u32* width, u32* height)
    {
	return Result_TODO;
    }

    constexpr u32 BIN_PATH_SIZE = 100u;
    
    SV_AUX void bin_filepath(char* buf, size_t hash)
    {
	sprintf(buf, "bin/%zu.bin", hash);
    }

    Result bin_read(size_t hash, List<u8>& data)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return file_read_binary(filepath, data);
    }
    
    Result bin_read(size_t hash, Archive& archive)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return archive.openFile(filepath);
    }

    Result bin_write(size_t hash, const void* data, size_t size)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return file_write_binary(filepath, (u8*)data, size);
    }
    
    Result bin_write(size_t hash, Archive& archive)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return archive.saveFile(filepath);
    }

    // INTERNAL

    void graphics_swapchain_resize();

    ////////////////////////////////////////////////////////////////// USER CALLBACKS /////////////////////////////////////////////////////////////

    void os_update_user_callbacks()
    {
#if SV_DEV

	static Time last_update = 0.0;
	Time now = timer_now();
	
	if (platform.user_lib) {

	    if (now - last_update > 1.0) {

		// Check if the file is modified

		FILETIME creation_time;
		FILETIME last_access_time;
		FILETIME last_write_time;

		HANDLE file = CreateFile("Game.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file != INVALID_HANDLE_VALUE) {

		    if (GetFileTime(file, &creation_time, &last_access_time, &last_write_time)) {

			SYSTEMTIME time;
			FileTimeToSystemTime(&last_write_time, &time);

			// TODO: check if it is modified and write a cross platform function
			bool modified = true;
			
			if (!modified) {
			    CloseHandle(file);
			    return;
			}
		    }
		    else {
			CloseHandle(file);
			SV_LOG_ERROR("Can't get the Game.dll file times");
			return;
		    }

		    CloseHandle(file);
		}
		else {
		    SV_LOG_ERROR("Game.dll not found");
		    return;
		}
	    }
	    else return;

	    FreeLibrary(platform.user_lib);
	    platform.user_lib = 0;
	}

	last_update = now;
	
	Result res = file_copy("Game.dll", "system/GameTemp.dll");
	if (result_fail) {
	    SV_LOG_ERROR("Can't create temporal game dll: %s", result_str(res));
	}
	
	platform.user_lib = LoadLibrary("system/GameTemp.dll");
#else
	platform.user_lib = LoadLibrary("Game.dll");
#endif

	if (platform.user_lib) {

	    engine.user.initialize = (UserInitializeFn)GetProcAddress(platform.user_lib, "user_initialize");
	    engine.user.update = (UserUpdateFn)GetProcAddress(platform.user_lib, "user_update");
	    engine.user.close = (UserCloseFn)GetProcAddress(platform.user_lib, "user_close");
	    engine.user.validate_scene = (UserValidateSceneFn)GetProcAddress(platform.user_lib, "user_validate_scene");
	    engine.user.get_scene_filepath = (UserGetSceneFilepathFn)GetProcAddress(platform.user_lib, "user_get_scene_filepath");
	    engine.user.initialize_scene = (UserInitializeSceneFn)GetProcAddress(platform.user_lib, "user_initialize_scene");
	    engine.user.serialize_scene = (UserSerializeSceneFn)GetProcAddress(platform.user_lib, "user_serialize_scene");
	}
	else SV_LOG_ERROR("Can't find game code");
    }
    
    Result os_create_window()
    {
	platform.handle = CreateWindowExA(0u,
				   "SilverWindow",
				   "SilverEngine",
				   WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
				   0, 0, 1080, 720,
				   0, 0, 0, 0
	    );

	if (platform.handle == 0) {
	    return Result_PlatformError;
	}
	
	return Result_Success;
    }
    
    void os_recive_input()
    {
	MSG msg;
	
	while (PeekMessageA(&msg, 0, 0u, 0u, PM_REMOVE) > 0) {
	    TranslateMessage(&msg);
	    DispatchMessageW(&msg);
	}

	if (platform.resize) {
	    platform.resize = false;

	    graphics_swapchain_resize();
	}
    }
    
    Result os_destroy_window()
    {
	if (platform.handle)
	    return DestroyWindow(platform.handle) ? Result_Success : Result_PlatformError;
	return Result_Success;
    }
    
}
