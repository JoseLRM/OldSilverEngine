#pragma once

#ifdef SV_RES_PATH
#define SV_RES_PATH_W L"" SV_RES_PATH
#endif

// std includes
#include <math.h>
#include <DirectXMath.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <array>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <filesystem>
#include <fstream>

using namespace DirectX;

// types
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef float	f32;
typedef double	f64;

typedef wchar_t	wchar;

typedef void* WindowHandle;

typedef int		BOOL;
#define FALSE	0
#define TRUE	1

// limits

constexpr u8	u8_max	= std::numeric_limits<u8>::max();
constexpr u16	u16_max	= std::numeric_limits<u16>::max();
constexpr u32	u32_max	= std::numeric_limits<u32>::max();
constexpr u64	u64_max	= std::numeric_limits<u64>::max();

constexpr i8	i8_min		= std::numeric_limits<i8>::min();
constexpr i16	i16_min		= std::numeric_limits<i16>::min();
constexpr i32	i32_min		= std::numeric_limits<i32>::min();
constexpr i64	i64_min		= std::numeric_limits<i64>::min();

constexpr i8	i8_max = std::numeric_limits<i8>::max();
constexpr i16	i16_max = std::numeric_limits<i16>::max();
constexpr i32	i32_max = std::numeric_limits<i32>::max();
constexpr i64	i64_max = std::numeric_limits<i64>::max();

constexpr f32	f32_min = std::numeric_limits<f32>::min();
constexpr f64	f64_min = std::numeric_limits<f64>::min();

constexpr f32	f32_max = std::numeric_limits<f32>::max();
constexpr f64	f64_max = std::numeric_limits<f64>::max();

static_assert(std::numeric_limits<f32>::is_iec559, "IEEE 754 required");
constexpr f32	f32_inf = std::numeric_limits<f32>::infinity();
constexpr f32	f32_ninf = -1.f * std::numeric_limits<f32>::infinity();
static_assert(std::numeric_limits<f64>::is_iec559, "IEEE 754 required");
constexpr f64	f64_inf = std::numeric_limits<f64>::infinity();
constexpr f64	f64_ninf = -1.0 * std::numeric_limits<f64>::infinity();

// Misc

#define svCheck(x) do{ sv::Result __res__ = (x); if(sv::result_fail(__res__)) return __res__; }while(0)
#define SV_ZERO_MEMORY(dest, size) memset(dest, 0, size)
#define SV_BIT(x) (1ULL << x) 
#define foreach(_it, _end) for (u32 _it = 0u; _it < u32(_end); ++_it)

#ifdef SV_PLATFORM_WIN
#	define SV_INLINE __forceinline
#else
#	define SV_INLINE inline
#endif

#ifdef SV_SYS_PATH
#define SV_SYS(filepath) "$" filepath
#else
#define SV_SYS(filepath) filepath
#endif

// Result

namespace sv {

	enum Result : u32 {
		Result_Success,
		Result_CloseRequest,
		Result_TODO,
		Result_UnknownError,
		Result_PlatformError,
		Result_GraphicsAPIError,
		Result_CompileError,
		Result_NotFound = 404u,
		Result_InvalidFormat,
		Result_InvalidUsage,
		Result_Unsupported,
		Result_UnsupportedVersion,
		Result_Duplicated,
	};

	SV_INLINE bool result_okay(Result res) { return res == Result_Success; }
	SV_INLINE bool result_fail(Result res) { return res != Result_Success; }
	SV_INLINE const char* result_str(Result res)
	{
		switch (res)
		{
		case sv::Result_Success:
			return "Success";
		case sv::Result_CloseRequest:
			return "Close request";
		case sv::Result_TODO:
			return "TODO";
		case sv::Result_UnknownError:
			return "Unknown error";
		case sv::Result_PlatformError:
			return "Platform error";
		case sv::Result_GraphicsAPIError:
			return "Graphics API error";
		case sv::Result_CompileError:
			return "Compile error";
		case sv::Result_NotFound:
			return "Not found";
		case sv::Result_InvalidFormat:
			return "Invalid format";
		case sv::Result_InvalidUsage:
			return "Invalid usage";
		case sv::Result_Unsupported:
			return "Unsupported";
		case sv::Result_UnsupportedVersion:
			return "Unsupported version";
		case sv::Result_Duplicated:
			return "Duplicated";
		default:
			return "Unknown result...";
		}
	}

}

