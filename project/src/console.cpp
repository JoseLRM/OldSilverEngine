#include "SilverEngine/core.h"

#include "SilverEngine/scene.h"
#include "renderer/renderer_internal.h"

#include <stdarg.h>

namespace sv {

	static std::unordered_map<std::string, CommandFn> commands;
	static std::string current_command;
	static u32 cursor_pos = 0u;
	static f32 text_offset = 0.f;
	static u32 line_count = 0u;
	static bool scroll_selected = false;
	
	static constexpr u32 CONSOLE_SIZE = 10000u;
	static constexpr f32 CONSOLE_HEIGHT = 1.7f;
	static constexpr u32 LINE_COUNT = 35u;
	static constexpr f32 TEXT_SIZE = CONSOLE_HEIGHT / f32(LINE_COUNT);
	static constexpr f32 COMMAND_TEXT_SIZE = 0.06f;
	static constexpr f32 SCROLL_WIDTH = 0.03f;
	static constexpr f32 SCROLL_HEIGHT = 0.12f;

	struct ConsoleBuffer {
		char* buff;
		size_t pos;
		bool flipped;
	};

	static ConsoleBuffer console_buffer;

	static Result command_exit(const char** args, u32 argc)
	{
		engine.console_active = false;
		return Result_Success;
	}

	static Result command_close(const char** args, u32 argc)
	{
		engine.close_request = true;
		return Result_Success;
	}

	static Result command_test(const char** args, u32 argc)
	{
		if (argc == 0u)
		foreach(i, 100) {
			console_print("Hola que tal, esto es un test %u\n", i);
		}

		else
		foreach(i, argc) {
			console_notify("ARG", args[i]);
		}

		return Result_Success;
	}

	static Result command_clear(const char** args, u32 argc)
	{
		console_clear();
		return Result_Success;
	}

	void initialize_console()
	{
		console_buffer.buff = (char*)malloc(CONSOLE_SIZE);
		console_buffer.pos = 0U;
		console_buffer.flipped = false;

		// Register default commands
		register_command("exit", command_exit);
		register_command("close", command_close);
		register_command("test", command_test);
		register_command("clear", command_clear);
	}

	void close_console()
	{
		if (console_buffer.buff) {

			free(console_buffer.buff);
			console_buffer.buff = nullptr;
		}

		console_buffer.pos = 0U;
		console_buffer.flipped = false;
	}

	Result execute_command(const char* command)
	{
		if (command == nullptr) {
			SV_LOG_ERROR("Null ptr command, what are you doing?");
			return Result_InvalidUsage;
		}

		while (*command == ' ') ++command;

		if (*command == '\0') {
			SV_LOG_ERROR("This is an empty command string");
			return Result_InvalidUsage;
		}

		const char* name = command;

		while (*command != ' ' && *command != '\0') ++command;

		size_t command_size = command - name;

		std::string command_name;
		command_name.resize(command_size);
		memcpy(command_name.data(), name, command_size);

		auto it = commands.find(command_name);
		if (it == commands.end()) {
			SV_LOG_ERROR("Command '%s' not found", command_name.c_str());
			return Result_NotFound;
		}

		std::vector<char*> args;

		while (*command == ' ') ++command;

		while (*command != '\0') {

			name = command;

			while (*command != '\0' && *command != ' ') {

				++command;
			}

			size_t arg_size = command - name;
			char*& arg = args.emplace_back();
			arg = (char*)malloc(arg_size + 1u);
			memcpy(arg, name, arg_size);
			arg[arg_size] = '\0';

			while (*command == ' ') ++command;
		}

		Result res = it->second((const char**)args.data(), u32(args.size()));

		// Free
		for (char* arg : args)
			free(arg);

		return res;
	}

	Result register_command(const char* name, CommandFn command_fn)
	{
		if (command_fn == nullptr || name == nullptr || *name == '\0') return Result_InvalidUsage;

		auto it = commands.find(name);
		if (it != commands.end()) return Result_Duplicated;
		
		commands[name] = command_fn;
		return Result_Success;
	}

