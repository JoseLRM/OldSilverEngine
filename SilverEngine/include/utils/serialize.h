#pragma once

#include "defines.h"
#include "utils/allocators.h"
#include "utils/math.h"

namespace sv {

    constexpr u32 VARNAME_SIZE = 30u;
    constexpr u32 VARVALUE_SIZE = 30u;

    enum VarType : u32 {
	VarType_None,
	    VarType_String,
	    VarType_Integer,
	    VarType_Real,
	    };
    
    struct Var {
	char name[VARNAME_SIZE];
	VarType type;
	union {
	    char string[VARVALUE_SIZE];
	    i64 integer;
	    f64 real;
	} value;
    };

    bool read_var_file(const char* filepath, List<Var>& vars);

    /////////////////////////// SERIALIZER //////////////////////////////////

    struct Serializer {
	constexpr u32 VERSION = 0u;
	RawList buff;
    };

    SV_API void serialize_begin(Serializer& s);
    SV_API bool serialize_end(Serializer& s, const char* filepath);

    SV_INLINE void serialize_u8(Serializer& s, u8 n)
    {
	s.buff.write_back(&n, sizeof(u8));
    }
    SV_INLINE void serialize_u16(Serializer& s, u16 n)
    {
	s.buff.write_back(&n, sizeof(u16));
    }
    SV_INLINE void serialize_u32(Serializer& s, u32 n)
    {
	s.buff.write_back(&n, sizeof(u32));
    }
    SV_INLINE void serialize_u64(Serializer& s, u64 n)
    {
	s.buff.write_back(&n, sizeof(u64));
    }

    SV_INLINE void serialize_i8(Serializer& s, i8 n)
    {
	s.buff.write_back(&n, sizeof(i8));
    }
    SV_INLINE void serialize_i16(Serializer& s, i16 n)
    {
	s.buff.write_back(&n, sizeof(i16));
    }
    SV_INLINE void serialize_i32(Serializer& s, i32 n)
    {
	s.buff.write_back(&n, sizeof(i32));
    }
    SV_INLINE void serialize_i64(Serializer& s, i64 n)
    {
	s.buff.write_back(&n, sizeof(i64));
    }

    SV_INLINE void serialize_f32(Serializer& s, f32 n)
    {
	s.buff.write_back(&n, sizeof(f32));
    }
    SV_INLINE void serialize_f64(Serializer& s, f64 n)
    {
	s.buff.write_back(&n, sizeof(f64));
    }

    SV_INLINE void serialize_char(Serializer& s, char n)
    {
	s.buff.write_back(&n, sizeof(char));
    }
    SV_INLINE void serialize_bool(Serializer& s, bool n)
    {
	s.buff.write_back(&n, sizeof(bool));
    }

    SV_INLINE void serialize_string(Serializer& s, const char* str)
    {
	size_t len = strlen(str) + 1u;
	s.buff.reserve(len);
	s.buff.write_back(str, len);
    }

    SV_INLINE void serialize_v2_f32(Serializer& s, const v2_f32& v)
    {
	s.buff.reserve(sizeof(f32) * 2u);
	serialize_f32(s, v.x);
	serialize_f32(s, v.y);
    }
    SV_INLINE void serialize_v3_f32(Serializer& s, const v3_f32& v)
    {
	s.buff.reserve(sizeof(f32) * 3u);
	serialize_f32(s, v.x);
	serialize_f32(s, v.y);
	serialize_f32(s, v.z);
    }
    SV_INLINE void serialize_v4_f32(Serializer& s, const v4_f32& v)
    {
	s.buff.reserve(sizeof(f32) * 4u);
	serialize_f32(s, v.x);
	serialize_f32(s, v.y);
	serialize_f32(s, v.z);
	serialize_f32(s, v.w);
    }

    SV_INLINE void serialize_version(Serializer& s, Version n)
    {
	s.buff.write_back(&n, sizeof(Version));
    }

    /////////////////////////// DESERIALIZER //////////////////////////////////