// Ptr Handle
#define SV_DEFINE_HANDLE(x) struct x { x() = delete; ~x() = delete; }

// Assertion

namespace sv {
	void throw_assertion(const char* content, u32 line, const char* file);
}

#ifdef SV_ENABLE_ASSERTION
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif

// Console

namespace sv {
	
	typedef Result(*CommandFn)(const char** args, u32 argc);

	Result execute_command(const char* command);
	Result register_command(const char* name, CommandFn command_fn);

	void console_print(const char* str, ...);
	void console_notify(const char* title, const char* str, ...);
	void console_clear();

}

#define SV_LOG_CLEAR() sv::__internal__do_not_call_this_please_or_you_will_die__console_clear()
#define SV_LOG_SEPARATOR() sv::console_print("----------------------------------------------------\n")
#define SV_LOG(x, ...) sv::console_print(x, __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::console_notify("INFO", x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::console_notify("WARNING", x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::console_notify("ERROR", x, __VA_ARGS__)

// Debug profiler

#ifdef SV_ENABLE_PROFILER

#define SV_PROFILER_SCALAR float

namespace sv {

	void				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(const char* name);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(const char* name);
	float				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(const char* name);

	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(const char* name, SV_PROFILER_SCALAR value);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(const char* name, SV_PROFILER_SCALAR value);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(const char* name, SV_PROFILER_SCALAR value);
	SV_PROFILER_SCALAR	__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(const char* name);

}

#define SV_PROFILER(content) content

#define SV_PROFILER_CHRONO_BEGIN(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(x)
#define SV_PROFILER_CHRONO_END(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(x)
#define SV_PROFILER_CHRONO_GET(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(x)

// TODO
#ifdef SV_ENABLE_LOGGING
#define SV_PROFILER_CHRONO_LOG(x) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(5u, "[PROFILER] %s: %fms", x, sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(x))
#else
#define SV_PROFILER_CHRONO_LOG(x)
#endif

#define SV_PROFILER_SCALAR_SET(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(x, value)
#define SV_PROFILER_SCALAR_ADD(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(x, value)
#define SV_PROFILER_SCALAR_SUB(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(x, -(value))
#define SV_PROFILER_SCALAR_MUL(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(x, value)
#define SV_PROFILER_SCALAR_DIV(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(x, 1.0/(value))
#define SV_PROFILER_SCALAR_GET(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(x)

#else

#define SV_PROFILER(content) do{}while(false)

#define SV_PROFILER_CHRONO_BEGIN(x) do{}while(false)
#define SV_PROFILER_CHRONO_END(x) do{}while(false)
#define SV_PROFILER_CHRONO_GET(x) 0.0
#define SV_PROFILER_CHRONO_LOG(x) do{}while(false)

#define SV_PROFILER_SCALAR_SET(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_ADD(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_SUB(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_MUL(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_DIV(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_GET(x) 0.0

#endif


#include "utils/math.h"
#include "window.h"

// Platform

namespace sv {

	enum MessageStyle : u32 {
		MessageStyle_None = 0u,

		MessageStyle_IconInfo = SV_BIT(0),
		MessageStyle_IconWarning = SV_BIT(1),
		MessageStyle_IconError = SV_BIT(2),

