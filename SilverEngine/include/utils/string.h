#pragma once

namespace sv {

    struct String {

	SV_INLINE void set_buffer_ptr(char* ptr, size_t size)
	    {
		_buff.set_ptr(ptr, size);
		
	    }

	SV_INLINE void reset()
	    {
		_buff.reset();
	    }

	SV_INLINE void set(const char* str)
	    {
		set(str, 0u, strlen(str));
	    }
	
	SV_INLINE void set(const char* str, size_t str_offset, size_t str_size)
	    {
		reset();
		append(str, str_offset, str_size);
	    }

	SV_INLINE void append(const char* str, size_t str_offset, size_t str_size)
	    {
		if (_buff.size() && *(_buff.data() + _buff.size() - 1u) == '\0') {
		    _buff.pop_back(1u);
		}
		_buff.write_back(str + str_offset, str_size);

		char c = '\0';
		_buff.write_back(&c, sizeof(char));
	    }

	SV_INLINE const char* c_str() const
	    {
		return (const char*)_buff.data();
	    }

	SV_INLINE char* c_str()
	    {
		return (char*)_buff.data();
	    }

	SV_INLINE size_t size() const
	    {
		size_t size = _buff.size();
		if (size && *(_buff.data() + _buff.size() - 1u) == '\0')
		    --size;
		return size;
	    }

	SV_INLINE bool empty() { return size() == 0u; } const

	SV_INLINE void clear()
	    {
		_buff.clear();
	    }
	
	RawList _buff;
    };

    SV_INLINE size_t string_split(const char* line, char* delimiters, u32 count)
    {
	const char* it = line;
	while (*it != '\0') {

	    bool end = false;

	    foreach(i, count)
		if (delimiters[i] == *it) {
		    end = true;
		    break;
		}

	    if (end) break;
	    
	    ++it;
	}

	size_t size = it - line;
	
	return size;
    }

    SV_INLINE const char* filepath_name(const char* filepath)
    {
	size_t s = strlen(filepath);

	if (s) {

	    --s;
	    while (s && filepath[s] != '/') --s;

	    if (s) {
		return filepath + s + 1u;
	    }
	}
	
	return filepath;
    }

    SV_INLINE char* filepath_name(char* filepath)
    {
	size_t s = strlen(filepath);

	if (s) {

	    --s;
	    while (s && filepath[s] != '/') --s;

	    if (s) {
		return filepath + s + 1u;
	    }
	}
	
	return filepath;
    }

    SV_INLINE char* filepath_extension(char* filepath)
    {
	char* last_dot = nullptr;

	char* it = filepath;

	while (*it) {

	    switch (*it) {

	    case '/':
		last_dot = nullptr;
		break;

	    case '.':
		last_dot = it;
		break;
		
	    }
	    
	    ++it;
	}

	if (last_dot) {

	    if (last_dot == filepath)
		last_dot = nullptr;
	    
	    else if (*(last_dot - 1u) == '/')
		last_dot = nullptr;
	}

	return last_dot;
    }

    constexpr const char* filepath_extension(const char* filepath)
    {
	const char* last_dot = nullptr;

	const char* it = filepath;

	while (*it) {

	    switch (*it) {

	    case '/':
		last_dot = nullptr;
		break;

	    case '.':
		last_dot = it;
		break;
		
	    }
	    
	    ++it;
	}

	if (last_dot) {

	    if (last_dot == filepath)
		last_dot = nullptr;
	    
	    else if (*(last_dot - 1u) == '/')
		last_dot = nullptr;
	}

	return last_dot;
    }
    
    // Character utils
    
    constexpr bool char_is_letter(char c) 
    {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    
    constexpr bool char_is_number(char c)
    {
	return c >= '0' && c <= '9';
    }

    // Line processing

    struct LineProcessor {
	String line_buff;
	const char* str;
	size_t offset;
	u32 line_count;
	const char* line;
    };
    
    SV_INLINE void line_begin(LineProcessor& processor, const char* str)
    {
	processor.str = str;
	processor.offset = 0u;
	processor.line_count = 0u;
    }

    SV_INLINE bool line_next(LineProcessor& processor)
    {
	processor.offset += processor.line_buff.size();

	char del = '\n';
	size_t size = string_split(processor.str + processor.offset, &del, 1u);
	if (*(processor.str + processor.offset + size) != '\0')
	    ++size;

	if (size) {

	    processor.line_buff.reset();
	    processor.line_buff.append(processor.str, processor.offset, size);
	    processor.line = processor.line_buff.c_str();
	    ++processor.line_count;
	    return true;
	}
	else {
	    processor.line_buff.clear();
	    processor.line = nullptr;
	    return false;
	}
    }

    SV_INLINE void line_jump_spaces(const char*& line)
    {
	while (*line == ' ') ++line;
    }

    SV_INLINE bool line_read_f32(const char*& line, f32& value)
    {
	value = 0.f;
	line_jump_spaces(line);

	const char* start = line;

	if (*line == '-') ++line;

	u32 dots = 0u;
	
	while (*line != '\0' && *line != ' ' && *line != '\n' && *line != '\r') {
	    if (!char_is_number(*line) && *line != '.')
		return false;

	    if (*line == '.')
		++dots;
	    
	    ++line;
	}

	if (line == start || dots > 1) return false;
	
	size_t size = line - start;	

	char value_str[20u];
	memcpy(value_str, start, size);
	value_str[size] = '\0';
	value = (f32)atof(value_str);

	return true;
    }

    SV_INLINE bool line_read_i32(const char*& line, i32& value, char* delimiters, u32 delimiter_count)
    {
	value = 0;
	line_jump_spaces(line);

	const char* start = line;
	
	if (*line == '-') ++line;
	
	while (*line != '\0' && *line != '\n' && *line != '\r') {

	    bool end = false;

	    foreach(i, delimiter_count)
		if (delimiters[i] == *line) end = true;

	    if (end)
		break;
	    
	    if (!char_is_number(*line))
		return false;
	    ++line;
	}

	if (line == start) return false;
	
	size_t size = line - start;	

	char value_str[20u];
	memcpy(value_str, start, size);
	value_str[size] = '\0';
	value = atoi(value_str);

	return true;
    }

    SV_INLINE bool line_read_v3_f32(const char*& line, v3_f32& value)
    {
	bool res = line_read_f32(line, value.x);
	if (res) res = line_read_f32(line, value.y);
	if (res) res = line_read_f32(line, value.z);
	return res;
    }
    
}
