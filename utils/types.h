#pragma once

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct half {
	union {
		short i;
	};
};

typedef half 			 	f16;
typedef float 			 	f32;
typedef double				f64;

#ifdef _WIN64

typedef i64					iword;
typedef u64					uword;
typedef i64					iw;
typedef u64					uw;

#else

typedef i32 				iword;
typedef u32					uword;
typedef i32					iw;
typedef u32					uw;

#endif

#ifdef _MSC_VER
typedef TCHAR				tchar;
#endif
typedef void*				pvoid;

enum TypeId {
	T_NONE = 0
	, T_BIT
	, T_INT8
	, T_SINT8 = T_INT8
	, T_I8 = T_INT8
	, T_UINT8
	, T_U8 = T_UINT8
	, T_INT16
	, T_SINT16 = T_INT16
	, T_I16 = T_INT16
	, T_UINT16
	, T_U16 = T_UINT16
	, T_INT32
	, T_SINT32 = T_INT32
	, T_I32 = T_INT32
	, T_UINT32
	, T_U32 = T_UINT32
	, T_INT64
	, T_SINT64 = T_INT64
	, T_I64 = T_INT64
	, T_UINT64
	, T_U64 = T_UINT64
	, T_FLOAT16
	, T_FLOAT2 = T_FLOAT16
	, T_F16 = T_FLOAT16
	, T_FLOAT32
	, T_FLOAT4 = T_FLOAT32
	, T_F32 = T_FLOAT32
	, T_FLOAT64
	, T_FLOAT8 = T_FLOAT64
	, T_F64 = T_FLOAT64

	, T_TYPES_COUNT
};

inline uword size_of(TypeId const type_id) {
	static const uword sc_sizes[T_TYPES_COUNT] = {
		0
		, 0
		, 1
		, 1
		, 2
		, 2
		, 4
		, 4
		, 8
		, 8
		, 2
		, 4
		, 8
	};

	return sc_sizes[type_id];
};

inline uword bits_size_of(TypeId const type_id) {
	static const uword sc_sizes[T_TYPES_COUNT] = {
		0
		, 1
		, 8
		, 8
		, 16
		, 16
		, 32
		, 32
		, 64
		, 64
		, 16
		, 32
		, 64
	};

	return sc_sizes[type_id];
}

struct type_traits_base {
	enum {
		bits_per_byte = 8
	};
};

template <class T> struct type_traits : public type_traits_base {
};

