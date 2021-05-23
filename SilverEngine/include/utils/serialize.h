#pragma once

#include "defines.h"
#include "utils/allocators.h"
#include "utils/math.h"
#include "utils/string.h"

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
	static constexpr u32 VERSION = 0u;
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
    SV_INLINE void serialize_size_t(Serializer& s, size_t n)
    {
	u64 n0 = u64(n);
	s.buff.write_back(&n0, sizeof(u64));
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

    SV_INLINE void serialize_color(Serializer& s, Color n)
    {
	s.buff.write_back(&n, sizeof(Color));
    }

    SV_INLINE void serialize_xmmatrix(Serializer& s, const XMMATRIX& n)
    {
	s.buff.write_back(&n, sizeof(XMMATRIX));
    }

    SV_INLINE void serialize_string(Serializer& s, const char* str)
    {
	size_t len = strlen(str) + 1u;
	s.buff.reserve(len);
	s.buff.write_back(str, len);
    }
    SV_INLINE void serialize_string(Serializer& s, const String& str)
    {
	const char* st = str.c_str();

	if (st)
	    serialize_string(s, st);
	else {

	    char c = '\0';
	    serialize_string(s, &c);
	}
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

    SV_INLINE void serialize_v2_f32_array(Serializer& s, const v2_f32* v, u32 count)
    {
	s.buff.reserve(sizeof(f32) * 2u * count + sizeof(u32));

	serialize_u32(s, count);

	const v2_f32* end = v + size_t(count);

	while (v != end)  {
	
	    serialize_f32(s, v->x);
	    serialize_f32(s, v->y);
	    ++v;
	}
    }
    SV_INLINE void serialize_v3_f32_array(Serializer& s, const v3_f32* v, u32 count)
    {
	s.buff.reserve(sizeof(f32) * 3u * count + sizeof(u32));

	serialize_u32(s, count);

	const v3_f32* end = v + size_t(count);

	while (v != end)  {
	
	    serialize_f32(s, v->x);
	    serialize_f32(s, v->y);
	    serialize_f32(s, v->z);
	    ++v;
	}
    }
    SV_INLINE void serialize_v4_f32_array(Serializer& s, const v4_f32* v, u32 count)
    {
	s.buff.reserve(sizeof(f32) * 4u * count + sizeof(u32));

	serialize_u32(s, count);

	const v4_f32* end = v + size_t(count);

	while (v != end)  {
	
	    serialize_f32(s, v->x);
	    serialize_f32(s, v->y);
	    serialize_f32(s, v->z);
	    serialize_f32(s, v->w);
	    ++v;
	}
    }

    SV_INLINE void serialize_u32_array(Serializer& s, const u32* n, u32 count)
    {
	s.buff.reserve(sizeof(u32) * (count + 1u));
	serialize_u32(s, count);

	foreach(i, count)
	    serialize_u32(s, n[i]);
    }

    SV_INLINE void serialize_v2_f32_array(Serializer& s, const List<v2_f32>& list)
    {
	serialize_v2_f32_array(s, list.data(), (u32)list.size());
    }
    SV_INLINE void serialize_v3_f32_array(Serializer& s, const List<v3_f32>& list)
    {
	serialize_v3_f32_array(s, list.data(), (u32)list.size());
    }
    SV_INLINE void serialize_v4_f32_array(Serializer& s, const List<v4_f32>& list)
    {
	serialize_v4_f32_array(s, list.data(), (u32)list.size());
    }

    SV_INLINE void serialize_u32_array(Serializer& s, const List<u32>& list)
    {
	serialize_u32_array(s, list.data(), (u32)list.size());
    }

    SV_INLINE void serialize_version(Serializer& s, Version n)
    {
	s.buff.write_back(&n, sizeof(Version));
    }

    /////////////////////////// DESERIALIZER //////////////////////////////////

    struct Deserializer {
	static constexpr u32 LAST_VERSION_SUPPORTED = 0u;
	u32 serializer_version;
	Version engine_version;
	RawList buff;
	size_t pos;
    };

    SV_API bool deserialize_begin(Deserializer& d, const char* filepath);
    SV_API void deserialize_end(Deserializer& d);

    SV_INLINE bool deserialize_assert(Deserializer& d, size_t size)
    {
	return (d.pos + size) <= d.buff.size();
    }

    SV_INLINE void deserialize_u8(Deserializer& d, u8& n)
    {
	d.buff.read_safe(&n, sizeof(u8), d.pos);
	d.pos += sizeof(u8);
    }
    SV_INLINE void deserialize_u16(Deserializer& d, u16& n)
    {
	d.buff.read_safe(&n, sizeof(u16), d.pos);
	d.pos += sizeof(u16);
    }
    SV_INLINE void deserialize_u32(Deserializer& d, u32& n)
    {
	d.buff.read_safe(&n, sizeof(u32), d.pos);
	d.pos += sizeof(u32);
    }
    SV_INLINE void deserialize_u64(Deserializer& d, u64& n)
    {
	d.buff.read_safe(&n, sizeof(u64), d.pos);
	d.pos += sizeof(u64);
    }
    SV_INLINE void deserialize_size_t(Deserializer& d, size_t& n)
    {
	u64 n0;
	d.buff.read_safe(&n0, sizeof(u64), d.pos);
	d.pos += sizeof(u64);
	n = size_t(n0);
    }

    SV_INLINE void deserialize_i8(Deserializer& d, i8& n)
    {
	d.buff.read_safe(&n, sizeof(i8), d.pos);
	d.pos += sizeof(i8);
    }
    SV_INLINE void deserialize_i16(Deserializer& d, i16& n)
    {
	d.buff.read_safe(&n, sizeof(i16), d.pos);
	d.pos += sizeof(i16);
    }
    SV_INLINE void deserialize_i32(Deserializer& d, i32& n)
    {
	d.buff.read_safe(&n, sizeof(i32), d.pos);
	d.pos += sizeof(i32);
    }
    SV_INLINE void deserialize_i64(Deserializer& d, i64& n)
    {
	d.buff.read_safe(&n, sizeof(u64), d.pos);
	d.pos += sizeof(u64);
    }

    SV_INLINE void deserialize_f32(Deserializer& d, f32& n)
    {
	d.buff.read_safe(&n, sizeof(f32), d.pos);
	d.pos += sizeof(f32);
    }
    SV_INLINE void deserialize_f64(Deserializer& d, f64& n)
    {
	d.buff.read_safe(&n, sizeof(f64), d.pos);
	d.pos += sizeof(f64);
    }

    SV_INLINE void deserialize_char(Deserializer& d, char& n)
    {
	d.buff.read_safe(&n, sizeof(char), d.pos);
	d.pos += sizeof(char);
    }
    SV_INLINE void deserialize_bool(Deserializer& d, bool& n)
    {
	d.buff.read_safe(&n, sizeof(bool), d.pos);
	d.pos += sizeof(bool);
    }

    SV_INLINE void deserialize_color(Deserializer& d, Color& n)
    {
	d.buff.read_safe(&n, sizeof(Color), d.pos);
	d.pos += sizeof(Color);
    }

    SV_INLINE void deserialize_xmmatrix(Deserializer& d, XMMATRIX& n)
    {
	d.buff.read_safe(&n, sizeof(XMMATRIX), d.pos);
	d.pos += sizeof(XMMATRIX);
    }

    SV_INLINE size_t deserialize_string_size(Deserializer& d)
    {
	size_t size;
	const char* str = (const char*)(d.buff.data() + d.pos);
	size = strlen(str);
	return size;
    }
    SV_INLINE void deserialize_string(Deserializer& d, char* str, size_t buff_size)
    {
	size_t size = deserialize_string_size(d);

	size_t read = SV_MIN(size, buff_size - 1u);
	memcpy(str, d.buff.data() + d.pos, read);
	str[read] = '\0';
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

    SV_INLINE void deserialize_v2_f32_array(Deserializer& d, List<v2_f32>& list)
    {
	u32 count;
	deserialize_u32(d, count);

	list.resize(count);

	foreach(i, count)  {

	    v2_f32& v = list[i];
	    deserialize_f32(d, v.x);
	    deserialize_f32(d, v.y);
	}
    }
    SV_INLINE void deserialize_v3_f32_array(Deserializer& d, List<v3_f32>& list)
    {
	u32 count;
	deserialize_u32(d, count);

	list.resize(count);

	foreach(i, count)  {

	    v3_f32& v = list[i];
	    deserialize_f32(d, v.x);
	    deserialize_f32(d, v.y);
	    deserialize_f32(d, v.z);
	}
    }
    SV_INLINE void deserialize_v4_f32_array(Deserializer& d, List<v4_f32>& list)
    {
	u32 count;
	deserialize_u32(d, count);

	list.resize(count);

	foreach(i, count)  {

	    v4_f32& v = list[i];
	    deserialize_f32(d, v.x);
	    deserialize_f32(d, v.y);
	    deserialize_f32(d, v.z);
	    deserialize_f32(d, v.w);
	}
    }

    SV_INLINE void deserialize_u32_array(Deserializer& d, List<u32>& list)
    {
	u32 count;
	deserialize_u32(d, count);

	list.resize(count);

	foreach(i, count)  {
	    
	    deserialize_u32(d, list[i]);
	}
    }

    SV_INLINE void deserialize_version(Deserializer& d, Version& n)
    {
	d.buff.read_safe(&n, sizeof(Version), d.pos);
	d.pos += sizeof(Version);
    }    
    
}