		MessageStyle_Ok = SV_BIT(3),
		MessageStyle_OkCancel = SV_BIT(4),
		MessageStyle_YesNo = SV_BIT(5),
		MessageStyle_YesNoCancel = SV_BIT(6),
	};
	typedef u32 MessageStyleFlags;

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style = MessageStyle_None);

	std::string file_dialog_open(u32 filterCount, const char** filters, const char* startPath);
	std::string file_dialog_save(u32 filterCount, const char** filters, const char* startPath);

	void set_cursor_position(Window* window, f32 x, f32 y);

	void system_pause();

	// Hash functions

	template<typename T>
	constexpr void hash_combine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	SV_INLINE size_t hash_string(const char* str)
	{
		// TEMPORAL
		size_t res = 0u;
		size_t strLength = strlen(str);
		hash_combine(res, strLength);
		while (*str != '\0') {
			hash_combine(res, (const size_t)* str);
			++str;
		}
		return res;
	}

	// Timer

	class Time {
		f64 m_Time;
	public:
		constexpr Time() : m_Time(0.0) {}
		constexpr Time(f64 t) : m_Time(t) {}

		SV_INLINE operator f64() const noexcept { return m_Time; }

		SV_INLINE Time timeSince(Time time) const noexcept { return Time(time.m_Time - m_Time); }

		SV_INLINE f64 toSeconds_f64() const noexcept { return m_Time; }
		SV_INLINE u32 toSeconds_u32() const noexcept { return u32(m_Time); }
		SV_INLINE u64 toSeconds_u64() const noexcept { return u64(m_Time); }
		SV_INLINE f64 toMillis_f64() const noexcept { return m_Time * 1000.0; }
		SV_INLINE u32 toMillis_u32() const noexcept { return u32(m_Time * 1000.0); }
		SV_INLINE u64 toMillis_u64() const noexcept { return u64(m_Time * 1000.0); }
	};

	struct Date {
		u32 year;
		u32 month;
		u32 day;
		u32 hour;
		u32 minute;
		u32 second;
	};

	Time timer_now();
	Date timer_date();

	// Version

	struct Version {
		u32 major = 0u;
		u32 minor = 0u;
		u32 revision = 0u;

		constexpr Version() = default;
		constexpr Version(u32 major, u32 minor, u32 revision)
			: major(major), minor(minor), revision(revision) {}

		std::string toString() const noexcept {
			std::stringstream ss;
			ss << major << '.' << minor << '.' << revision;
			return ss.str();
		}
		u64 getVersion() const noexcept
		{
			return u64(major) * 1000000L + u64(minor) * 1000L + u64(revision);
		}

		bool operator== (const Version& other) const noexcept
		{
			return major == other.major && minor == other.minor && revision == other.revision;
		}
		bool operator!= (const Version& other) const noexcept
		{
			return !this->operator==(other);
		}
		bool operator< (const Version& other) const noexcept
		{
			if (major != other.major) return major < other.major;
			else if (minor != other.minor) return minor < other.minor;
			else return revision < other.revision;
		}
		bool operator> (const Version& other) const noexcept
		{
			if (major != other.major) return major > other.major;
			else if (minor != other.minor) return minor > other.minor;
			else return revision > other.revision;
		}
		bool operator<= (const Version& other) const noexcept {
			return this->operator<(other) || this->operator==(other);
		}
		bool operator>= (const Version& other) const noexcept {
			return this->operator>(other) || this->operator==(other);
		}

	};

	// File Management

#if defined(SV_RES_PATH) && defined(SV_SYS_PATH)

#define SV_PARSE_FILEPATH() std::string filepath_str; \
	if (!sv::path_is_absolute(filepath)) { \
		if (*filepath == '$') { \
			filepath_str = SV_SYS_PATH; ++filepath; \
		} \
		else \
			filepath_str = SV_RES_PATH; filepath_str += filepath; \
		filepath = filepath_str.c_str(); \
	} 

#elif defined(SV_SYS_PATH) 

#define SV_PARSE_FILEPATH() std::string filepath_str; \
	if (!sv::path_is_absolute(filepath)) { \
		if (filepath[0] == '$') { \
			filepath_str = SV_SYS_PATH; filepath_str += filepath + 1u; \
			filepath = filepath_str.c_str(); \
		} \
	} 

#elif defined(SV_RES_PATH)

#define SV_PARSE_FILEPATH() std::string filepath_str; \
	if (!sv::path_is_absolute(filepath)) { \
		filepath_str = SV_RES_PATH; filepath_str += filepath; \
		filepath = filepath_str.c_str(); \
	} 

#else

#define SV_PARSE_FILEPATH()

