#include "utils/serialize.h"
#include "core/engine.h"
#include "platform/os.h"

namespace sv {

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
	SV_CHECK(file_read_binary(filePath, d.buff));
	
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
