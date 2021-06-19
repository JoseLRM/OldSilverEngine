#include "debug/console.h"

#if SV_EDITOR

#include "debug/editor.h"

#include "core/scene.h"
#include "core/engine.h"
#include "core/renderer/renderer_internal.h"

#include "platform/os.h"

#include <stdarg.h>

namespace sv {

    static constexpr u32 COMMAND_LINE_SIZE = 100u;
    static constexpr u32 HISTORY_COUNT = 100u;
    static constexpr u32 CONSOLE_SIZE = 200000u;
    static constexpr f32 CONSOLE_HEIGHT = 1.7f;
    static constexpr u32 LINE_COUNT = 40u;
    static constexpr f32 TEXT_SIZE = CONSOLE_HEIGHT / f32(LINE_COUNT);
    static constexpr f32 COMMAND_TEXT_SIZE = 0.06f;
    static constexpr f32 SCROLL_WIDTH = 0.03f;
    static constexpr f32 SCROLL_HEIGHT = 0.12f;
    static constexpr f32 ANIMATION_TIME = 0.25f;

    struct GlobalConsoleData {
	
		bool active = false;
	
		char* buff;
		size_t buff_pos;
		bool buff_flip;

		char history[COMMAND_LINE_SIZE + 1u][HISTORY_COUNT] = {};
		bool history_flip = false;
		u32 history_pos = 0u;
		u32 history_offset = 0u;

		ThickHashTable<CommandFn, 300u> commands;
		char current_command[COMMAND_LINE_SIZE + 1u];
		u32 cursor_pos = 0u;
		f32 text_offset = 0.f;
		u32 line_count = 0u;
		bool scroll_selected = false;
		f32 show_fade = 0.f;
    };

    SV_INTERNAL GlobalConsoleData console;

    static bool command_exit(const char** args, u32 argc)
    {
		engine.close_request = true;
		return true;
    }

    static bool command_close(const char** args, u32 argc)
    {
		console.active = false;
		return true;
    }

    static bool command_test(const char** args, u32 argc)
    {
		if (argc == 0u)
			foreach(i, 100) {
				console_print("Hola que tal, esto es un test %u\n", i);
			}

		else

			foreach(i, argc) {
				console_notify("ARG", args[i]);
			}

		return true;
    }

    static bool command_clear(const char** args, u32 argc)
    {
		console_clear();
		return true;
    }

    static bool command_scene(const char** args, u32 argc) {

		if (argc > 1u) {
			SV_LOG_ERROR("Too much arguments");
			return false;
		}

		if (argc == 0u) {
			SV_LOG_ERROR("Should specify the scene name");
			return false;
		}

		const char* name = args[0];

		if (set_scene(name)) SV_LOG("Loading scene '%s'", name);
		else {

			SV_LOG_ERROR("Scene '%s' not found", name);
			return false;
		}

		return true;
    }

    static bool command_fps(const char** args, u32 argc) {
		
		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return false;
		}

		SV_LOG("%u", engine.FPS);

