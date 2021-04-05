#include "core.h"

#if SV_DEV

#include "scene.h"
#include "dev.h"
#include "renderer/renderer_internal.h"

#include <stdarg.h>

namespace sv {

    static constexpr u32 HISTORY_COUNT = 100u;
    static constexpr u32 CONSOLE_SIZE = 10000u;
    static constexpr f32 CONSOLE_HEIGHT = 1.7f;
    static constexpr u32 LINE_COUNT = 35u;
    static constexpr f32 TEXT_SIZE = CONSOLE_HEIGHT / f32(LINE_COUNT);
    static constexpr f32 COMMAND_TEXT_SIZE = 0.06f;
    static constexpr f32 SCROLL_WIDTH = 0.03f;
    static constexpr f32 SCROLL_HEIGHT = 0.12f;
    static constexpr f32 ANIMATION_TIME = 0.25f;

    struct GlobalConsoleData {
	char* buff;
	size_t buff_pos;
	bool buff_flip;
		
	std::string history[HISTORY_COUNT] = {};
	bool history_flip = false;
	u32 history_pos = 0u;
	u32 history_offset = 0u;

	std::unordered_map<std::string, CommandFn> commands;
	std::string current_command;
	u32 cursor_pos = 0u;
	f32 text_offset = 0.f;
	u32 line_count = 0u;
	bool scroll_selected = false;
	f32 show_fade = 0.f;
    };

    static GlobalConsoleData console;

    static Result command_exit(const char** args, u32 argc)
    {
	dev.console_active = false;
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

    static Result command_import_model(const char** args, u32 argc) 
    {
	if (argc < 1u) {
	    SV_LOG_ERROR("This command need more arguments");
	    return Result_InvalidUsage;
	}

	if (argc > 2u) {
	    SV_LOG_ERROR("Too much arguments");
	    return Result_InvalidUsage;
	}

	const char* srcpath;
	const char* dstpath;

	std::string srcpath_str;

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

	    //srcpath_str = file_dialog_open(2u, filters, "");
	    return Result_TODO;
	    //if (srcpath_str.empty()) return Result_Success;
	    //srcpath = srcpath_str.c_str();
	}

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

	// Save scene
	char filepath[300];
	const char* name = engine.scene->name.c_str();
	
	if (user_get_scene_filepath(name, filepath)) {

	    Result res = save_scene(engine.scene, filepath);

	    if (result_fail(res)) {
		SV_LOG_ERROR("Can't save the scene '%s': %s", name, result_str(res));
		return res;
	    }
	    else {
		SV_LOG("Scene '%s' saved at '%s'", name, filepath);
	    }
		
	    return Result_Success;
	}
	else {
	    SV_LOG_INFO("The scene '%s' can't be saved (Specified by the user)", name);
	    return Result_Success;
	}  
    }

