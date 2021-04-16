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
	std::cout << str;
	OutputDebugString(str);
    }

    void printf(const char* str, ...)
    {
	va_list args;
	va_start(args, str);

	char log_buffer[1000];

	vsnprintf(log_buffer, 1000, str, args);
	sv::print(log_buffer);
	
	va_end(args);
    }
    
    void show_message(const char* title, const char* content, bool error)
    {
	MessageBox(0, content, title, MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
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

    bool file_read_binary(const char* filepath, u8** pdata, size_t* psize)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return false;
	}

	DWORD size;
	size = GetFileSize(file, NULL);
	*psize = (size_t)size;

	if (*psize == 0u) return false;
	
	*pdata = (u8*)allocate_memory(*psize);
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, (void*)*pdata, size, NULL, NULL);
	
	CloseHandle(file);
	return true;
    }

    bool file_read_binary(const char* filepath, List<u8>& data)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return false;
	}

	DWORD size;
	size = GetFileSize(file, NULL);

	if (size == 0u) return false;
	data.resize(size_t(size));
	
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, data.data(), size, NULL, NULL);
	
	CloseHandle(file);
	return true;
    }
    
    bool file_read_text(const char* filepath, char** pstr, size_t* psize)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
	    return false;
	}

	DWORD size;
	size = GetFileSize(file, NULL);

	if (size == 0u) return false;
	
	*psize = (size_t)size;
	*pstr = (char*)allocate_memory(size);
	SetFilePointer(file, NULL, NULL, FILE_BEGIN);
	ReadFile(file, *pstr, size, NULL, NULL);
	
	CloseHandle(file);
	return true;
    }

    SV_AUX bool create_path(const char* filepath)
    {
	char folder[FILEPATH_SIZE] = "\0";

	size_t file_size = strlen(filepath);
		
	while (true) {

	    size_t folder_size = strlen(folder);

	    const char* it = filepath + folder_size;
	    while (*it && *it != '/') ++it;

	    if (*it == '\0') break;
	    else ++it;

	    if (*it == '\0') break;

	    folder_size = it - filepath;
	    memcpy(folder, filepath, folder_size);
	    folder[folder_size] = '\0';

	    if (!CreateDirectory(folder, NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
		return false;
	    }
	}
    }
    
    bool file_write_binary(const char* filepath, const u8* data, size_t size, bool append, bool recursive)
    {
	HANDLE file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE) {
	    
	    if (recursive) {
		
		if (!create_path(filepath)) return false;

		file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) return false;
	    }
	    else return false;
	}

	WriteFile(file, data, (DWORD)size, NULL, NULL);
	
	CloseHandle(file);
	return true;
    }
    
    bool file_write_text(const char* filepath, const char* str, size_t size, bool append, bool recursive)
    {
	HANDLE file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE) {
	    if (recursive) {
		
		if (!create_path(filepath)) return false;

		file = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, append ? CREATE_NEW : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) return false;
	    }
	    else return false;
	}

	WriteFile(file, str, (DWORD)size, NULL, NULL);
	
	CloseHandle(file);
	return true;
    }

    SV_AUX Date filetime_to_date(const FILETIME& file)
    {
	SYSTEMTIME sys;
	FileTimeToSystemTime(&file, &sys);

	Date date;
	date.year = (u32)sys.wYear;
	date.month = (u32)sys.wMonth;
	date.day = (u32)sys.wDay;
	date.hour = (u32)sys.wHour;
	date.minute = (u32)sys.wMinute;
	date.second = (u32)sys.wSecond;
	date.milliseconds = (u32)sys.wMilliseconds;

	return date;
    }

    bool file_date(const char* filepath, Date* create, Date* last_write, Date* last_access)
    {
	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
	    return false;

	FILETIME creation_time;
	FILETIME last_access_time;
	FILETIME last_write_time;
	
	if (GetFileTime(file, &creation_time, &last_access_time, &last_write_time)) {

	    if (create) *create = filetime_to_date(creation_time);
	    if (last_access) *last_access = filetime_to_date(last_access_time);
	    if (last_write) *last_write = filetime_to_date(last_write_time);
	}
	else {
	    CloseHandle(file);
	    return false;
	}

	CloseHandle(file);
	return true;
    }

    bool file_remove(const char* filepath)
    {
	return DeleteFile(filepath);
    }
    
    bool file_copy(const char* srcpath, const char* dstpath)
    {
	return CopyFileA(srcpath, dstpath, FALSE);
    }

    bool file_exists(const char* filepath)
    {
	DWORD att = GetFileAttributes(filepath);
	if(INVALID_FILE_ATTRIBUTES == att && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
	    return false;
	}
	return true;
    }

    SV_AUX FolderElement finddata_to_folderelement(const WIN32_FIND_DATAA& d)
    {
	FolderElement e;
	e.is_file = !(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	e.create_date = filetime_to_date(d.ftCreationTime);
	e.last_write_date = filetime_to_date(d.ftLastWriteTime);
	e.last_access_date = filetime_to_date(d.ftLastAccessTime);
	sprintf(e.name, "%s", (const char*)d.cFileName);
	size_t size = strlen(e.name);
	if (size == 0u) e.extension = nullptr;
	else {
	    const char* begin = e.name;
	    const char* it = begin + size;

	    while (it != begin && *it != '.') {
		--it;
	    }

	    if (it == begin) {
		e.extension = nullptr;
	    }
	    else {
		SV_ASSERT(*it == '.');
		e.extension = it + 1u;
	    }
	}
	return e;
    }

    bool folder_iterator_begin(const char* folderpath_, FolderIterator* iterator, FolderElement* element)
    {
	WIN32_FIND_DATAA data;

	// Clear path
	char folderpath[MAX_PATH];
	
	const char* it = folderpath_;
	char* it0 = folderpath;

	*it0++ = '.';
	*it0++ = '\\';
	
	while (*it != '\0') {
	    if (*it == '/') *it0 = '\\';
	    else *it0 = *it;
	    
	    ++it;
	    ++it0;
	}

	if (*(it0 - 1u) != '\\')
	    *it0++ = '\\';
	*it0++ = '*';
	*it0++ = '\0';
	

	HANDLE find = FindFirstFileA(folderpath, &data);
	
	if (find == INVALID_HANDLE_VALUE) return false;

	*element = finddata_to_folderelement(data);
	iterator->_handle = (u64)find;

	return true;
    }
    
    bool folder_iterator_next(FolderIterator* iterator, FolderElement* element)
    {
	HANDLE find = (HANDLE)iterator->_handle;
	
	WIN32_FIND_DATAA data;
	if (FindNextFileA(find, &data)) {

	    *element = finddata_to_folderelement(data);
	    return true;
	}

	return false;
    }

    void folder_iterator_close(FolderIterator* iterator)
    {
	HANDLE find = (HANDLE)iterator->_handle;
	FindClose(find);
    }

    constexpr u32 BIN_PATH_SIZE = 100u;
    
    SV_AUX void bin_filepath(char* buf, size_t hash)
    {
	sprintf(buf, "bin/%zu.bin", hash);
    }

    bool bin_read(size_t hash, List<u8>& data)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return file_read_binary(filepath, data);
    }
    
    bool bin_read(size_t hash, Archive& archive)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return archive.openFile(filepath);
    }

    bool bin_write(size_t hash, const void* data, size_t size)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return file_write_binary(filepath, (u8*)data, size);
    }
    
    bool bin_write(size_t hash, Archive& archive)
    {
	char filepath[BIN_PATH_SIZE];
	bin_filepath(filepath, hash);
	return archive.saveFile(filepath);
    }

    // INTERNAL

    void graphics_swapchain_resize();

    void os_compile_gamecode()
    {
	static bool shell = false;

	if (!shell) {
	    shell = true;
	    ShellExecuteA(NULL, "open", "system\\shell.bat", NULL, NULL, SW_HIDE);
	}
	
	ShellExecuteA(NULL, "open", "system\\build_game.bat", NULL, NULL, SW_HIDE);
    }

    ////////////////////////////////////////////////////////////////// USER CALLBACKS /////////////////////////////////////////////////////////////

    void os_free_user_callbacks()
    {
	engine.user = {};

	if (platform.user_lib) {
	    FreeLibrary(platform.user_lib);
	    platform.user_lib = 0;
	}
    }
    
    void os_update_user_callbacks(const char* dll)
    {
	if (platform.user_lib) {
	    FreeLibrary(platform.user_lib);
	    platform.user_lib = 0;
	}

	platform.user_lib = LoadLibrary(dll);
	
	if (platform.user_lib) {

	    engine.user.initialize = (UserInitializeFn)GetProcAddress(platform.user_lib, "user_initialize");
	    engine.user.update = (UserUpdateFn)GetProcAddress(platform.user_lib, "user_update");
	    engine.user.close = (UserCloseFn)GetProcAddress(platform.user_lib, "user_close");
	    engine.user.validate_scene = (UserValidateSceneFn)GetProcAddress(platform.user_lib, "user_validate_scene");
	    engine.user.get_scene_filepath = (UserGetSceneFilepathFn)GetProcAddress(platform.user_lib, "user_get_scene_filepath");
	    engine.user.initialize_scene = (UserInitializeSceneFn)GetProcAddress(platform.user_lib, "user_initialize_scene");
	    engine.user.close_scene = (UserCloseSceneFn)GetProcAddress(platform.user_lib, "user_close_scene");
	    engine.user.serialize_scene = (UserSerializeSceneFn)GetProcAddress(platform.user_lib, "user_serialize_scene");

	    SV_LOG_INFO("User callbacks loaded");
	}
	else SV_LOG_ERROR("Can't find game code");
    }

    // Memory
    
    void* os_allocate_memory(size_t size)
    {
	void* ptr = nullptr;
	while (ptr == nullptr) ptr = malloc(size);
	return ptr;
    }
    
    void os_free_memory(void* ptr)
    {
	free(ptr);
    }
    
    bool os_startup()
    {
	platform.handle = CreateWindowExA(0u,
				   "SilverWindow",
				   "SilverEngine",
				   WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
				   0, 0, 1080, 720,
				   0, 0, 0, 0
	    );

	if (platform.handle == 0) {
	    return false;
	}
	
	return true;
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
    
    bool os_shutdown()
    {
	if (platform.handle)
	    return DestroyWindow(platform.handle);
	return true;
    }
    
}