    struct Deserializer {
	constexpr u32 LAST_VERSION_SUPPORTED = 0u;
	u32 serializer_version;
	Version engine_version;
	RawList buff;
	size_t pos;
    }

    SV_API bool deserialize_begin(Deserializer& d, const char* filepath);
    SV_API void deserialize_end(Deserializer& d);

    SV_INLINE bool deserialize_assert(Deserializer& d, size_t size)
    {
	return (d.pos + size) <= d.buff.size();
    }

    SV_INLINE void deserialize_u8(Deserializer& d, u8& n)
    {
	s.buff.read_safe(&n, sizeof(u8), d.pos);
	d.pos += sizeof(u8);
    }
    SV_INLINE void deserialize_u16(Deserializer& d, u16& n)
    {
	s.buff.read_safe(&n, sizeof(u16), d.pos);
	d.pos += sizeof(u16);
    }
    SV_INLINE void deserialize_u32(Deserializer& d, u32& n)
    {
	s.buff.read_safe(&n, sizeof(u32), d.pos);
	d.pos += sizeof(u32);
    }
    SV_INLINE void deserialize_u64(Deserializer& d, u64& n)
    {
	s.buff.read_safe(&n, sizeof(u64), d.pos);
	d.pos += sizeof(u64);
    }

    SV_INLINE void deserialize_i8(Deserializer& d, i8& n)
    {
	s.buff.read_safe(&n, sizeof(i8), d.pos);
	d.pos += sizeof(i8);
    }
    SV_INLINE void deserialize_i16(Deserializer& d, i16& n)
    {
	s.buff.read_safe(&n, sizeof(i16), d.pos);
	d.pos += sizeof(i16);
    }
    SV_INLINE void deserialize_i32(Deserializer& d, i32& n)
    {
	s.buff.read_safe(&n, sizeof(i32), d.pos);
	d.pos += sizeof(i32);
    }
    SV_INLINE void deserialize_i64(Deserializer& d, i64& n)
    {
	s.buff.read_safe(&n, sizeof(u64), d.pos);
	d.pos += sizeof(u64);
    }

    SV_INLINE void deserialize_f32(Deserializer& d, f32& n)
    {
	s.buff.read_safe(&n, sizeof(f32), d.pos);
	d.pos += sizeof(f32);
    }
    SV_INLINE void deserialize_f64(Deserializer& d, f64& n)
    {
	s.buff.read_safe(&n, sizeof(f64), d.pos);
	d.pos += sizeof(f64);
    }

    SV_INLINE void deserialize_char(Deserializer& d, char& n)
    {
	s.buff.read_safe(&n, sizeof(char), d.pos);
	d.pos += sizeof(char);
    }
    SV_INLINE void deserialize_bool(Deserializer& d, bool& n)
    {
	s.buff.read_safe(&n, sizeof(bool), d.pos);
	d.pos += sizeof(bool);
    }

    SV_INLINE void deserialize_string_size(Deserializer& d, size_t& size)
    {
	const char* str = (const char*)(d.buff.data() + d.pos);
	size = strlen(str);
    }
    SV_INLINE void deserialize_string(Deserializer& d, char* str, size_t size)
    {
	memcpy(str, d.buff.data(), size + 1u);
	d.pos += size + 1u;
    }

    SV_INLINE void deserialize_v2_f32(Deserializer& d, v2_f32& v)
    {
	deserialize_f32(d, v.x);
	deserialize_f32(d, v.y);
    }
    SV_INLINE void deserialize_v3_f32(Deserializer& d, v3_f32& v)
    {
	deserialize_f32(d, v.x);
	deserialize_f32(d, v.y);
	deserialize_f32(d, v.z);
    }
    SV_INLINE void deserialize_v4_f32(Deserializer& d, v4_f32& v)
    {
	deserialize_f32(d, v.x);
	deserialize_f32(d, v.y);
	deserialize_f32(d, v.z);
	deserialize_f32(d, v.w);
    }

    SV_INLINE void deserialize_version(Deserializer& d, Version n)
    {
	s.buff.read_safe(&n, sizeof(Version), d.pos);
	d.pos += sizeof(Version);
    }    
    
}