	static std::string date_string(const Date& date)
	{
		std::stringstream stream;
		stream << '[';
		if (date.hour < 10u) stream << '0';
		stream << date.hour << ':';
		if (date.minute < 10u) stream << '0';
		stream << date.minute << ':';
		if (date.second < 10u) stream << '0';
		stream << date.second << ']';
		std::string res = stream.str();
		return res;
	}

	static void _write(const char* str, size_t size)
	{
		if (size == 0u) return;

		size_t capacity = CONSOLE_SIZE - console_buffer.pos;

		size_t rest;
		size_t write;

		if (size > capacity) {

			rest = size - capacity;
			write = capacity;
		}
		else {
			rest = 0u;
			write = size;
		}

		memcpy(console_buffer.buff + console_buffer.pos, str, write);

		if (rest) {

			console_buffer.flipped = true;
			console_buffer.pos = 0u;
			_write(str + write, rest);
		}
		else console_buffer.pos += write;
	}

	void console_print(const char* str, ...)
	{
		text_offset = 0.f;

		va_list args;
		va_start(args, str);

		char log_buffer[1000];

		vsnprintf(log_buffer, 1000, str, args);

		_write(log_buffer, strlen(log_buffer));
		printf(log_buffer);

		va_end(args);
	}

	void console_notify(const char* title, const char* str, ...)
	{
		text_offset = 0.f;

		va_list args;
		va_start(args, str);

		size_t offset = 0u;
		char log_buffer[1000];

		// Title
		log_buffer[offset++] = '[';

		while (*title != '\0') log_buffer[offset++] = *title++;

		log_buffer[offset++] = ']';
		log_buffer[offset++] = ' ';

		vsnprintf(log_buffer + offset, 1000 - offset - 1u, str, args);

		size_t end_pos = strlen(log_buffer);
		log_buffer[end_pos] = '\n';
		log_buffer[end_pos + 1u] = '\0';

		_write(log_buffer, strlen(log_buffer));
		printf(log_buffer);

		va_end(args);
	}

	void console_clear()
	{
		console_buffer.pos = 0u;
		console_buffer.flipped = false;
	}

	void update_console()
	{
		if (!engine.console_active) return;

		current_command.insert(current_command.begin() + cursor_pos, input.text.begin(), input.text.end());
		cursor_pos += u32(input.text.size());

		bool execute = false;

		foreach(i, input.text_commands.size()) {

			TextCommand txtcmd = input.text_commands[i];

			switch (txtcmd)
			{
			case TextCommand_DeleteLeft:
				if (cursor_pos) {

					--cursor_pos;
					current_command.erase(current_command.begin() + cursor_pos);
				}
				break;
			case TextCommand_DeleteRight:
				if (cursor_pos < current_command.size()) current_command.erase(current_command.begin() + cursor_pos);
				break;

			case TextCommand_Enter:
			{
				execute = true;
			}
			break;

			case TextCommand_Escape:
				break;
			}
		}

		if (input.keys[Key_Left] == InputState_Pressed) cursor_pos = (cursor_pos == 0u) ? cursor_pos : (cursor_pos - 1u);
		if (input.keys[Key_Right] == InputState_Pressed) cursor_pos = (cursor_pos == current_command.size()) ? cursor_pos : (cursor_pos + 1u);

		if (execute && current_command.size()) {
			execute_command(current_command.c_str());
			current_command.clear();
			cursor_pos = 0u;
		}

		text_offset = std::max(text_offset + input.mouse_wheel, 0.f);

		if (!scroll_selected && line_count > LINE_COUNT && input.mouse_buttons[MouseButton_Left] == InputState_Pressed && input.mouse_position.x > 0.5f - SCROLL_WIDTH * 0.5f && input.mouse_position.y > 0.5f - CONSOLE_HEIGHT * 0.5f) {

			scroll_selected = true;
		}
		else if (scroll_selected) {

			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed || input.mouse_buttons[MouseButton_Left] == InputState_None)
				scroll_selected = false;
			else {

				f32 min = 0.5f - CONSOLE_HEIGHT * 0.5f + SCROLL_HEIGHT * 0.25f;
				f32 max = 0.5f - SCROLL_HEIGHT * 0.25f;
				f32 value = (input.mouse_position.y - min) / (max - min);

				text_offset = std::max(0.f, value * f32(line_count - std::min(line_count, LINE_COUNT)));
			}
		}