    static Result command_clear_scene(const char** args, u32 argc) {

	if (argc) {
	    SV_LOG_ERROR("This command doesn't need arguments");
	    return Result_InvalidUsage;
	}

	Result res = clear_scene(engine.scene);

	if (result_okay(res)) {
	    SV_LOG("Scene '%s' cleared", engine.scene->name.c_str());
	    return Result_Success;
	}
	else {
	    SV_LOG_ERROR("Can't clear the scene: %s", res);
	    return res;
	}
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
	console.buff = (char*)malloc(CONSOLE_SIZE);
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
	    Archive archive;
			
	    Result res = bin_read(hash_string("CONSOLE HISTORY"), archive);

	    if (result_okay(res)) {

		u32 history_count;
		archive >> history_count;

		history_count = std::min(history_count, HISTORY_COUNT);

		for (i32 i = history_count - 1u; i >= 0; --i) {
					
		    archive >> console.history[size_t(i)];
		}

		console.history_pos = history_count;
		if (console.history_pos == HISTORY_COUNT) {
		    console.history_pos = 0u;
		    console.history_flip = true;
		}
	    }
	    else {
		SV_LOG_ERROR("Command history not found");
	    }
	}
    }

    void close_console()
    {
	if (console.buff) {

	    free(console.buff);
	    console.buff = nullptr;
	}

	console.buff_pos = 0U;
	console.buff_flip = false;

	// Save command history
	{
	    Archive archive;

	    u32 history_count = console.history_flip ? HISTORY_COUNT : console.history_pos;
	    archive << history_count;
	    foreach(i, history_count) {

		i32 pos = (i32(console.history_pos) - i32(i + 1u));
		while (pos < 0) pos = HISTORY_COUNT + pos;
		u32 j = pos;
		archive << console.history[j];
	    }

	    if (result_fail(bin_write(hash_string("CONSOLE HISTORY"), archive))) {
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
	    console.history[console.history_pos] = command;
	    u32 new_pos = (console.history_pos + 1u) % HISTORY_COUNT;
	    if (new_pos < console.history_pos) console.history_flip = true;
	    console.history_pos = new_pos;
	    console.history_offset = 0u;
	}

	const char* name = command;

	while (*command != ' ' && *command != '\0') ++command;

	size_t command_size = command - name;

	std::string command_name;
	command_name.resize(command_size);

	memcpy((void*)command_name.data(), name, command_size);

	auto it = console.commands.find(command_name);
	if (it == console.commands.end()) {
	    SV_LOG_ERROR("Command '%s' not found", command_name.c_str());
	    return Result_NotFound;
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

	auto it = console.commands.find(name);
	if (it != console.commands.end()) return Result_Duplicated;
		
	console.commands[name] = command_fn;
	return Result_Success;
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

    void update_console()
    {
	f32 animation_advance = engine.deltatime * (1.f / ANIMATION_TIME);

	if (!dev.console_active || !input.unused) {
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
					
		    console.current_command = console.history[size_t(pos)];
		    console.cursor_pos = (u32)console.current_command.size();
		}
		else {
		    console.current_command.clear();
		    console.cursor_pos = 0u;
		}
	    }
	}
		
	if (input.text.size()) {

	    console.current_command.insert(console.current_command.begin() + console.cursor_pos, input.text.begin(), input.text.end());
	    console.cursor_pos += u32(input.text.size());

	    console.history_offset = 0u;
	}

	bool execute = false;

	foreach(i, input.text_commands.size()) {

	    TextCommand txtcmd = input.text_commands[i];

	    switch (txtcmd)
	    {
	    case TextCommand_DeleteLeft:
		if (console.cursor_pos) {

		    --console.cursor_pos;
		    console.current_command.erase(console.current_command.begin() + console.cursor_pos);
		}
		break;
	    case TextCommand_DeleteRight:
		if (console.cursor_pos < console.current_command.size()) console.current_command.erase(console.current_command.begin() + console.cursor_pos);
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

	if (input.keys[Key_Left] == InputState_Pressed) console.cursor_pos = (console.cursor_pos == 0u) ? console.cursor_pos : (console.cursor_pos - 1u);
	if (input.keys[Key_Right] == InputState_Pressed) console.cursor_pos = (console.cursor_pos == console.current_command.size()) ? console.cursor_pos : (console.cursor_pos + 1u);

	if (execute && console.current_command.size()) {
	    execute_command(console.current_command.c_str());
	    console.current_command.clear();
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

		f32 min = 0.5f - (CONSOLE_HEIGHT - font_console.vertical_offset * COMMAND_TEXT_SIZE) * 0.5f + SCROLL_HEIGHT * 0.25f;
		f32 max = 0.5f - SCROLL_HEIGHT * 0.25f;
		f32 value = (input.mouse_position.y - min) / (max - min);

		console.text_offset = std::max(0.f, value * f32(console.line_count - std::min(console.line_count, LINE_COUNT)));
	    }
	}

	input.text_commands.clear();
	input.text.clear();
    }

    void draw_console()
    {
	if (!dev.console_active && console.show_fade == 0.f) return;

	CommandList cmd = graphics_commandlist_get();

	// Flip console
	if (console.buff_flip && console.buff_pos) {

	    char* aux = (char*)malloc(console.buff_pos);
	    memcpy(aux, console.buff, console.buff_pos);

	    memcpy(console.buff, console.buff + console.buff_pos, CONSOLE_SIZE - console.buff_pos);
	    memcpy(console.buff + CONSOLE_SIZE - console.buff_pos, aux, console.buff_pos);

	    free(aux);
	    console.buff_pos = CONSOLE_SIZE;
	    console.buff_flip = false;
	}

	f32 console_height = CONSOLE_HEIGHT - font_console.vertical_offset * COMMAND_TEXT_SIZE;
	f32 command_height = COMMAND_TEXT_SIZE - font_console.vertical_offset * COMMAND_TEXT_SIZE;
	f32 animation = (1.f - console.show_fade) * (console_height + COMMAND_TEXT_SIZE);

	f32 window_width = (f32)os_window_size().x;
	f32 window_height = (f32)os_window_size().y;
	f32 aspect = window_width / window_height;

	console.line_count = compute_text_lines(console.buff, u32(console.buff_pos), TEXT_SIZE, 2.f, aspect, &font_console);
	console.text_offset = std::min(console.text_offset, std::max(f32(console.line_count) - f32(LINE_COUNT), 0.f));

	begin_debug_batch(cmd);

	f32 console_width = 2.f;
	f32 console_x = 0.f;

	if (console.line_count > LINE_COUNT) {

	    console_width -= SCROLL_WIDTH;
	    console_x -= SCROLL_WIDTH * 0.5f;

	    // Draw scroll
	    draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, 1.f - console_height * 0.5f + animation, 0.f }, { SCROLL_WIDTH, console_height }, Color::Gray(20u), cmd);

	    f32 value = console.text_offset / f32(console.line_count - std::min(console.line_count, LINE_COUNT));

	    draw_debug_quad({ 1.f - SCROLL_WIDTH * 0.5f, animation + (1.f - console_height + SCROLL_HEIGHT * 0.5f) + (console_height - SCROLL_HEIGHT) * value, 0.f }, { SCROLL_WIDTH, SCROLL_HEIGHT }, Color::White(), cmd);
	}

	draw_debug_quad({ console_x, animation + 1.f - console_height * 0.5f, 0.f }, { console_width, console_height }, Color::Black(180u), cmd);
	draw_debug_quad({ 0.f, animation + 1.f - console_height - command_height * 0.5f, 0.f }, { 2.f, command_height }, Color::Black(), cmd);

	end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	f32 text_x = -1.f;
	f32 text_y = 1.f - console_height + animation;

	f32 buffer_x = -1.f;
	f32 buffer_y = 1.f + animation;

	if (console.current_command.size()) {
			
	    draw_text(console.current_command.c_str(), console.current_command.size(), text_x, text_y, 2.f, 1u, COMMAND_TEXT_SIZE, aspect, TextAlignment_Left, &font_console, Color::White(), cmd);
	}

	if (console.buff_pos) {

	    draw_text_area(console.buff, console.buff_pos, buffer_x, buffer_y, console_width, LINE_COUNT, TEXT_SIZE, aspect, TextAlignment_Left, u32(console.text_offset), true, &font_console, Color::White(), cmd);
	}

	begin_debug_batch(cmd);

	f32 width = compute_text_width(console.current_command.c_str(), console.cursor_pos, COMMAND_TEXT_SIZE, aspect, &font_console);
	f32 char_width = font_console.glyphs['i'].w * COMMAND_TEXT_SIZE;

	draw_debug_quad({ text_x + width + char_width * 0.5f, text_y - command_height * 0.5f, 0.f }, { char_width, command_height }, Color::Red(100u), cmd);

	end_debug_batch(true, false, XMMatrixIdentity(), cmd);
    }

}

#endif