		return true;
    }

    static bool command_timer(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return false;
		}

		SV_LOG("%lf", timer_now());

		return true;
    }

    static bool command_import_model(const char** args, u32 argc) 
    {
		if (argc < 1u) {
			SV_LOG_ERROR("This command need more arguments");
			return false;
		}

		if (argc > 2u) {
			SV_LOG_ERROR("Too much arguments");
			return false;
		}

		const char* srcpath;
		const char* dstpath;

		char src[FILEPATH_SIZE + 1u] = "";

		if (argc == 2u) {
			srcpath = args[0u];
			dstpath = args[1u];
		}
		else {
			dstpath = args[0u];

			const char* filters[] = {
				"all", "*",
				"obj", "*.obj",
			};

			if (!file_dialog_open(src, 2u, filters, "")) {
				return true;
			}

			srcpath = src;
		}

		ModelInfo model_info;

		if (!load_model(src, model_info)) {

			SV_LOG_ERROR("Can't load '%s'", srcpath);
			return false;
		}
		
		if (!import_model(dstpath, model_info)) {

			SV_LOG_ERROR("Can't save '%s'", dstpath);
			return false;
		}

		SV_LOG("Model imported successfully:\nSrc: '%s'\nDst: '%s'", srcpath, dstpath);

		return true;
    }

    static bool command_save_scene(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return false;
		}

		if (!save_scene()) {
			SV_LOG_ERROR("Can't save the scene");
			return false;
		}
		else {
			SV_LOG("Scene saved");
		}
		
		return true;  
    }

    static bool command_clear_scene(const char** args, u32 argc) {

		if (argc) {
			SV_LOG_ERROR("This command doesn't need arguments");
			return false;
		}

		if (clear_scene()) {
			SV_LOG("Scene cleared");
			return true;
		}
		else {
			SV_LOG_ERROR("Can't clear the scene");
			return false;
		}
    }

    static bool command_create_entity_model(const char** args, u32 argc) {

		if (argc != 1u) {
			SV_LOG_ERROR("This command need one argument");
			return false;
		}

		Entity parent = create_entity(0, args[0]);
	
		return create_entity_model(parent, args[0]);
    }

    void _console_initialize()
    {
		console.buff = (char*)SV_ALLOCATE_MEMORY(CONSOLE_SIZE);
		console.buff_pos = 0U;
		console.buff_flip = false;

		// Register default commands
	
		register_command("exit", command_exit);
		register_command("close", command_close);
		register_command("test", command_test);
		register_command("clear", command_clear);
		register_command("scene", command_scene);
		register_command("fps", command_fps);
		register_command("timer", command_timer);
		register_command("import_model", command_import_model);
		register_command("save_scene", command_save_scene);
		register_command("clear_scene", command_clear_scene);
		register_command("create_entity_model", command_create_entity_model);
	
		//  Recive command history from last execution
		{
			Deserializer d;

			if (bin_read(hash_string("CONSOLE HISTORY"), d, true)) {

				u32 history_count;
				deserialize_u32(d, history_count);

				history_count = SV_MIN(history_count, HISTORY_COUNT);

				for (i32 i = history_count - 1u; i >= 0; --i) {

					char* s = console.history[size_t(i)];
					deserialize_string(d, s, COMMAND_LINE_SIZE + 1u);
				}

				console.history_pos = history_count;
				if (console.history_pos == HISTORY_COUNT) {
					console.history_pos = 0u;
					console.history_flip = true;
				}

				deserialize_end(d);
			}
			else {
				SV_LOG_ERROR("Command history not found");
			}
		}
    }

    void _console_close()
    {
		if (console.buff) {

			free(console.buff);
			console.buff = nullptr;
		}

		console.buff_pos = 0U;
		console.buff_flip = false;

		console.commands.clear();

		// Save command history
		{
			Serializer s;

			serialize_begin(s);

			u32 history_count = console.history_flip ? HISTORY_COUNT : console.history_pos;

			serialize_u32(s, history_count);

			foreach(i, history_count) {

				i32 pos = (i32(console.history_pos) - i32(i + 1u));
				while (pos < 0) pos = HISTORY_COUNT + pos;
				u32 j = pos;
				serialize_string(s, console.history[j]);
			}

			if (!bin_write(hash_string("CONSOLE HISTORY"), s, true)) {
				SV_LOG_ERROR("Can't save the console history");
			}
		}
    }

    bool execute_command(const char* command)
    {
		if (command == nullptr) {
			SV_LOG_ERROR("Null ptr command, what are you doing?");
			return false;
		}

		while (*command == ' ') ++command;

		size_t command_size = strlen(command);

		if (command_size == 0u) {
			SV_LOG_ERROR("This is an empty command string");
			return false;
		}
		else if (command_size > COMMAND_LINE_SIZE) {
			SV_LOG_ERROR("This command is too large");
			return false;
		}

		// Save to history
		{
			strcpy(console.history[console.history_pos], command);
			u32 new_pos = (console.history_pos + 1u) % HISTORY_COUNT;
			if (new_pos < console.history_pos) console.history_flip = true;
			console.history_pos = new_pos;
			console.history_offset = 0u;
		}

		const char* name = command;

		while (*command != ' ' && *command != '\0') ++command;

		command_size = command - name;

		char command_name[COMMAND_LINE_SIZE + 1u];
		memcpy(command_name, name, command_size);
		command_name[command_size] = '\0';

		CommandFn* fn = console.commands.find(command_name);
		if (fn == nullptr) {
			SV_LOG_ERROR("Command '%s' not found", command_name);
			return false;
		}

		List<char*> args;

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
			arg = (char*)SV_ALLOCATE_MEMORY(arg_size + 1u);
			memcpy(arg, name, arg_size);
			arg[arg_size] = '\0';

			if (line && *command != '\0') ++command;

			while (*command == ' ') ++command;
		}

		bool res = (*fn)((const char**)args.data(), u32(args.size()));

		// Free
		for (char* arg : args)
			free(arg);

		return res;
    }

    bool register_command(const char* name, CommandFn command_fn)
    {
		if (command_fn == nullptr || name == nullptr || *name == '\0') return false;

		CommandFn* fn = console.commands.find(name);
		if (fn != nullptr) return false;
		
		console.commands[name] = command_fn;
		return true;
    }

    static void _write(const char* str, size_t size)
    {
		if (size == 0u) return;

		size_t capacity = CONSOLE_SIZE - console.buff_pos;

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

		memcpy(console.buff + console.buff_pos, str, write);

		if (rest) {

			console.buff_flip = true;
			console.buff_pos = 0u;
			_write(str + write, rest);
		}
		else console.buff_pos += write;
    }

    void console_print(const char* str, ...)
    {
		console.text_offset = 0.f;

		va_list args;
		va_start(args, str);

		char log_buffer[1000];

		vsnprintf(log_buffer, 1000, str, args);

		_write(log_buffer, strlen(log_buffer));
		sv::print(log_buffer);

		va_end(args);
    }

    void console_notify(const char* title, const char* str, ...)
    {
		console.text_offset = 0.f;

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
		sv::print(log_buffer);

		va_end(args);
    }

    void console_clear()
    {
		console.buff_pos = 0u;
		console.buff_flip = false;
    }

    bool console_is_open()
    {
		return console.active;
    }
    
    void console_open()
    {
		console.active = true;
    }

    void console_close()
    {
		console.active = false;
    }

    void _console_update()
    {
		f32 animation_advance = engine.deltatime * (1.f / ANIMATION_TIME);

		if (!console.active || !input.unused) {
			console.show_fade = SV_MAX(0.f, console.show_fade - animation_advance);
			return;
		}

		console.show_fade = SV_MIN(1.f, console.show_fade + animation_advance);

		input.unused = false;

		// Command History
		{
			bool change_command = false;
		
			if (input.keys[Key_Up] == InputState_Pressed) {
				
				u32 limit = console.history_flip ? HISTORY_COUNT : console.history_pos;
				console.history_offset = SV_MIN(console.history_offset + 1u, limit);
				change_command = true;
			}
			if (input.keys[Key_Down] == InputState_Pressed) {
				
				if (console.history_offset != 0u) {
					
					--console.history_offset;
					change_command = true;
				}
			}
			
			if (change_command) {
				
				if (console.history_offset != 0u) {
					
					i32 pos = i32(console.history_pos) - i32(console.history_offset);
					while (pos < 0) pos = HISTORY_COUNT + pos;
					
					string_copy(console.current_command, console.history[size_t(pos)], COMMAND_LINE_SIZE + 1u);
					console.cursor_pos = (u32)strlen(console.current_command);
				}
				else {
					strcpy(console.current_command, "");
					console.cursor_pos = 0u;
				}
			}
		}

		bool execute;
		
		if (string_modify(console.current_command, COMMAND_LINE_SIZE + 1u, console.cursor_pos, &execute)) {

			console.history_offset = 0u;
		}

		if (execute && strlen(console.current_command)) {
			execute_command(console.current_command);
			strcpy(console.current_command, "");
			console.cursor_pos = 0u;
		}

		console.text_offset = SV_MAX(console.text_offset + input.mouse_wheel, 0.f);

		if (!console.scroll_selected && console.line_count > LINE_COUNT && input.mouse_buttons[MouseButton_Left] == InputState_Pressed && input.mouse_position.x > 0.5f - SCROLL_WIDTH * 0.5f && input.mouse_position.y > 0.5f - CONSOLE_HEIGHT * 0.5f) {

			console.scroll_selected = true;
		}
		else if (console.scroll_selected) {

			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed || input.mouse_buttons[MouseButton_Left] == InputState_None)
				console.scroll_selected = false;
			else {

				f32 min = 0.5f - (CONSOLE_HEIGHT - renderer->font_console.vertical_offset * COMMAND_TEXT_SIZE) * 0.5f + SCROLL_HEIGHT * 0.25f;
				f32 max = 0.5f - SCROLL_HEIGHT * 0.25f;
				f32 value = (input.mouse_position.y - min) / (max - min);

				console.text_offset = std::max(0.f, value * f32(console.line_count - std::min(console.line_count, LINE_COUNT)));
			}
		}
    }

    void _console_draw()
    {
		if (!console.active && console.show_fade == 0.f) return;

		CommandList cmd = graphics_commandlist_get();

		// Flip console
		if (console.buff_flip && console.buff_pos) {

			char* aux = (char*)SV_ALLOCATE_MEMORY(console.buff_pos);
			memcpy(aux, console.buff, console.buff_pos);

			memcpy(console.buff, console.buff + console.buff_pos, CONSOLE_SIZE - console.buff_pos);
			memcpy(console.buff + CONSOLE_SIZE - console.buff_pos, aux, console.buff_pos);

			free(aux);
			console.buff_pos = CONSOLE_SIZE;
			console.buff_flip = false;
		}

		f32 console_height = CONSOLE_HEIGHT - renderer->font_console.vertical_offset * COMMAND_TEXT_SIZE;
		f32 command_height = COMMAND_TEXT_SIZE - renderer->font_console.vertical_offset * COMMAND_TEXT_SIZE;
		f32 animation = (1.f - console.show_fade) * (console_height + COMMAND_TEXT_SIZE);

		f32 window_width = (f32)os_window_size().x;
		f32 window_height = (f32)os_window_size().y;
		f32 aspect = window_width / window_height;

		console.line_count = compute_text_lines(console.buff, u32(console.buff_pos), TEXT_SIZE, 2.f, aspect, &renderer->font_console);
		console.text_offset = std::min(console.text_offset, std::max(f32(console.line_count) - f32(LINE_COUNT), 0.f));

		imrend_begin_batch(cmd);

		imrend_camera(ImRendCamera_Clip, cmd);

		f32 console_width = 2.f;
		f32 console_x = 0.f;

		if (console.line_count > LINE_COUNT) {

			console_width -= SCROLL_WIDTH;
			console_x -= SCROLL_WIDTH * 0.5f;

			// Draw scroll
			imrend_draw_quad({ 1.f - SCROLL_WIDTH * 0.5f, 1.f - console_height * 0.5f + animation, 0.f }, { SCROLL_WIDTH, console_height }, Color::Gray(20u), cmd);

			f32 value = console.text_offset / f32(console.line_count - std::min(console.line_count, LINE_COUNT));

			imrend_draw_quad({ 1.f - SCROLL_WIDTH * 0.5f, animation + (1.f - console_height + SCROLL_HEIGHT * 0.5f) + (console_height - SCROLL_HEIGHT) * value, 0.f }, { SCROLL_WIDTH, SCROLL_HEIGHT }, Color::White(), cmd);
		}

		imrend_draw_quad({ console_x, animation + 1.f - console_height * 0.5f, 0.f }, { console_width, console_height }, Color::Black(180u), cmd);
		imrend_draw_quad({ 0.f, animation + 1.f - console_height - command_height * 0.5f, 0.f }, { 2.f, command_height }, Color::Black(), cmd);

		f32 text_x = -1.f;
		f32 text_y = 1.f - console_height + animation;

		f32 buffer_x = -1.f;
		f32 buffer_y = 1.f + animation;

		if (console.current_command[0]) {
			
			imrend_draw_text(console.current_command, strlen(console.current_command), text_x, text_y, 2.f, 1u, COMMAND_TEXT_SIZE, aspect, TextAlignment_Left, &renderer->font_console, Color::White(), cmd);
		}

		if (console.buff_pos) {

			imrend_draw_text_area(console.buff, console.buff_pos, buffer_x, buffer_y, console_width, LINE_COUNT, TEXT_SIZE, aspect, TextAlignment_Left, u32(console.text_offset), true, &renderer->font_console, Color::White(), cmd);
		}

		f32 width = compute_text_width(console.current_command, console.cursor_pos, COMMAND_TEXT_SIZE, aspect, &renderer->font_console);
		f32 char_width = renderer->font_console.glyphs['i'].w * COMMAND_TEXT_SIZE;

		imrend_draw_quad({ text_x + width + char_width * 0.5f, text_y - command_height * 0.5f, 0.f }, { char_width, command_height }, Color::Red(100u), cmd);

		imrend_flush(cmd);
    }

}

#endif

#if SV_SLOW

namespace sv {

    void throw_assertion(const char* content, u32 line, const char* file)
    {
#if SV_EDITOR
		console_notify("ASSERTION", "'%s', file: '%s', line: %u", content, file, line);	
#endif
	
		char text[500];
		memset(text, 0, 500);

		snprintf(text, 500, "'%s', file: '%s', line: %u", content, file, line);

		show_message("ASSERTION!!", text, true);
    }
    
}
#endif