#endif

	bool path_is_absolute(const char* path);
	void path_clear(char* path);

	bool path_is_absolute(const wchar* path);

	Result file_read_binary(const char* filePath, u8** pData, size_t* pSize);
	Result file_read_binary(const char* filePath, std::vector<u8>& data);
	Result file_read_text(const char* filePath, std::string& str);
	Result file_write_binary(const char* filePath, const u8* data, size_t size, bool append = false);
	Result file_write_text(const char* filePath, const char* str, size_t size, bool append = false);

	Result file_remove(const char* filePath);

	struct FileO {

		Result	open(const char* filePath, bool text = false, bool append = false);
		void	close();

		bool isOpen();

		void write(const u8* data, size_t size);
		void writeLine(const char* str);
		void writeLine(const std::string& str);

	private:
		std::ofstream stream;
	};

	struct FileI {

		Result	open(const char* filePath, bool text = false);
		void	close();

		bool isOpen();

		void	setPos(size_t pos);
		size_t	getPos();

		size_t getSize();

		void read(u8* data, size_t size);
		bool readLine(char* buf, size_t bufSize);
		bool readLine(std::string& str);

	private:
		std::ifstream stream;
	};

	class ArchiveO {

		size_t m_Size;
		size_t m_Capacity;
		u8* m_Data;

	public:
		ArchiveO();
		~ArchiveO();

		void reserve(size_t size);
		void write(const void* data, size_t size);
		void erase(size_t size);
		void clear();

		Result save_file(const char* filePath, bool append = false);

		template<typename T>
		inline ArchiveO& operator<<(const T& t)
		{
			write(&t, sizeof(T));
			return *this;
		}

		template<typename T>
		inline ArchiveO& operator<<(const std::vector<T>& vec)
		{
			size_t size = vec.size();
			write(&size, sizeof(size_t));

			for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
				this->operator<<(*it);
			}
			return *this;
		}

		inline ArchiveO& operator<<(const char* txt)
		{
			size_t txtLength = strlen(txt);
			write(&txtLength, sizeof(size_t));
			write(txt, txtLength);
			return *this;
		}

		inline ArchiveO& operator<<(const wchar* txt)
		{
			size_t txtLength = wcslen(txt);
			write(&txtLength, sizeof(size_t));
			write(txt, txtLength * sizeof(wchar));
			return *this;
		}

		inline ArchiveO& operator<<(const std::string& str)
		{
			size_t txtLength = str.size();
			write(&txtLength, sizeof(size_t));
			write(str.data(), txtLength);
			return *this;
		}

		inline ArchiveO& operator<<(const std::wstring& str)
		{
			size_t txtLength = str.size();
			write(&txtLength, sizeof(size_t));
			write(str.data(), txtLength * sizeof(wchar));
			return *this;
		}

	private:
		void allocate(size_t size);
		void free();

	};

	class ArchiveI {
		u8* m_Data;
		size_t m_Size;
		size_t m_Pos;

	public:
		ArchiveI();
		~ArchiveI();

		Result open_file(const char* filePath);

		void read(void* data, size_t size);

		inline size_t pos_get() const noexcept { return m_Pos; }
		inline void pos_set(size_t pos) noexcept { m_Pos = pos; }
		inline size_t size() const noexcept { return m_Size; }

		void clear();

		template<typename T>
		ArchiveI& operator>>(T& t)
		{
			read(&t, sizeof(T));
			return *this;
		}

		template<typename T>
		ArchiveI& operator>>(std::vector<T>& vec)
		{
			size_t size;
			read(&size, sizeof(size_t));
			size_t lastSize = vec.size();
			vec.resize(lastSize + size);
			read(vec.data() + lastSize, sizeof(T) * size);
			return *this;
		}

		ArchiveI& operator>>(std::string& str)
		{
			size_t txtLength;
			read(&txtLength, sizeof(size_t));
			str.resize(txtLength);
			read(str.data(), txtLength);
			return *this;
		}

		ArchiveI& operator>>(std::wstring& str)
		{
			size_t txtLength;
			read(&txtLength, sizeof(size_t));
			str.resize(txtLength);
			read(str.data(), txtLength * sizeof(wchar));
			return *this;
		}

	};

	Result load_image(const char* filePath, void** pdata, u32* width, u32* height);

	Result bin_read(size_t hash, std::vector<u8>& data);
	Result bin_read(size_t hash, ArchiveI& archive);

	Result bin_write(size_t hash, const void* data, size_t size);
	Result bin_write(size_t hash, ArchiveO& archive);

	// Character utils

	std::wstring parse_wstring(const char* c);
	std::string  parse_string(const wchar* c);

	constexpr bool char_is_letter(char c) 
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	constexpr bool char_is_number(char c)
	{
		return c >= '0' && c <= '9';
	}

	// Input

	enum MouseButton : u32 {
		MouseButton_Left,
		MouseButton_Right,
		MouseButton_Center,

		MouseButton_MaxEnum,
		MouseButton_None,
	};

	enum Key : u32 {
		Key_Tab,
		Key_Shift,
		Key_Control,
		Key_Capital,
		Key_Escape,
		Key_Alt,
		Key_Space,
		Key_Left,
		Key_Up,
		Key_Right,
		Key_Down,
		Key_Enter,
		Key_Insert,
		Key_Delete,
		Key_Supr,
		Key_A,
		Key_B,
		Key_C,
		Key_D,
		Key_E,
		Key_F,
		Key_G,
		Key_H,
		Key_I,
		Key_J,
		Key_K,
		Key_L,
		Key_M,
		Key_N,
		Key_O,
		Key_P,
		Key_Q,
		Key_R,
		Key_S,
		Key_T,
		Key_U,
		Key_V,
		Key_W,
		Key_Z,
		Key_Num0,
		Key_Num1,
		Key_Num2,
		Key_Num3,
		Key_Num4,
		Key_Num5,
		Key_Num6,
		Key_Num7,
		Key_Num8,
		Key_Num9,
		Key_F1,
		Key_F2,
		Key_F3,
		Key_F4,
		Key_F5,
		Key_F6,
		Key_F7,
		Key_F8,
		Key_F9,
		Key_F10,
		Key_F11,
		Key_F12,
		Key_F13,
		Key_F14,
		Key_F15,
		Key_F16,
		Key_F17,
		Key_F18,
		Key_F19,
		Key_F20,
		Key_F21,
		Key_F22,
		Key_F23,
		Key_F24,

		Key_MaxEnum,
		Key_None,
	};

	enum InputState : u8 {
		InputState_None,
		InputState_Pressed,
		InputState_Hold,
		InputState_Released,
	};

	enum TextCommand : u32 {
		TextCommand_Null,
		TextCommand_DeleteLeft,
		TextCommand_DeleteRight,
		TextCommand_Enter,
		TextCommand_Escape,
	};

	// Globals

	struct Scene;
	struct BaseComponent;
	struct IGUI;

	typedef u16 CompID;
	typedef u32 Entity;

	struct ApplicationCallbacks {
		Result(*initialize)();
		void(*update)();
		void(*render)();
		Result(*close)();
		Result(*initialize_scene)(Scene* scene);
		void(*show_component)(IGUI* igui, CompID comp_id, BaseComponent* comp);
		Result(*close_scene)(Scene* scene);
	};

	struct GlobalEngineData {

		const Version			version = { 0, 0, 0 };
		const std::string		name = "SilverEngine 0.0.0";
		f32						deltatime = 0.f;
		u64						frame_count = 0U;
		u32						FPS = 0u;
		ApplicationCallbacks	app_callbacks;
		bool					close_request = false;
		bool					able_to_run = false;
		Window*					window = nullptr;
		Scene*					scene = nullptr;
		std::string				next_scene_name;
		bool					console_active = false;
	};

	struct GlobalInputData {

		InputState keys[Key_MaxEnum];
		InputState mouse_buttons[MouseButton_MaxEnum];

		std::string					text;
		std::vector<TextCommand>	text_commands;

		v2_f32	mouse_position;
		v2_f32	mouse_last_pos;
		v2_f32	mouse_dragged;
		f32		mouse_wheel;

		Window* focused_window = nullptr;

	};

	extern GlobalInputData	input;
	extern GlobalEngineData engine;

}
