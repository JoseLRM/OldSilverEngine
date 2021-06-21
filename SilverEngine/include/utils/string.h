#pragma once

#include "platform/input.h"

namespace sv {

    struct String {

		SV_INLINE void set_buffer_ptr(char* ptr, size_t size) {
			_buff.set_ptr(ptr, size);	
		}

		SV_INLINE void reset() {
			_buff.reset();
		}

		SV_INLINE void set(const char* str) {
			set(str, 0u, strlen(str));
		}
	
		SV_INLINE void set(const char* str, size_t str_offset, size_t str_size) {
			reset();
			append(str, str_offset, str_size);
		}

		SV_INLINE void append(char c) {
			append(&c, 0u, 1u);
		}
		SV_INLINE void append(const char* str) {

			append(str, 0u, strlen(str));
		}

		SV_INLINE void append(const char* str, size_t str_offset, size_t str_size) {
			if (_buff.size() && *(_buff.data() + _buff.size() - 1u) == '\0') {
				_buff.pop_back(1u);
			}
			_buff.write_back(str + str_offset, str_size);
			
			char c = '\0';
			_buff.write_back(&c, sizeof(char));
		}

		SV_INLINE const char* c_str() const {
			return (const char*)_buff.data();
		}

		SV_INLINE char* c_str() {
			return (char*)_buff.data();
		}

		SV_INLINE size_t size() const {
			size_t size = _buff.size();
			if (size && *(_buff.data() + _buff.size() - 1u) == '\0')
				--size;
			return size;
		}
		
		SV_INLINE bool empty() { return size() == 0u; } const
		
		SV_INLINE void clear() {
			_buff.clear();
		}
	
		RawList _buff;
    };

	inline const char* string_validate(const char* str)
	{
		return str ? str : "";
	}

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

    constexpr size_t string_size(const char* str)
    {
		size_t size = 0u;
		while (*str++) ++size;
		return size;
    }

	constexpr bool string_equals(const char* s0, const char* s1)
    {
		while (*s0 && *s1) {
			if (*s0 != *s1)
				return false;

			++s0;
			++s1;
		}

		return *s0 == *s1;
    }

    SV_INLINE size_t string_append(char* dst, const char* src, size_t buff_size)
    {
		size_t src_size = string_size(src);
		size_t dst_size = string_size(dst);

		size_t new_size = src_size + dst_size;

		size_t overflows = (buff_size < (new_size + 1u)) ? (new_size + 1u) - buff_size : 0u;

		size_t append_size = (overflows > src_size) ? 0u : (src_size - overflows);

		memcpy(dst + dst_size, src, append_size);
		new_size = dst_size + append_size;
		dst[new_size] = '\0';
	
		return overflows;
    }

	SV_INLINE size_t string_append(char* dst, char c, size_t buff_size)
	{
		// TODO: Optimize
		char src[2];
		src[0] = c;
		src[1] = '\0';
		return string_append(dst, src, buff_size);
	}

    SV_INLINE void string_erase(char* str, size_t index)
    {
		size_t size = string_size(str);

		char* it = str + index + 1u;
		char* end = str + size;

		while (it < end) {

			*(it - 1u) = *it;
			++it;
		}
		*(end - 1u) = '\0';
    }

    SV_INLINE size_t string_copy(char* dst, const char* src, size_t buff_size)
    {
		size_t src_size = string_size(src);

		size_t size = SV_MIN(buff_size - 1u, src_size);
		memcpy(dst, src, size);
		dst[size] = '\0';
		return (src_size > buff_size - 1u) ? (src_size - buff_size - 1u) : 0u;
    }

    SV_INLINE size_t string_insert(char* dst, const char* src, size_t index, size_t buff_size)
    {
		if (buff_size <= index)
			return 0u;
	
		size_t src_size = string_size(src);
		size_t dst_size = string_size(dst);

		index = SV_MIN(dst_size, index);

		size_t moved_index = index + src_size;
		if (moved_index < buff_size - 1u) {

			size_t move_size = SV_MIN(dst_size - index, buff_size - moved_index - 1u);

			char* end = dst + moved_index - 1u;
			char* it0 = dst + moved_index + move_size;
			char* it1 = dst + index + move_size;

			while (it0 != end) {

				*it0 = *it1;
				--it1;
				--it0;
			}
		}

		size_t src_cpy = SV_MIN(src_size, buff_size - 1u - index);
		memcpy(dst + index, src, src_cpy);

		size_t final_size = SV_MIN(buff_size - 1u, src_size + dst_size);
		dst[final_size] = '\0';

		if (src_size + dst_size > buff_size - 1u) {
			return (src_size - dst_size) - (buff_size - 1u);
		}
		return 0u;
    }

	SV_INLINE bool string_modify(char* dst, size_t buff_size, u32& cursor, bool* _enter)
	{
		bool modified = false;
		bool enter = false;
		
		if (buff_size >= 2u) {
		
			size_t max_line_size = buff_size - 1u;
		
			if (input.text[0] && cursor < max_line_size) {

				size_t current_size = string_size(dst);
				size_t append_size = string_size(input.text);
			
				if (current_size + append_size > max_line_size)
					append_size = max_line_size - current_size;

				if (append_size) {

					size_t overflow = string_insert(dst, input.text, cursor, buff_size);
	    
					cursor += u32(string_size(input.text) - overflow);
					modified = true;
				}
			}

			foreach(i, input.text_commands.size()) {

				TextCommand txtcmd = input.text_commands[i];

				switch (txtcmd)
				{
				case TextCommand_DeleteLeft:
					if (cursor) {

						--cursor;
						string_erase(dst, cursor);
					}
					break;
				case TextCommand_DeleteRight:
				{
					size_t size = strlen(dst);
					if (cursor < size) {
		    
						string_erase(dst, cursor);
					}
				}
				break;
		
				case TextCommand_Enter:
				{
					enter = true;
				}
				break;

				case TextCommand_Escape:
					break;
				}
			}

			if (input.keys[Key_Left] == InputState_Pressed) cursor = (cursor == 0u) ? cursor : (cursor - 1u);
			if (input.keys[Key_Right] == InputState_Pressed) cursor = (cursor == strlen(dst)) ? cursor : (cursor + 1u);
		}

		if (_enter) *_enter = enter;

		return modified;
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

	SV_INLINE bool line_read_f64(const char*& line, f64& value)
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

		char value_str[50u];
		memcpy(value_str, start, size);
		value_str[size] = '\0';
		char* si;
		value = (f64)strtod(value_str, &si);

		return true;
    }

    SV_INLINE bool line_read_i32(const char*& line, i32& value, const char* delimiters, u32 delimiter_count)
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

	SV_INLINE bool line_read_i64(const char*& line, i64& value, const char* delimiters, u32 delimiter_count)
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

		char value_str[50u];
		memcpy(value_str, start, size);
		value_str[size] = '\0';
		value = atol(value_str);

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
