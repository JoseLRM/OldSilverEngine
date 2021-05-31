#include "platform/os.h"

#define NOMINMAX
#include "Windows.h"

#undef near
#undef far

#include <iostream>

#include "user.h"
#include "core/engine.h"

namespace sv {

    struct GlobalPlatformData {
		HINSTANCE hinstance;
		HINSTANCE user_lib;
		HWND handle;
		v2_u32 size;
		v2_u32 position;
		bool resize;
		WindowState state;

		bool resize_request;
		v2_u32 new_size;
		v2_u32 new_position;
		DWORD new_style;
		v2_u32 old_size; // Size before fullscreen
		v2_u32 old_position; // Position before fullscreen
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
				string_append(input.text, "    ", INPUT_TEXT_SIZE + 1u);
				break;

			case 0x0D:
				input.text_commands.push_back(TextCommand_Enter);
				break;

			default:
				string_append(input.text, char(wParam), INPUT_TEXT_SIZE + 1u);
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
			}

			platform.resize = true;

			break;
		}
		case WM_MOVE:
		{
			platform.position.x = LOWORD(lParam);
			platform.position.y = HIWORD(lParam);

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

}


// TODO: if SV_DEV is disabled use WinMain
int main(int argc, char* argv[])
{
    using namespace sv;
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
	
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
    platform.resize_request = false;
    platform.state = WindowState_Windowed;

    engine_main();

    if (platform.user_lib)
		FreeLibrary(platform.user_lib);

    return 0;
}

namespace sv {

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

    bool show_dialog_yesno(const char* title, const char* content)
    {
		int res = MessageBox(0, content, title, MB_YESNO | MB_ICONQUESTION);

		return res == IDYES;
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
			v2_u32 size = os_desktop_size();

			platform.new_size = size;
			platform.new_position = { 0u, 0u };
			platform.new_style = style;
			platform.old_size = platform.size;
			platform.old_position = platform.position;
			platform.resize_request = true;
		}
		else {

			if (platform.state != WindowState_Fullscreen)
				return;

			platform.state = WindowState_Windowed;
	    
			DWORD style = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

			platform.new_size = platform.old_size;
			platform.new_position = platform.old_position;
			platform.new_style = style;
			platform.resize_request = true;
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

    LibraryHandle os_library_load(const char* filepath_)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		LibraryHandle lib = (LibraryHandle)LoadLibrary(filepath);
		return lib;
    }
    
    void os_library_free(LibraryHandle lib)
    {
		if (lib) {

			FreeLibrary((HINSTANCE)lib);
		}
    }
    
    void* os_library_proc_address(LibraryHandle lib, const char* name)
    {
		return (void*) GetProcAddress((HINSTANCE)lib, name);
    }

    // File Management

    SV_AUX bool file_dialog(char* buff, u32 filterCount, const char** filters, const char* filepath_, bool open, bool is_file)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	    
		// TODO: Not use classes, this should be in c in the future
		RawList abs_filter;

		for (u32 i = 0; i < filterCount; ++i) {
			abs_filter.write_back(filters[i * 2u], strlen(filters[i * 2u]));
			char c = '\0';
			abs_filter.write_back(&c, sizeof(char));

			abs_filter.write_back(filters[i * 2u + 1u], strlen(filters[i * 2u + 1u]));
			abs_filter.write_back(&c, sizeof(char));
		}

		OPENFILENAMEA file;
		SV_ZERO_MEMORY(&file, sizeof(OPENFILENAMEA));

		file.lStructSize = sizeof(OPENFILENAMEA);
		file.hwndOwner = platform.handle;
		file.lpstrFilter = (char*)abs_filter.data();
		file.nFilterIndex = 1u;
		file.lpstrFile = buff;
		file.lpstrInitialDir = filepath;
		file.nMaxFile = MAX_PATH;
		file.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;

		BOOL result;

		if (open) result = GetOpenFileNameA(&file);
		else result = GetSaveFileNameA(&file);

