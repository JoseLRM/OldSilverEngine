#include "SilverEngine/core.h"

#include "SilverEngine/scene.h"

namespace sv {

	static std::unordered_map<std::string, CommandFn> commands;

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

		return it->second(0, 0);
	}

	Result register_command(const char* name, CommandFn command_fn)
	{
		if (command_fn == nullptr || name == nullptr || *name == '\0') return Result_InvalidUsage;

		auto it = commands.find(name);
		if (it != commands.end()) return Result_Duplicated;
		
		commands[name] = command_fn;
		return Result_Success;
	}

}