		input.text_commands.clear();
		input.text.clear();
	}

	void draw_console(GPUImage* offscreen,  CommandList cmd)
	{
		if (!engine.console_active) return;

		// Flip console
		if (console_buffer.flipped && console_buffer.pos) {

			char* aux = (char*)malloc(console_buffer.pos);
			memcpy(aux, console_buffer.buff, console_buffer.pos);

			memcpy(console_buffer.buff, console_buffer.buff + console_buffer.pos, CONSOLE_SIZE - console_buffer.pos);
			memcpy(console_buffer.buff + CONSOLE_SIZE - console_buffer.pos, aux, console_buffer.pos);

			free(aux);
			console_buffer.pos = CONSOLE_SIZE;
			console_buffer.flipped = false;
		}

		f32 window_width = (f32)window_width_get(engine.window);
		f32 window_height = (f32)window_height_get(engine.window);
		f32 aspect = window_width / window_height;

		line_count = compute_text_lines(console_buffer.buff, console_buffer.pos, TEXT_SIZE, 2.f, aspect, &font_console);
		text_offset = std::min(text_offset, std::max(f32(line_count) - f32(LINE_COUNT), 0.f));

		begin_debug_batch(cmd);

		f32 console_width = 2.f;
		f32 console_x = 0.f;

		if (line_count > LINE_COUNT) {

			console_width -= SCROLL_WIDTH;
			console_x -= SCROLL_WIDTH * 0.5f;

			// Draw scroll
			draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, 1.f - CONSOLE_HEIGHT * 0.5f, 0.f }, { SCROLL_WIDTH, CONSOLE_HEIGHT }, Color::Gray(20u), cmd);

			f32 value = text_offset / f32(line_count - std::min(line_count, LINE_COUNT));

			draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, (1.f - CONSOLE_HEIGHT + SCROLL_HEIGHT * 0.5f) + (CONSOLE_HEIGHT - SCROLL_HEIGHT) * value, 0.f }, { SCROLL_WIDTH, SCROLL_HEIGHT }, Color::White(), cmd);
		}

		draw_debug_quad({ console_x, 1.f - CONSOLE_HEIGHT * 0.5f, 0.f }, { console_width, CONSOLE_HEIGHT }, Color::Black(180u), cmd);
		draw_debug_quad({ 0.f, 1.f - CONSOLE_HEIGHT - COMMAND_TEXT_SIZE * 0.5f, 0.f }, { 2.f, COMMAND_TEXT_SIZE }, Color::Black(), cmd);

		end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

		f32 text_x = -1.f;
		f32 text_y = 1.f - CONSOLE_HEIGHT;

		f32 buffer_x = -1.f;
		f32 buffer_y = 1.f;

		if (current_command.size()) {
			
			draw_text(offscreen, current_command.c_str(), current_command.size(), text_x, text_y, 2.f, 1u, COMMAND_TEXT_SIZE, aspect, TextAlignment_Left, &font_console, Color::White(), cmd);
		}

		if (console_buffer.pos) {

			draw_text_area(offscreen, console_buffer.buff, console_buffer.pos, buffer_x, buffer_y, console_width, LINE_COUNT, TEXT_SIZE, aspect, TextAlignment_Left, u32(text_offset), true, &font_console, Color::White(), cmd);
		}

		begin_debug_batch(cmd);

		f32 width = compute_text_width(current_command.c_str(), cursor_pos, COMMAND_TEXT_SIZE, aspect, &font_console);
		f32 char_width = font_console.glyphs['i'].w * COMMAND_TEXT_SIZE;

		draw_debug_quad({ text_x + width + char_width * 0.5f, text_y - COMMAND_TEXT_SIZE * 0.5f, 0.f }, { char_width, COMMAND_TEXT_SIZE }, Color::Red(100u), cmd);

		end_debug_batch(offscreen, XMMatrixIdentity(), cmd);
	}

}