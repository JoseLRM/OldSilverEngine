#include "core.h"
#include "os.h"

namespace sv {

    void throw_assertion(const char* content, u32 line, const char* file)
    {
#if SV_DEV
	console_notify("ASSERTION", "'%s', file: '%s', line: %u", content, file, line);	
#endif
	
	char text[500];
	memset(text, 0, 500);

	snprintf(text, 500, "'%s', file: '%s', line: %u", content, file, line);

	sv::show_message("ASSERTION!!", text, true);
    }

    ///////////////////////////////////////////////// MEMORY /////////////////////////////////////////////////

    void* os_allocate_memory(size_t size);
    void os_free_memory(void* ptr);
    
    void* allocate_memory(size_t size)
    {
	return os_allocate_memory(size);
    }
    
    void free_memory(void* ptr)
    {
	return os_free_memory(ptr);
    }
    
    ///////////////////////////////////////////////// TIMER /////////////////////////////////////////////////

    static std::chrono::steady_clock::time_point g_InitialTime = std::chrono::steady_clock::now();

    static std::chrono::steady_clock::time_point timer_now_chrono()
    {
	return std::chrono::steady_clock::now();
    }

    f64 timer_now()
    {
	return f64(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
    }

    Date timer_date()
    {
	__time64_t t;
	_time64(&t);
	tm time;
	_localtime64_s(&time, &t);

	Date date;
	date.year = time.tm_year + 1900;
	date.month = time.tm_mon + 1;
	date.day = time.tm_mday;
	date.hour = time.tm_hour;
	date.minute = time.tm_min;
	date.second = time.tm_sec;

	return date;
    }

    ///////////////////////////////////////////////// ARCHIVE /////////////////////////////////////////////////

    Archive::Archive() : _capacity(0u), _size(0u), _data(nullptr), _pos(0u)
    {}

    Archive::~Archive()
    {
	free();
    }

    void Archive::reserve(size_t size)
    {
	if (_size + size > _capacity) {

	    size_t newSize = size_t(double(_size + size) * 1.5);

	    bool write_version = false;

	    if (_size == 0u) {
		newSize += sizeof(Version);
		write_version = true;
	    }

	    allocate(newSize);

	    if (write_version) {
		memcpy(_data, &engine.version, sizeof(Version));
		_size = sizeof(Version);
	    }
	}
    }

    void Archive::write(const void* data, size_t size)
    {
	reserve(size);

	memcpy(_data + _size, data, size);
	_size += size;
    }

    void Archive::read(void* data, size_t size)
    {
	if (_pos + size > _size) {
	    size_t invalidSize = (_pos + size) - _size;
	    SV_ZERO_MEMORY((u8*)data + size - invalidSize, invalidSize);
	    size -= invalidSize;
	    SV_LOG_WARNING("Archive reading, out of bounds");
	}
	memcpy(data, _data + _pos, size);
	_pos += size;
    }

    void Archive::erase(size_t size)
    {
	SV_ASSERT(size <= _size);
	_size -= size;
    }

    bool Archive::openFile(const char* filePath)
    {
	_pos = 0u;
	SV_CHECK(file_read_binary(filePath, &_data, &_size));
	if (_size < sizeof(Version)) return false;
	this->operator>>(version);
	return true;
    }

    bool Archive::saveFile(const char* filePath, bool append)
    {
	return file_write_binary(filePath, _data, _size, append);
    }

    void Archive::allocate(size_t size)
    {
	u8* newData = (u8*)operator new(size);

	if (_data) {
	    memcpy(newData, _data, _size);
	    operator delete[](_data);
	}

	_data = newData;
	_capacity = size;
    }

    void Archive::free()
    {
	if (_data) {
	    operator delete[](_data);
	    _data = nullptr;
	    _size = 0u;
	    _capacity = 0u;
	    _pos = 0u;
	}
    }

    bool read_var_file(const char* filepath, List<Var>& vars)
    {
	char* text;
	size_t size;
	
	if (file_read_text(filepath, &text, &size)) {

	    const char* it = text;
	    u32 line_count = 0u;

	    while (true) {

		const char* line = it;
		Var var;

		while (*it != ' ' || *it != '\n' || *it != '\0' || *it != '=') {
		    ++it;
		}

		if (*it == '\n' || *it == '\0') {
		    SV_LOG_ERROR("Value not found at line %u", line_count);
		}		
		else {
		    size_t name_size = it - line >= VARNAME_SIZE;

		    if (name_size >= VARNAME_SIZE) {
			SV_LOG_ERROR("The value name at line %u is too large, the limit is %u chars", line_count, VARNAME_SIZE);
		    }
		    else if (name_size != 0u) {

			memcpy(var.name, line, name_size);
			var.name[name_size] = '\0';

			//const char* value = it;
			++it;
			while (*it != '\n' || *it != '\0') {
			    ++it;
			}
			
		    }
		}

		if (*it == '\0')
		    break;
		else ++it;
	    }
	    
	    free_memory(text);
	    return true;
	}
	return false;
    }
    
    // TEMP
    XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat)
    {
	XMVECTOR direction, pos, target;
		
	direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	pos = position.getDX();

	direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(directionQuat.get_dx()));

	target = XMVectorAdd(pos, direction);
	return XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
    }
}
