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
	static f32 show_fade = 0.f;

	static constexpr u32 HISTORY_COUNT = 300u;
	static constexpr u32 CONSOLE_SIZE = 10000u;
	static constexpr f32 CONSOLE_HEIGHT = 1.7f;
	static constexpr u32 LINE_COUNT = 35u;
	static constexpr f32 TEXT_SIZE = CONSOLE_HEIGHT / f32(LINE_COUNT);
	static constexpr f32 COMMAND_TEXT_SIZE = 0.06f;
	static constexpr f32 SCROLL_WIDTH = 0.03f;
	static constexpr f32 SCROLL_HEIGHT = 0.12f;
	static constexpr f32 ANIMATION_TIME = 0.25f;

	struct ConsoleBuffer {
		char* buff;
		size_t pos;
		bool flipped;
		std::string history[HISTORY_COUNT] = {};
		bool history_flip = false;
		u32 history_pos = 0u;
		u32 history_offset = 0u;
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

	static Result command_scene(const char** args, u32 argc) {

		if (argc > 1u) {
			SV_LOG_ERROR("Too much arguments");
			return Result_InvalidUsage;
		}

		if (argc == 0u) {
			SV_LOG_ERROR("Should specify the scene name");
			return Result_InvalidUsage;
		}

		const char* name = args[0];

		Result res = set_active_scene(name);

		if (result_okay(res)) SV_LOG("Loading scene '%s'", name);
		else {

			if (res == Result_NotFound) {
				SV_LOG_ERROR("Scene '%s' not found", name);
			}
			else SV_LOG_ERROR("%s", result_str(res));
		}

		return res;
	}

	static Result command_fps(const char** args, u32 argc) {
		
		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return Result_InvalidUsage;
		}

		SV_LOG("%u", engine.FPS);

		return Result_Success;
	}

	static Result command_timer(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return Result_InvalidUsage;
		}

		SV_LOG("%lf", timer_now().toSeconds_f64());

		return Result_Success;
	}

	static Result command_import_mesh(const char** args, u32 argc) 
	{
		if (argc < 2u) {
			SV_LOG_ERROR("This command need more arguments");
			return Result_InvalidUsage;
		}

		if (argc > 2u) {
			SV_LOG_ERROR("Too much arguments");
			return Result_InvalidUsage;
		}

		const char* srcpath = args[0u];
		const char* dstpath = args[1u];

		ModelInfo model_info;

		Result res = load_model(srcpath, model_info);
		if (result_fail(res)) {

			SV_LOG_ERROR("Can't load '%s': %s", srcpath, result_str(res));
			return res;
		}

		res = import_model(dstpath, model_info);
		
		if (result_fail(res)) {

			SV_LOG_ERROR("Can't save '%s': %s", dstpath, result_str(res));
			return res;
		}

		SV_LOG("Model imported successfully:\nSrc: '%s'\nDst: '%s'", srcpath, dstpath);

		return Result_Success;
	}

	static Result command_save_scene(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return Result_InvalidUsage;
		}

		std::string filepath = engine.callbacks.get_scene_filepath ? engine.callbacks.get_scene_filepath(engine.scene->name.c_str()) : "";

		Result res;

		if (filepath.size())
			res = save_scene(engine.scene, filepath.c_str());
		else
			res = Result_NotFound;

		if (result_fail(res)) {
			SV_LOG_ERROR("Can't save the scene '%s': %s", engine.scene->name.c_str(), result_str(res));
			return res;
		}
		else {
			SV_LOG("Scene '%s' saved at '%s'", engine.scene->name.c_str(), filepath.c_str());
		}
		
		return Result_Success;
	}

	static Result command_clear_scene(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return Result_InvalidUsage;
		}

		Result res = clear_scene(engine.scene);

		SV_LOG("Scene '%s' cleared", engine.scene->name.c_str());

		return Result_Success;
	}

	static Result command_create_entity_model(const char** args, u32 argc) {

		if (argc != 1u) {
			SV_LOG_ERROR("This command need one argument");
			return Result_InvalidUsage;
		}

		if (engine.scene == nullptr) {
			SV_LOG_ERROR("Must initialize a scene");
			return Result_InvalidUsage;
		}

		return create_entity_model(engine.scene, SV_ENTITY_NULL, args[0]);
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
		register_command("scene", command_scene);
		register_command("fps", command_fps);
		register_command("timer", command_timer);
		register_command("import_model", command_import_mesh);
		register_command("save_scene", command_save_scene);
		register_command("clear_scene", command_clear_scene);
		register_command("create_entity_model", command_create_entity_model);

		// Recive command history from last execution
		{
			Archive archive;
			
			Result res = bin_read(archive, hash_string("CONSOLE HISTORY"));

			if (result_okay(res)) {

				u32 history_count;
				archive >> history_count;
				for ( ;console_buffer.history_pos < history_count; ++console_buffer.history_pos) {
					
					archive >> console_buffer.history[console_buffer.history_pos];
				}
			}
			else {
				SV_LOG_ERROR("Command history not found");
			}
		}
	}

	void close_console()
	{
		if (console_buffer.buff) {

			free(console_buffer.buff);
			console_buffer.buff = nullptr;
		}

		console_buffer.pos = 0U;
		console_buffer.flipped = false;

		// Save command history
		{
			Archive archive;

			archive << HISTORY_COUNT;
			foreach(i, HISTORY_COUNT) {

				u32 j = (console_buffer.history_pos + i) % HISTORY_COUNT;
				archive << console_buffer.history[j];
			}

			if (result_fail(bin_write(archive, hash_string("CONSOLE HISTORY")))) {
				SV_LOG_ERROR("Can't save the console history");
			}
		}
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

		// Save to history
		{
			console_buffer.history[console_buffer.history_pos] = command;
			u32 new_pos = (console_buffer.history_pos + 1u) % HISTORY_COUNT;
			if (new_pos < console_buffer.history_pos) console_buffer.history_flip = true;
			console_buffer.history_pos = new_pos;
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

			bool line = false;

			if (*command == '"') {
				line = true;
				++command;
			}

			name = command;

			if (line) while (*command != '\0' && *command != '"') ++command;
			else while (*command != '\0' && *command != ' ') ++command;

			size_t arg_size = command - name;
			char*& arg = args.emplace_back();
			arg = (char*)malloc(arg_size + 1u);
			memcpy(arg, name, arg_size);
			arg[arg_size] = '\0';

			if (line && *command != '\0') ++command;

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
		f32 animation_advance = engine.deltatime * (1.f / ANIMATION_TIME);

		if (!engine.console_active || !input.unused) {
			show_fade = std::max(0.f, show_fade - animation_advance);
			return;
		}

		show_fade = std::min(1.f, show_fade + animation_advance);

		input.unused = false;

		// Command History
		{
			bool change_command = false;
		
			if (input.keys[Key_Up] == InputState_Pressed) {
				
				u32 limit = console_buffer.history_flip ? HISTORY_COUNT : console_buffer.history_pos;
				console_buffer.history_offset = std::min(console_buffer.history_offset + 1u, limit - 1u);
				change_command = true;
			}
			if (input.keys[Key_Down] == InputState_Pressed) {
				
				if (console_buffer.history_offset != 0u) {
					
					--console_buffer.history_offset;
					change_command = true;
				}
			}
			
			if (change_command) {
				
				if (console_buffer.history_offset != 0u) {
					
					i32 pos = i32(console_buffer.history_pos) - i32(console_buffer.history_offset);
					while (pos < 0) pos = HISTORY_COUNT + pos;
					
					current_command = console_buffer.history[size_t(pos)];
				}
				else current_command.clear();
			}
		}
		
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

				f32 min = 0.5f - (CONSOLE_HEIGHT - font_console.vertical_offset * COMMAND_TEXT_SIZE) * 0.5f + SCROLL_HEIGHT * 0.25f;
				f32 max = 0.5f - SCROLL_HEIGHT * 0.25f;
				f32 value = (input.mouse_position.y - min) / (max - min);

				text_offset = std::max(0.f, value * f32(line_count - std::min(line_count, LINE_COUNT)));
			}
		}

		input.text_commands.clear();
		input.text.clear();
	}

	void draw_console(CommandList cmd)
	{
		if (!engine.console_active && show_fade == 0.f) return;

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

		f32 console_height = CONSOLE_HEIGHT - font_console.vertical_offset * COMMAND_TEXT_SIZE;
		f32 command_height = COMMAND_TEXT_SIZE - font_console.vertical_offset * COMMAND_TEXT_SIZE;
		f32 animation = (1.f - show_fade) * (console_height + COMMAND_TEXT_SIZE);

		f32 window_width = (f32)window_width_get(engine.window);
		f32 window_height = (f32)window_height_get(engine.window);
		f32 aspect = window_width / window_height;

		line_count = compute_text_lines(console_buffer.buff, u32(console_buffer.pos), TEXT_SIZE, 2.f, aspect, &font_console);
		text_offset = std::min(text_offset, std::max(f32(line_count) - f32(LINE_COUNT), 0.f));

		begin_debug_batch(cmd);

		f32 console_width = 2.f;
		f32 console_x = 0.f;

		if (line_count > LINE_COUNT) {

			console_width -= SCROLL_WIDTH;
			console_x -= SCROLL_WIDTH * 0.5f;

			// Draw scroll
			draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, 1.f - console_height * 0.5f + animation, 0.f }, { SCROLL_WIDTH, console_height }, Color::Gray(20u), cmd);

			f32 value = text_offset / f32(line_count - std::min(line_count, LINE_COUNT));

			draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, animation + (1.f - console_height + SCROLL_HEIGHT * 0.5f) + (console_height - SCROLL_HEIGHT) * value, 0.f }, { SCROLL_WIDTH, SCROLL_HEIGHT }, Color::White(), cmd);
		}

		draw_debug_quad({ console_x, animation + 1.f - console_height * 0.5f, 0.f }, { console_width, console_height }, Color::Black(180u), cmd);
		draw_debug_quad({ 0.f, animation + 1.f - console_height - command_height * 0.5f, 0.f }, { 2.f, command_height }, Color::Black(), cmd);

		end_debug_batch(engine.offscreen, nullptr, XMMatrixIdentity(), cmd);

		f32 text_x = -1.f;
		f32 text_y = 1.f - console_height + animation;

		f32 buffer_x = -1.f;
		f32 buffer_y = 1.f + animation;

		if (current_command.size()) {
			
			draw_text(engine.offscreen, current_command.c_str(), current_command.size(), text_x, text_y, 2.f, 1u, COMMAND_TEXT_SIZE, aspect, TextAlignment_Left, &font_console, Color::White(), cmd);
		}

		if (console_buffer.pos) {

			draw_text_area(engine.offscreen, console_buffer.buff, console_buffer.pos, buffer_x, buffer_y, console_width, LINE_COUNT, TEXT_SIZE, aspect, TextAlignment_Left, u32(text_offset), true, &font_console, Color::White(), cmd);
		}

		begin_debug_batch(cmd);

		f32 width = compute_text_width(current_command.c_str(), cursor_pos, COMMAND_TEXT_SIZE, aspect, &font_console);
		f32 char_width = font_console.glyphs['i'].w * COMMAND_TEXT_SIZE;

		draw_debug_quad({ text_x + width + char_width * 0.5f, text_y - command_height * 0.5f, 0.f }, { char_width, command_height }, Color::Red(100u), cmd);

		end_debug_batch(engine.offscreen, nullptr, XMMatrixIdentity(), cmd);
	}

}