		if (result == TRUE) {
			path_clear(buff);
			return true;
		}
		else return false;
    }

    bool file_dialog_open(char* buff, u32 filterCount, const char** filters, const char* startPath)
    {
		return file_dialog(buff, filterCount, filters, startPath, true, true);
    }
    
    bool file_dialog_save(char* buff, u32 filterCount, const char** filters, const char* startPath)
    {
		return file_dialog(buff, filterCount, filters, startPath, false, true);
    }
    
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

    bool file_read_binary(const char* filepath_, u8** pdata, size_t* psize)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE) {
			return false;
		}

		DWORD size;
		size = GetFileSize(file, NULL);
		*psize = (size_t)size;

		if (*psize == 0u) return false;
	
		*pdata = (u8*)SV_ALLOCATE_MEMORY(*psize);
		SetFilePointer(file, NULL, NULL, FILE_BEGIN);
		ReadFile(file, (void*)*pdata, size, NULL, NULL);
	
		CloseHandle(file);
		return true;
    }

    bool file_read_binary(const char* filepath_, RawList& data)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
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
    
    bool file_read_text(const char* filepath_, char** pstr, size_t* psize)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE) {
			return false;
		}

		DWORD size;
		size = GetFileSize(file, NULL);

		if (size == 0u) return false;
	
		*psize = (size_t)size;
		*pstr = (char*)SV_ALLOCATE_MEMORY(size + 1u);
		SetFilePointer(file, NULL, NULL, FILE_BEGIN);
		ReadFile(file, *pstr, size, NULL, NULL);
		(*pstr)[*psize] = '\0';
	
		CloseHandle(file);
		return true;
    }

    bool file_read_text(const char* filepath_, String& str)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		char* buff;
		size_t size;
		if (file_read_text(filepath, &buff, &size)) {

			str.set_buffer_ptr(buff, size);
	    
			return true;
		}
		return false;
    }

    SV_AUX bool create_path(const char* filepath)
    {	
		char folder[FILEPATH_SIZE] = "\0";
		
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

			if (folder_size == 3u && folder[1u] == ':')
				continue;

			if (!CreateDirectory(folder, NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
				return false;
			}
		}

		return true;
    }
    
    bool file_write_binary(const char* filepath_, const u8* data, size_t size, bool append, bool recursive)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
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
    
    bool file_write_text(const char* filepath_, const char* str, size_t size, bool append, bool recursive)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
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

    bool file_date(const char* filepath_, Date* create, Date* last_write, Date* last_access)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
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

    bool file_remove(const char* filepath_)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		return DeleteFile(filepath);
    }
    
    bool file_copy(const char* srcpath_, const char* dstpath_)
    {
		char srcpath[MAX_PATH];
		char dstpath[MAX_PATH];
		filepath_resolve(srcpath, srcpath_);
		filepath_resolve(dstpath, dstpath_);
	
		if (!CopyFileA(srcpath, dstpath, FALSE)) {

			create_path(dstpath);
			return CopyFileA(srcpath, dstpath, FALSE);
		}
	
		return true;
    }

    bool file_exists(const char* filepath_)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);
	
		DWORD att = GetFileAttributes(filepath);
		if(INVALID_FILE_ATTRIBUTES == att)
		{
			return false;
		}
		return true;
    }

    bool folder_create(const char* filepath_, bool recursive)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);

		if (recursive) create_path(filepath);
		return CreateDirectory(filepath, NULL);
    }

	bool folder_remove(const char* filepath_)
    {
		char filepath[MAX_PATH];
		filepath_resolve(filepath, filepath_);

		return RemoveDirectoryA(filepath);
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

    bool folder_iterator_begin(const char* folderpath__, FolderIterator* iterator, FolderElement* element)
    {
		WIN32_FIND_DATAA data;

		char folderpath_[MAX_PATH];
		filepath_resolve(folderpath_, folderpath__);

		// Clear path
		char folderpath[MAX_PATH];
	
		const char* it = folderpath_;
		char* it0 = folderpath;

		if (!path_is_absolute(folderpath_)) {
			*it0++ = '.';
			*it0++ = '\\';
		}
	
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
    
    SV_AUX void bin_filepath(char* buf, u64 hash, bool system)
    {
		if (system)
			sprintf(buf, "$system/cache/%zu.bin", hash);
		else
			sprintf(buf, "bin/%zu.bin", hash);
    }

    bool bin_read(u64 hash, RawList& data, bool system)
    {
		char filepath[BIN_PATH_SIZE + 1u];
		bin_filepath(filepath, hash, system);
		return file_read_binary(filepath, data);
    }
    
    bool bin_read(u64 hash, Deserializer& deserializer, bool system)
    {
		char filepath[BIN_PATH_SIZE + 1u];
		bin_filepath(filepath, hash, system);
		return deserialize_begin(deserializer, filepath);
    }

    bool bin_write(u64 hash, const void* data, size_t size, bool system)
    {
		char filepath[BIN_PATH_SIZE + 1u];
		bin_filepath(filepath, hash, system);
		return file_write_binary(filepath, (u8*)data, size);
    }
    
    bool bin_write(u64 hash, Serializer& serializer, bool system)
    {
		char filepath[BIN_PATH_SIZE + 1u];
		bin_filepath(filepath, hash, system);
		return serialize_end(serializer, filepath);
    }

    //////////////////////////////// MULTITHREADING ////////////////////////////

    bool mutex_create(Mutex& mutex)
    {
		mutex._handle = (u64)CreateMutexA(NULL, false, NULL);
		return mutex._handle != NULL;
    }
    
    void mutex_destroy(Mutex mutex)
    {
		if (mutex._handle != NULL) {
			CloseHandle((HANDLE)mutex._handle);
		}
    }

    void mutex_lock(Mutex mutex)
    {
		SV_ASSERT(mutex._handle != 0u);
		WaitForSingleObject((HANDLE)mutex._handle, INFINITE);
    }
    
    void mutex_unlock(Mutex mutex)
    {
		SV_ASSERT(mutex._handle != 0u);
		ReleaseMutex((HANDLE)mutex._handle);
    }

    // INTERNAL

    void graphics_swapchain_resize();

    ////////////////////////////////////////////////////////////////// USER CALLBACKS /////////////////////////////////////////////////////////////

    void _os_free_user_callbacks()
    {
		_user_callbacks_set({});

		if (platform.user_lib) {
			FreeLibrary(platform.user_lib);
			platform.user_lib = 0;
		}
    }
    
    bool _os_update_user_callbacks(const char* dll_)
    {
		char dll[MAX_PATH];
		filepath_resolve(dll, dll_);
	
		_os_free_user_callbacks();

		platform.user_lib = LoadLibrary(dll);
	
		if (platform.user_lib) {

			UserCallbacks c = {};

			c.initialize = (UserInitializeFn)GetProcAddress(platform.user_lib, "user_initialize");
			c.close = (UserCloseFn)GetProcAddress(platform.user_lib, "user_close");
			c.validate_scene = (UserValidateSceneFn)GetProcAddress(platform.user_lib, "user_validate_scene");
			c.get_scene_filepath = (UserGetSceneFilepathFn)GetProcAddress(platform.user_lib, "user_get_scene_filepath");

			if (c.initialize == nullptr) {
				SV_LOG_ERROR("Can't load game code: Initialize function not found");
				return false;
			}
			if (c.close == nullptr) {
				SV_LOG_ERROR("Can't load game code: Close function not found");
				return false;
			}
	    
			_user_callbacks_set(c);

			return true;
		}
		else SV_LOG_ERROR("Can't find game code");
		return false;
    }

    // Memory
    
    void* _allocate_memory(size_t size)
    {
		void* ptr = nullptr;
		while (ptr == nullptr) ptr = malloc(size);
		memset(ptr, 0, size);
		return ptr;
    }
    
    void _free_memory(void* ptr)
    {
		free(ptr);
    }

#if SV_DEV

    void _os_compile_gamecode()
    {
		char project_path[MAX_PATH + 100];
		strcpy(project_path, engine.project_path);

		foreach (i, strlen(project_path)) {

			char& c = project_path[i];
			if (c == '/')
				c = '\\';
		}

#if SV_SLOW
		strcat(project_path, " shell");
#endif
	
		ShellExecute(NULL, "open", "system\\build_game.bat", project_path, NULL, SW_HIDE);
    }

#endif
    
    bool _os_startup()
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
    
    void _os_recive_input()
    {
		if (platform.resize_request) {

			platform.resize_request = false;
			SetWindowLongPtrW(platform.handle, GWL_STYLE, (LONG_PTR)platform.new_style);
			SetWindowPos(platform.handle, 0, platform.new_position.x, platform.new_position.y, platform.new_size.x, platform.new_size.y, 0);
		}
	
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
    
    bool _os_shutdown()
    {
		if (platform.handle)
			return DestroyWindow(platform.handle);
		return true;
    }
    
}