template <> struct type_traits<i8> : public type_traits_base {
	typedef i8 value_type;

	enum {
		value_type_id = T_I8,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<u8> : public type_traits_base {
	typedef u8 value_type;

	enum {
		value_type_id = T_U8,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<i16> : public type_traits_base {
	typedef i16 value_type;

	enum {
		value_type_id = T_I16,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<u16> : public type_traits_base {
	typedef u16 value_type;

	enum {
		value_type_id = T_U16,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<i32> : public type_traits_base {
	typedef i32 value_type;

	enum {
		value_type_id = T_I32,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<u32> : public type_traits_base {
	typedef u32 value_type;

	enum {
		value_type_id = T_U32,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<i64> : public type_traits_base {
	typedef i64 value_type;

	enum {
		value_type_id = T_I64,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<u64> : public type_traits_base {
	typedef u64 value_type;

	enum {
		value_type_id = T_U64,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<f16> : public type_traits_base {
	typedef f16 value_type;

	enum {
		value_type_id = T_F16,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<f32> : public type_traits_base {
	typedef f32 value_type;

	enum {
		value_type_id = T_F32,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

template <> struct type_traits<f64> : public type_traits_base {
	typedef f64 value_type;

	enum {
		value_type_id = T_F64,
		value_size = sizeof(value_type),
		value_size_bits = value_size * bits_per_byte
	};
};

typedef signed int HR;

enum MxError {
	MX_OK = 0
	, MX_VBO_NOT_FOUND
	, MX_IBO_NOT_FOUND
};

#ifdef _MSC_VER

template <class T> class strt : public std::basic_string<T> {
public:
	typedef typename std::basic_string<T> xstr;
	typedef typename xstr::const_pointer const_pointer;
	typedef typename xstr::const_iterator const_iterator;
	typedef typename xstr::value_type value_type;
	typedef typename xstr::size_type size_type;

public:
	//operator std::string() { return ; }
	template<typename... A> strt(A&&... a) : xstr(std::forward<A>(a)...) {}
	xstr& operator = (xstr const& s) { return *this; }
	xstr& operator = (xstr&& s) { return *this = std::move(s); }

	bool ciequal(const value_type* s) const {
		return std::toupper(*this) == std::toupper(xstr(s)); // todo: make through uint*_t
	}

	bool ciequal(const strt& s) const {
		return std::toupper(*this) == std::toupper(s); // todo: make through uint*_t
	}

	bool ciequal(const xstr& s) const {
		return std::toupper(*this) == std::toupper(s); // todo: make through uint*_t
	}

	int replace(const xstr& f, const xstr& t) {
		if (!f.size())
			return 0;

		if (t.size() > f.size()) {
			size_t newsize = strt<T>::size(), dif = t.size() - f.size();
			for (size_t i=find(f); i!=strt<T>::npos; i=find(f, i+f.size()), newsize+=dif);
			strt<T>::reserve(newsize);
		}

		int count = 0;
		for (size_t i=find(f); i!=strt<T>::npos; xstr::replace(i, f.size(), t), i=find(f, i+t.size()), ++count);
		return count;
	}
};

class bstr : public strt<char> {
public:
	typedef strt<char> xstrt;
	typedef xstrt::xstr xstr;
	typedef xstrt::const_pointer const_pointer;
	typedef xstrt::const_iterator const_iterator;
	typedef xstrt::value_type value_type;
	typedef xstrt::size_type size_type;

public:
	template <typename... A> bstr(A&&... a) : xstrt(std::forward<A>(a)...) {}
	bstr& operator = (bstr const& s) { return *this; }
	bstr& operator = (bstr&& s) { return *this = std::move(s); }

	strt& format(const value_type* _Ptr, ...) {
		va_list lst;
		va_start(lst, _Ptr);
		size_t sz = _vscprintf(_Ptr, lst);
		reserve(sz+1);
		resize(sz);
		sz = _vsnprintf_s((value_type*)data(), capacity(), size(), _Ptr, lst);
		va_end(lst);

		if (sz <= 0) {
			resize(0);
		} else if (sz != size()) {
			resize(sz);
		}

		return *this;
	}
};

class wstr : public strt<wchar_t> {
public:
	typedef strt<wchar_t> xstrt;
	typedef xstrt::xstr xstr;
	typedef xstrt::const_pointer const_pointer;
	typedef xstrt::const_iterator const_iterator;
	typedef xstrt::value_type value_type;
	typedef xstrt::size_type size_type;

public:
	template <typename... A> wstr(A&&... a) : xstrt(std::forward<A>(a)...) {}
	wstr& operator = (wstr const& s) { return *this; }
	wstr& operator = (wstr&& s) { return *this = std::move(s); }

	strt& format(const value_type* _Ptr, ...) {
		va_list lst;
		va_start(lst, _Ptr);
		size_t sz = _vscwprintf(_Ptr, lst);
		reserve(sz+1);
		resize(sz);
		_vsnwprintf_s((value_type*)data(), capacity(), size(), _Ptr, lst);
		va_end(lst);

		if (sz <= 0) {
			resize(0);
		} else if (sz != size()) {
			resize(sz);
		}

		return *this;
	}
};

typedef bstr& bstrref;
typedef const bstr& cbstrref;
typedef const bstr cbstr;
typedef bstr const * cbstrptr;
typedef bstr * const bstrcptr;
typedef bstr const * const cbstrcptr;
typedef std::vector<bstr> bstrs;
typedef std::vector<bstr> vbstr;
typedef std::list<bstr> lbstr;

typedef wstr& wstrref;
typedef const wstr& cwstrref;
typedef const wstr cwstr;
typedef wstr const * cwstrptr;
typedef wstr * const wstrcptr;
typedef wstr const * const cwstrcptr;
typedef std::vector<wstr> wstrs;
typedef std::vector<wstr> vwstr;
typedef std::list<wstr> lwstr;

char const * const cstr(cbstrref str) { return str.c_str(); }
wchar_t const * const cstr(cwstrref str) { return str.c_str(); }

#endif

typedef std::string cstringref; // TODO: remove

#ifdef _UNICODE
typedef std::wstring tstr;
#else
typedef std::string tstr;
#endif

typedef tstr& tstrref;
typedef const tstr& ctstrref;
typedef const tstr ctstr;
typedef tstr const * ctstrptr;
typedef tstr * const tstrcptr;
typedef tstr const * const ctstrcptr;
typedef std::vector<tstr> tstrs;
typedef std::vector<tstr> vtstr;
typedef std::list<tstr> ltstr;

inline char const* const cstr(char const* const str) { return str; }
inline char const* const cstr(const std::string& str) { return str.c_str(); }
inline wchar_t const* const cstr(wchar_t const* const str) { return str; }
inline wchar_t const* const cstr(const std::wstring& str) { return str.c_str(); }

#define _def_true
#define _def_false
#define _def_zero
#define _def_minus_1

#ifdef _DEBUG
#define VRFY(a) assert(a)
#else
#define VRFY(a) ((void)(a))
#endif

#define self (*this)