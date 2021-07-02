#pragma once

#include "defines.h"
#include "utils/math.h"
#include "utils/allocators.h"
#include "utils/serialize.h"
#include "utils/string.h"

namespace sv {

    SV_API void print(const char* str);
    SV_API void printf(const char* str, ...);
    
    SV_API void show_message(const char* title, const char* content, bool error);
    SV_API bool show_dialog_yesno(const char* title, const char* content); // Returns true if yes

    SV_API bool file_dialog_open(char* buff, u32 filterCount, const char** filters, const char* startPath);
    SV_API bool file_dialog_save(char* buff, u32 filterCount, const char** filters, const char* startPath);

    //TODO void set_cursor_position(Window* window, f32 x, f32 y);

    void system_pause();

    // Timer

    struct Date {
		u32 year;
		u32 month;
		u32 day;
		u32 hour;
		u32 minute;
		u32 second;
		u32 milliseconds;

		SV_INLINE bool operator<=(const Date& other)
			{
				if (year != other.year) return year <= other.year;
				else if (month != other.month) return month <= other.month;
				else if (day != other.day) return day <= other.day;
				else if (hour != other.hour) return hour <= other.hour;
				else if (minute != other.minute) return minute <= other.minute;
				else if (second != other.second) return second <= other.second;
				return milliseconds <= other.milliseconds;
			}

		SV_INLINE bool operator!=(const Date& other)
			{
				return (year != other.year || month != other.month || day != other.day ||
						hour != other.hour || minute != other.minute || second != other.second ||
						milliseconds != other.milliseconds);
			}
    };

    SV_API f64 timer_now();
    SV_API Date timer_date();
    
    // Window

    enum WindowState : u32 {
		WindowState_Windowed,
		WindowState_Maximized,
		WindowState_Minimized,
		WindowState_Fullscreen,
    };

    typedef u64 LibraryHandle;
    
    SV_API u64 os_window_handle();
    SV_API v2_u32 os_window_size();
    SV_API f32 os_window_aspect();
    SV_API void os_window_set_fullscreen(bool fullscreen);
    SV_API WindowState os_window_state();
    SV_API v2_u32 os_desktop_size();

    SV_API LibraryHandle os_library_load(const char* filepath);
    SV_API void os_library_free(LibraryHandle lib);
    SV_API void* os_library_proc_address(LibraryHandle lib, const char* name);
    
    // File Management

    SV_API bool path_is_absolute(const char* path);
    SV_API void path_clear(char* path);

    SV_API bool file_read_binary(const char* filepath, u8** pData, size_t* pSize);
    SV_API bool file_read_binary(const char* filepath, RawList& data);
    SV_API bool file_read_text(const char* filepath, char** pstr, size_t* psize);
    SV_API bool file_read_text(const char* filepath, String& str);
    SV_API bool file_write_binary(const char* filepath, const u8* data, size_t size, bool append = false, bool recursive = true);
    SV_API bool file_write_text(const char* filepath, const char* str, size_t size, bool append = false, bool recursive = true);

    SV_API bool file_remove(const char* filepath);
    SV_API bool file_copy(const char* srcpath, const char* dstpath);
    SV_API bool file_exists(const char* filepath);
    SV_API bool folder_create(const char* filepath, bool recursive = false);
	SV_API bool folder_remove(const char* filepath);

    SV_API bool file_date(const char* filepath, Date* create, Date* last_write, Date* last_access);

    struct FolderIterator {
		u64 _handle;
    };

    struct FolderElement {
		bool is_file;
		Date create_date;
		Date last_write_date;
		Date last_access_date;
		char name[FILENAME_SIZE + 1u];
		const char* extension;
    };

    SV_API bool folder_iterator_begin(const char* folderpath, FolderIterator* iterator, FolderElement* element);
    SV_API bool folder_iterator_next(FolderIterator* iterator, FolderElement* element);
    SV_API void folder_iterator_close(FolderIterator* iterator);
    
    SV_API bool load_image(const char* filePath, void** pdata, u32* width, u32* height);

    // TODO: Move to utils/serialize.h
    SV_API bool bin_read(u64 hash, RawList& data, bool system = false);
    SV_API bool bin_read(u64 hash, Deserializer& deserializer, bool system = false); // Begins the deserializer

    SV_API bool bin_write(u64 hash, const void* data, size_t size, bool system = false);
    SV_API bool bin_write(u64 hash, Serializer& serializer, bool system = false); // Ends the serializer

    // MULTITHREADING STUFF

    struct Mutex { u64 _handle = 0u; };
    
    SV_API bool mutex_create(Mutex& mutex);
    SV_API void mutex_destroy(Mutex mutex);

    SV_API void mutex_lock(Mutex mutex);
    SV_API void mutex_unlock(Mutex mutex);

    SV_INLINE bool mutex_valid(Mutex mutex) { return mutex._handle != 0u; }

    struct _LockGuard {
		Mutex* m;
		_LockGuard(Mutex* mutex) : m(mutex) { mutex_lock(*m); }
		~_LockGuard() { mutex_unlock(*m); }
    };

#define SV_LOCK_GUARD(mutex, name) _LockGuard name(&mutex);

	// DYNAMIC LIBRARIES

	typedef u64 Library;

	Library library_load(const char* filepath);
	void    library_free(Library library);
	void*   library_address(Library library, const char* name);

    // INTERNAL

    bool _os_startup();
    void _os_recive_input();
    bool _os_shutdown();
    
}
