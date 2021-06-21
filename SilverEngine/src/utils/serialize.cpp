#include "utils/serialize.h"
#include "core/engine.h"
#include "platform/os.h"

namespace sv {

    bool read_var_file(const char* filepath, List<Var>& vars)
    {
		char* text;
		size_t size;
	
		if (file_read_text(filepath, &text, &size)) {

			LineProcessor p;

			line_begin(p, text);

			while (line_next(p)) {

				Var var;

				line_jump_spaces(p.line);

				if (p.line[0] == '\0') {
					continue;
				}

				char delimiters[] = { '\n', '\r', '=', ' ' };
				size_t name_size = string_split(p.line, delimiters, SV_ARRAY_SIZE(delimiters));
				
				if (name_size == 0u) {
					SV_LOG_ERROR("Value name not found at line %u", p.line_count);
				}		
				else {
					
					if (name_size >= VARNAME_SIZE) {
						SV_LOG_ERROR("The value name at line %u is too large, the limit is %u chars", p.line_count, VARNAME_SIZE);
					}
					else {

						memcpy(var.name, p.line, name_size);
						var.name[name_size] = '\0';

						bool valid_value = false;

						p.line += name_size;
						line_jump_spaces(p.line);

						if (p.line[0] != '\0' && p.line[0] != '\r' && p.line[0] != '\n') {

							++p.line;
							line_jump_spaces(p.line);
							
							size_t value_size = string_split(p.line, delimiters, SV_ARRAY_SIZE(delimiters));

							if (value_size) {

								if (p.line[0] == '"') {

									++p.line;

									char string_delimiters[] = { '\n', '\r', '"' };
									
									size_t string_size = string_split(p.line, string_delimiters, SV_ARRAY_SIZE(string_delimiters));
									

									if (p.line[string_size] == '"') {
										
										var.type = VarType_String;
										memcpy(var.value.string, p.line, string_size);
										var.value.string[string_size] = '\0';
										valid_value = true;
									}
								}
								else {

									u32 minus_count = 0u;
									u32 dot_count = 0u;
									bool is_number = true;

									foreach(i, value_size) {

										char c = p.line[i];
											
										if (!char_is_number(c)) {

											if (c == '.') ++dot_count;
											else if (c == '-') ++minus_count;
											else is_number = false;
										}
									}

									if (is_number && dot_count <= 1u && minus_count <= 1u) {

										if (dot_count) {

											var.type = VarType_Real;

											if (line_read_f64(p.line, var.value.real)) {
												valid_value = true;
											}
										}
										else {

											var.type = VarType_Integer;

											if (line_read_i64(p.line, var.value.integer, NULL, 0)) {
												valid_value = true;
											}
										}
									}
									else if (!is_number && value_size >= 4) {

										if (value_size == 4 && p.line[0] == 't' && p.line[1] == 'r' && p.line[2] == 'u' && p.line[3] == 'e') {
											var.type = VarType_Boolean;
											var.value.boolean = true;
											valid_value = true;
										}
										else if (value_size == 5 && p.line[0] == 'f' && p.line[1] == 'a' && p.line[2] == 'l' && p.line[3] == 's' && p.line[4] == 'e') {
											var.type = VarType_Boolean;
											var.value.boolean = false;
											valid_value = true;
										}
									}
								}
							}
						}
						else {
							var.type = VarType_Null;
							valid_value = true;
						}

						if (!valid_value) {
							SV_LOG_ERROR("Invalid value format in line %u", p.line_count);
						}
						else vars.push_back(var);
					}
				}
			}
	    
			SV_FREE_MEMORY(text);
			return true;
		}
		return false;
    }

    //////////////////////////////////// SERIALIZER /////////////////

    void serialize_begin(Serializer& s)
    {
		s.buff.reset();
	
		// TODO: move to .cpp
		serialize_version(s, engine.version);
		serialize_u32(s, Serializer::VERSION);
    }

    bool serialize_end(Serializer& s, const char* filepath)
    {
		return file_write_binary(filepath, s.buff.data(), s.buff.size(), false);
    }

    //////////////////////////////////// DESERIALIZER /////////////////

    bool deserialize_begin(Deserializer& d, const char* filepath)
    {
		SV_CHECK(file_read_binary(filepath, d.buff));
	
		d.pos = 0u;

		SV_CHECK(deserialize_assert(d, sizeof(Version) + sizeof(u32)));

		deserialize_version(d, d.engine_version);
		deserialize_u32(d, d.serializer_version);

		if (d.serializer_version < Deserializer::LAST_VERSION_SUPPORTED) {
			deserialize_end(d);
			return false;
		}

		return true;
    }
    
    void deserialize_end(Deserializer& d)
    {
		d.buff.clear();
    }
    
}
