#pragma once

#include "defines.h"

namespace sv {

    struct SV_API Archive {

	Archive();
	~Archive();

	void reserve(size_t size);
	void write(const void* data, size_t size);
	void read(void* data, size_t size);
	void erase(size_t size);
		
	SV_INLINE size_t size() const noexcept { return _size; }
	SV_INLINE void clear() noexcept { if (_size) _size = sizeof(Version); }
	SV_INLINE void startRead() noexcept { _pos = sizeof(Version); }

	bool openFile(const char* filePath);
	bool saveFile(const char* filePath, bool append = false);

	SV_INLINE Archive& operator<<(char n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(bool n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u8 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u16 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i8 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i16 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(f32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(f64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Version& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT2& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT3& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT4& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMMATRIX& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Color& n) { write(&n, sizeof(n)); return *this; }

	template<typename T, typename F>
	    SV_INLINE Archive& operator<<(const Vector2D<T, F>& n) { write(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	    SV_INLINE Archive& operator<<(const Vector3D<T, F>& n) { write(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	    SV_INLINE Archive& operator<<(const Vector4D<T, F>& n) { write(&n, sizeof(n)); return *this; }

	template<typename T>
	    inline Archive& operator<<(const std::vector<T>& vec)
	{
	    size_t size = vec.size();
	    write(&size, sizeof(size_t));

	    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
		this->operator<<(*it);
	    }
	    return *this;
	}

	inline Archive& operator<<(const char* txt)
	{
	    size_t txtLength = strlen(txt);
	    write(&txtLength, sizeof(size_t));
	    write(txt, txtLength);
	    return *this;
	}

	inline Archive& operator<<(const std::string& str)
	{
	    size_t txtLength = str.size();
	    write(&txtLength, sizeof(size_t));
	    write(str.data(), txtLength);
	    return *this;
	}

	// READ

	SV_INLINE Archive& operator>>(bool& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(char& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u8& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u16& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i8& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i16& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(f32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(f64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Version& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT2& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT3& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT4& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMMATRIX& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Color& n) { read(&n, sizeof(n)); return *this; }

	template<typename T, typename F>
	    SV_INLINE Archive& operator>>(Vector2D<T, F>& n) { read(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	    SV_INLINE Archive& operator>>(Vector3D<T, F>& n) { read(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	    SV_INLINE Archive& operator>>(Vector4D<T, F>& n) { read(&n, sizeof(n)); return *this; }

	template<typename T>
	    Archive& operator>>(std::vector<T>& vec)
	{
	    size_t size;
	    read(&size, sizeof(size_t));
	    size_t lastSize = vec.size();
	    vec.resize(lastSize + size);
	    read(vec.data() + lastSize, sizeof(T) * size);
	    return *this;
	}
	
	Archive& operator>>(std::string& str)
	{
	    size_t txtLength;
	    read(&txtLength, sizeof(size_t));
	    str.resize(txtLength);
	    read((void*)str.data(), txtLength);
	    return *this;
	}

	Archive& operator>>(char* str)
	{
	    size_t txtLength;
	    read(&txtLength, sizeof(size_t));
	    read((void*)str, txtLength);
	    str[txtLength] = '\0';
	    return *this;
	}

	size_t _pos;
	size_t _size;
	size_t _capacity;
	u8* _data;
	Version version;

    private:
	void allocate(size_t size);
	void free();

    };

    //TODO: move
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

    
}
