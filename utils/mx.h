const size_t MX_VERSION = 0x00010003;

#if defined _MSC_VER
#	if defined WIN32
#		define USE_ASSEMBLY
#		define USE_INTRINSICS
#	elif defined WIN64
#		define USE_INTRINSICS
#	else
#		error Unknown windows platform
#	endif
#else
#	define USE_INTRINSICS
#endif

namespace mx {

#define PERM2(type, a, b) type a##b() const { return type(a, b); }
#define PERMS2(type, x, y) PERM2(type, x, x) PERM2(type, x, y) PERM2(type, y, x) PERM2(type, y, y)

#define PERM3(type, a, b, c) type a##b##c() const { return type(a, b, c); }
#define PERMS31(type, a, b) PERM3(type, a, b, x) PERM3(type, a, b, y) PERM3(type, a, b, z)
#define PERMS32(type, a) PERMS31(type, a, x) PERMS31(type, a, y) PERMS31(type, a, z)
#define PERMS3(type, x, y, z) PERMS32(type, x) PERMS32(type, y) PERMS32(type, z)

#define PERM4(type, a, b, c, d) type a##b##c##d() const { return type(a, b, c, d); }
#define PERMS41(type, a, b, c) PERM4(type, a, b, c, x) PERM4(type, a, b, c, y) PERM4(type, a, b, c, z) PERM4(type, a, b, c, w)
#define PERMS42(type, a, b) PERMS41(type, a, b, x) PERMS41(type, a, b, y) PERMS41(type, a, b, z) PERMS41(type, a, b, w)
#define PERMS43(type, a) PERMS42(type, a, x) PERMS42(type, a, y) PERMS42(type, a, z) PERMS42(type, a, w)
#define PERMS4(type, x, y, z, w) PERMS43(type, x) PERMS43(type, y) PERMS43(type, z) PERMS43(type, w)

#if defined _MSC_VER
#define mx_inline __forceinline
#define mx_fastcall __fastcall
#define mx_align16 __declspec(align(32))
#else
#define mx_inline
#define mx_fastcall
#define mx_align16
#endif

const f64 pi = 3.14159265358979323846264338327950288419716939967511;
const f64 eps = 1E-6;
const f64 DEG2RAD = pi / 180.0;
const f64 RAD2DEG = 180.0 / pi;

const f64 KG2LB					= 2.204622622;
const f64 LB2KB					= 1.0 / KG2LB;
const f64 SLUG2KG				= 14.59;
const f64 KG2SLUG				= 1.0 / SLUG2KG;

const f64 NM2M					= 1852;
const f64 M2NM					= 1.0 / NM2M;
const f64 NM2KM					= NM2M * 0.001;
const f64 KM2NM					= 1.0 / NM2KM;
const f64 FT2M					= 0.3048;
const f64 M2FT					= 1.0 / FT2M;
const f64 FT2NM					= FT2M * M2NM;
const f64 NM2FT					= 1.0 / FT2M;

const f64 KTS2KMH				= NM2KM;
const f64 KMH2KTS				= 1.0 / KTS2KMH;
const f64 MPS2KMH				= 3.6;
const f64 KMH2MPS				= 1.0 / MPS2KMH;
const f64 FPM2KTS				= FT2M * 60.0 * M2NM;
const f64 KTS2FPM				= 1.0 / FPM2KTS;
const f64 FPS2KTS				= FT2M * 3600.0 * M2NM;
const f64 KTS2FPS				= 1.0 / FPS2KTS;
const f64 KTS2MPS				= KTS2KMH * KMH2MPS;
const f64 MPS2KTS				= 1.0 / KTS2MPS;
const f64 FPM2MPS				= FT2M / 60.0;
const f64 MPS2FPM				= 1.0 / FPM2MPS;


mx_inline f32 deg2rad(const f32 deg) { return deg*f32(DEG2RAD); }
mx_inline f64 deg2rad(const f64 deg) { return deg*DEG2RAD; }
mx_inline f32 rad2deg(const f32 rad) { return rad*f32(RAD2DEG); }
mx_inline f64 rad2deg(const f64 rad) { return rad*RAD2DEG; }

template <typename T> mx_inline T abs(const T a) { return a>=T(0) ? a : -a; }
template <typename T> mx_inline T _min(const T a, const T b) { return a<b ? a : b; }
template <typename T> mx_inline T _max(const T a, const T b) { return a>b ? a : b; }
template <typename T> mx_inline T pwr3(const T a) { return a*a*a; }
template <typename T> mx_inline T signum(const T a) { return abs(a)==T(0) ? T(0) : a>T(0) ? T(1) : T(-1); }

mx_inline bool is_zero(const f32 a) { return abs(a) < f32(eps); }
mx_inline bool is_zero(const f64 a) { return abs(a) < f64(eps); }

mx_inline bool is_not_zero(const f32 a) { return abs(a) >= f32(eps); }
mx_inline bool is_not_zero(const f64 a) { return abs(a) >= f64(eps); }

mx_inline bool is_one(const f32 a) { return abs(a-f32(1.0)) < f32(eps); }
mx_inline bool is_one(const f64 a) { return abs(a-f64(1.0)) < f64(eps); }

mx_inline bool is_equal(const f32 a, const f32 b) { return abs(a-b) < f32(eps); }
mx_inline bool is_equal(const f64 a, const f64 b) { return abs(a-b) < eps; }

mx_inline bool is_not_equal(const f32 a, const f32 b) { return abs(a-b) >= f32(eps); }
mx_inline bool is_not_equal(const f64 a, const f64 b) { return abs(a-b) >= eps; }

template <typename T>
class vec2t : public type_traits<T> {
public:
	typedef T type;
	typedef vec2t<T> v2;

public:
	vec2t() {}
	vec2t(const type s) : x(s), y(s) {}
	vec2t(const type a, const type b) : x(a), y(b) {}
	vec2t(const v2& v) : x(v.x), y(v.y) {}

	type operator ! () const { return sqrt(x*x+y*y); }
	type rlength() const { return T(1.0)/sqrt(x*x+y*y); }

	v2& operator = (const v2& v) { x = v.x; y = v.y; return *this; }

	v2& operator += (const v2& v) { x += v.x; y += v.y; return *this; }
	v2& operator -= (const v2& v) { x -= v.x; y -= v.y; return *this; }
	v2& operator *= (const v2& v) { x *= v.x; y *= v.y; return *this; }

	v2 operator - () const { return v2(-x, -y); }
	v2 operator ~ () const { type l = T(1.0)/sqrt(x*x+y*y); return v2(x*l, y*l); }
	v2 operator + (const v2& v) const { return v2(x+v.x, y+v.y); }
	v2 operator - (const v2& v) const { return v2(x-v.x, y-v.y); }
	v2 operator * (const v2& v) const { return v2(x*v.x, y*v.y); }
	v2 operator / (const v2& v) const { return v2(x/v.x, y/v.y); }

	v2& operator += (const type& a) { x+=a; y+=a; return *this; }
	v2& operator -= (const type& a) { x-=a; y-=a; return *this; }
	v2& operator *= (const type& a) { x*=a; y*=a; return *this; }
	v2& operator /= (const type& a) { x/=a; y/=a; return *this; }

	v2  operator  + (const type& a) { return v2(x+a, y+a); }
	v2  operator  - (const type& a) { return v2(x-a, y-a); }
	template <typename T> v2  operator  * (const T& a) { return v2(x*a, y*a); }
	v2  operator  / (const type& a) { return v2(x/a, y/a); }

	friend v2 operator + (const type& a, const v2& v) { return v2(a+v.x, a+v.y); }
	friend v2 operator - (const type& a, const v2& v) { return v2(a-v.x, a-v.y); }
	friend v2 operator * (const type& a, const v2& v) { return v2(a*v.x, a*v.y); }
	friend v2 operator / (const type& a, const v2& v) { return v2(a/v.x, a/v.y); }

	operator const type* () const { return &x; }
	operator type* () { return &x; }

	v2 deg2rad() const { return v2(deg2rad(x), deg2rad(y)); }
	v2 rad2deg() const { return v2(rad2deg(x), rad2deg(y)); }
	v2 sin() const { return v2(sin(x), sin(y)); }
	v2 cos() const { return v2(cos(x), cos(y)); }

	PERMS2(v2, x, y);
	PERMS2(v2, s, t);

public:
	mx_align16 union {
		struct { type x, y; };
		struct { type s, t; };
		struct { type _0, _1; };
		type a[2];
	};
};

typedef vec2t<f32> vec2f;
typedef vec2t<f64> vec2d;
typedef vec2t<f32> v2f;
typedef vec2t<f64> v2d;

///

void mx_fastcall add_v3f_v3f(f32* const d, const f32* const u, const f32* const v) {
	d[0] = u[0] + v[0];
	d[1] = u[1] + v[1];
	d[2] = u[2] + v[2];
}

void mx_fastcall sub_v3f_v3f(f32* const d, const f32* const u, const f32* const v) {
	d[0] = u[0] - v[0];
	d[1] = u[1] - v[1];
	d[2] = u[2] - v[2];
}

template <typename T>
class vec3t : public type_traits<T> {
public:
	typedef T type;
	typedef vec2t<T> v2;
	typedef vec3t<T> v3;

public:
	vec3t() {}
	vec3t(const type s) : x(s), y(s), z(s) {}
	vec3t(const type _x, const type _y, const type _z) : x(_x), y(_y), z(_z) {}
	vec3t(const vec2t<T>& v, const type _z = T(0.0)) : x(v.x), y(v.y), z(_z) {}
	vec3t(const v3& u) : x(u.x), y(u.y), z(u.z) {}

	mx_inline type operator ! () const { return sqrt(x*x+y*y+z*z); }
	mx_inline type rlength() const { return T(1.0)/sqrt(x*x+y*y+z*z); }

	mx_inline v3& operator  = (const type s) { x=s; y=s; z=s; return *this; }
	mx_inline v3& operator  = (const v3& u) { x=u.x; y=u.y; z=u.z; return *this; }
	mx_inline v3& operator += (const v3& u) { x+=u.x; y+=u.y; z+=u.z; return *this; }
	mx_inline v3& operator -= (const v3& u) { x-=u.x; y-=u.y; z-=u.z; return *this; }
	mx_inline v3& operator *= (const v3& u) { x*=u.x; y*=u.y; z*=u.z; return *this; }
	mx_inline v3& operator /= (const v3& u) { x/=u.x; y/=u.y; z/=u.z; return *this; }
	mx_inline v3& operator ^= (const v3& u) { *this = *this ^ u; return *this; }
	mx_inline v3  operator  - () const { return v3(-x, -y, -z); }
	mx_inline v3  operator  ~ () const { type l = T(1.0)/sqrt(x*x+y*y+z*z); return v3(x*l, y*l, z*l); }
	mx_inline v3  operator  + (const v3& u) { return v3(x+u.x, y+u.y, z+u.z); }
	mx_inline v3  operator  - (const v3& u) { return v3(x-u.x, y-u.y, z-u.z); }
	mx_inline v3  operator  * (const v3& u) { return v3(x*u.x, y*u.y, z*u.z); }
	mx_inline v3  operator  / (const v3& u) { return v3(x/u.x, y/u.y, z/u.z); }
	mx_inline v3  operator  ^ (const v3& u) { return v3(y*u.z-z*u.y, z*u.x-x*u.z, x*u.y-y*u.x); }
	mx_inline type operator | (const v3& v) { return x*v.x+y*v.y+z*v.z; }

	mx_inline v3& operator += (const type& a) { x+=a; y+=a; z+=a; return *this; }
	mx_inline v3& operator -= (const type& a) { x-=a; y-=a; z-=a; return *this; }
	mx_inline v3& operator *= (const type& a) { x*=a; y*=a; z*=a; return *this; }
	mx_inline v3& operator /= (const type& a) { x/=a; y/=a; z/=a; return *this; }

	mx_inline v3  operator +  (const type& a) { return v3(x+a, y+a, z+a); }
	mx_inline v3  operator -  (const type& a) { return v3(x-a, y-a, z-a); }
	mx_inline v3  operator *  (const type& a) { return v3(x*a, y*a, z*a); }
	mx_inline v3  operator /  (const type& a) { return v3(x/a, y/a, z/a); }

	template <typename S> mx_inline friend v3 operator + (const S a, const v3& v) { return v3(a+v.x, a+v.y, a+v.z); }
	template <typename S> mx_inline friend v3 operator - (const S a, const v3& v) { return v3(a-v.x, a-v.y, a-v.z); }
	template <typename S> mx_inline friend v3 operator * (const S a, const v3& v) { return v3(a*v.x, a*v.y, a*v.z); }
	template <typename S> mx_inline friend v3 operator / (const S a, const v3& v) { return v3(a/v.x, a/v.y, a/v.z); }

	mx_inline operator v2() const { return v2(x, y); }
	mx_inline operator const type* () const { return &x; }
	mx_inline operator type* () { return &x; }

	PERMS3(v3, x, y, z);
	PERMS3(v3, r, g, b);

	v3 deg2rad() const { return v3(deg2rad(x), deg2rad(y), deg2rad(z)); }
	v3 rad2deg() const { return v3(rad2deg(x), rad2deg(y), rad2deg(z)); }
	v3 sin() const { return v3(sin(x), sin(y), sin(z)); }
	v3 cos() const { return v3(cos(x), cos(y), cos(z)); }

public:
	mx_align16 union {
		struct { type x, y, z; };
		struct { type r, g, b; };
		struct { type _0, _1, _2; };
		type a[3];
	};
};

template <> vec3t<f32>& vec3t<f32>::operator += (const vec3t<f32>& u) { add_v3f_v3f(&this->x, &this->x, &u.x); return *this; }
template <> vec3t<f32>& vec3t<f32>::operator -= (const vec3t<f32>& u) { x-=u.x; y-=u.y; z-=u.z; return *this; }
template <> vec3t<f32>& vec3t<f32>::operator *= (const vec3t<f32>& u) { x*=u.x; y*=u.y; z*=u.z; return *this; }
template <> vec3t<f32> vec3t<f32>::operator + (const vec3t<f32>& u) { vec3t<f32> r; add_v3f_v3f(r, *this, u); return r; }
template <> vec3t<f32> vec3t<f32>::operator - (const vec3t<f32>& u) { return vec3t<f32>(x-u.x, y-u.y, z-u.z); }
template <> vec3t<f32> vec3t<f32>::operator * (const vec3t<f32>& u) { return vec3t<f32>(x*u.x, y*u.y, z*u.z); }

template <> vec3t<f64>& vec3t<f64>::operator += (const vec3t<f64>& u) { x+=u.x; y+=u.y; z+=u.z; return *this; }
template <> vec3t<f64> vec3t<f64>::operator + (const vec3t<f64>& u) { return vec3t<f64>(x+u.x, y+u.y, z+u.z); }

vec3t<f64> operator + (const vec3t<f32>& u, const vec3t<f64>& v) { return vec3t<f64>(u.x+v.x, u.y+v.y, u.z+v.z); }
vec3t<f64> operator + (const vec3t<f64>& u, const vec3t<f32>& v) { return vec3t<f64>(u.x+v.x, u.y+v.y, u.z+v.z); }

typedef vec3t<f32> vec3f;
typedef vec3t<f64> vec3d;
typedef vec3t<f32> v3f;
typedef vec3t<f64> v3d;

///

template <typename T>
class vec4t : public type_traits<T> {
public:
	typedef T type;
	typedef vec2t<type> v2;
	typedef vec3t<type> v3;
	typedef vec4t<type> v4;

public:
	vec4t() {}
	vec4t(const type s) : x(s), y(s), z(s), w(static_cast<type>(1.0)) {}
	vec4t(const type _x, const type _y, const type _z, const type _w=T(1.0)) : x(_x), y(_y), z(_z), w(_w) {}
	vec4t(const vec2t<T>& v, const type _z=T(0.0), const type _w=T(1.0)) : x(v.x), y(v.y), z(_z), w(_w) {}
	vec4t(const vec3t<T>& v, const type _w=T(1.0)) : x(v.x), y(v.y), z(v.z), w(_w) {}
	vec4t(const v4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	mx_inline type operator ! () const { return sqrt(x*x+y*y+z*z); }
	mx_inline type rlength() const { return T(1.0)/sqrt(x*x+y*y+z*z); }

	mx_inline v4  operator  - () const { return v4(-x, -y, -z, w); }
	mx_inline v4  operator  ~ () const { type l = T(1.0)/sqrt(x*x+y*y+z*z); return v4(x*l, y*l, z*l, w); }
	mx_inline v4& operator  = (const type s) { x=s; y=s; z=s; return *this; }
	mx_inline v4& operator  = (const v4& v) { x=v.x; y=v.y; z=v.z; w=v.w; return *this; }
	mx_inline v4& operator += (const v4& v) { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
	mx_inline v4& operator -= (const v4& v) { x-=v.x; y-=v.y; z-=v.z; w-=v.w; return *this; }
	mx_inline v4& operator *= (const v4& v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
	mx_inline v4& operator /= (const v4& v) { x/=v.x; y/=v.y; z/=v.z; w/=v.w; return *this; }
	mx_inline v4& operator ^= (const v4& v) { *this = *this ^ v; return *this; }
	mx_inline v4  operator  + (const v4& v) { return v4(x+v.x, y+v.y, z+v.z, w+v.w); }
	mx_inline v4  operator  - (const v4& v) { return v4(x-v.x, y-v.y, z-v.z, w-v.w); }
	mx_inline v4  operator  * (const v4& v) { return v4(x*v.x, y*v.y, z*v.z, w*v.w); }
	mx_inline v4  operator  / (const v4& v) { return v4(x/v.x, y/v.y, z/v.z, w/v.w); }
	mx_inline v4  operator  ^ (const v4& v) { return v4(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
	mx_inline type operator | (const v4& v) { return x*v.x+y*v.y+z*v.z; }

	mx_inline v4& operator += (const type& a) { x+=a; y+=a; z+=a; w+=a; return *this; }
	mx_inline v4& operator -= (const type& a) { x-=a; y-=a; z-=a; w-=a; return *this; }
	mx_inline v4& operator *= (const type& a) { x*=a; y*=a; z*=a; w*=a; return *this; }
	mx_inline v4& operator /= (const type& a) { x/=a; y/=a; z/=a; w/=a; return *this; }

	mx_inline v4  operator +  (const type& a) { return v4(x+a, y+a, z+a, w+a); }
	mx_inline v4  operator -  (const type& a) { return v4(x-a, y-a, z-a, w-a); }
	mx_inline v4  operator *  (const type& a) { return v4(x*a, y*a, z*a, w*a); }
	mx_inline v4  operator /  (const type& a) { return v4(x/a, y/a, z/a, w/a); }

	template <typename S> mx_inline friend v4 operator + (const S a, const v4& v) { return v4(a+v.x, a+v.y, a+v.z, a+v.w); }
	template <typename S> mx_inline friend v4 operator - (const S a, const v4& v) { return v4(a-v.x, a-v.y, a-v.z, a-v.w); }
	template <typename S> mx_inline friend v4 operator * (const S a, const v4& v) { return v4(a*v.x, a*v.y, a*v.z, a*v.w); }
	template <typename S> mx_inline friend v4 operator / (const S a, const v4& v) { return v4(a/v.x, a/v.y, a/v.z, a/v.w); }

	mx_inline operator v2() const { return v2(x, y); }
	mx_inline operator v3() const { return v3(x, y, z); }
	mx_inline operator const type* () const { return &x; }
	mx_inline operator type* () { return &x; }

	PERMS3(v3, x, y, z);
	//PERMS3(v3, r, g, b);

	PERMS4(v4, x, y, z, w);
	//PERMS4(v4, s, t, p, q);
	//PERMS4(v4, r, g, b, a);

	v4 deg2rad() const { return v4(deg2rad(x), deg2rad(y), deg2rad(z), w); }
	v4 rad2deg() const { return v4(rad2deg(x), rad2deg(y), rad2deg(z), w); }
	v3 sin() const { return v3(sin(x), sin(y), sin(z)); }
	v3 cos() const { return v3(cos(x), cos(y), cos(z)); }

	bool operator == (const v4& v) const { return x==v.x && y==v.y && z==v.y && w==v.w; }
	bool operator != (const v4& v) const { return x!=v.x || y!=v.y || z!=v.y || w!=v.w; }

public:
	mx_align16 union {
		struct { type x, y, z, w; };
		//struct { type s, t, p, q; };
		//struct { type r, g, b, a; };
		struct { type _0, _1, _2, _3; };
		//type v[4];
	};
};

typedef vec4t<f32> vec4f;
typedef vec4t<f64> vec4d;
typedef vec4t<i32> vec4i;
typedef vec4t<f32> v4f;
typedef vec4t<f64> v4d;
typedef vec4t<i32> v4i;

///

template <typename type> vec4t<type> operator + (const vec3t<type>& u, const vec4t<type>& v)
{ return vec4t<type>(u.x+v.x, u.y+v.y, u.z+v.z, v.w); }

///

template <typename T>
class mtx2x2t : public type_traits<T> {
public:
	typedef T type;
	typedef vec2t<T> v2;
	typedef mtx2x2t<T> m2;

public:
	mtx2x2t() {}
	mtx2x2t(const type a11, const type a12, const type a21, const type a22)
		: _00(a11), _01(a12), _10(a21), _11(a22) {}
	mtx2x2t(const m2& m) { memcpy(d, &m.d, sizeof(m2)); }

public:
	mx_align16 union {
		type d[2][2];
		type v[4];
		struct { type _00,_01,_10,_11; };
	};
};

///

template <typename T>
class mtx3x3t : public type_traits<T> {
public:
	typedef T type;
	typedef vec3t<T> v3;
	typedef vec4t<T> v4;
	typedef mtx3x3t<T> m3;

public:
	static const m3& identity() {
		static const m3 unit(
			T(1.0), T(0.0), T(0.0),
			T(0.0), T(1.0), T(0.0),
			T(0.0), T(0.0), T(1.0));
		return unit;
	}
	/*
	static m3 rotate(const type rad, const v3& v) {
		m3 r;
		const type cs = cos(rad);
		const type sn = sin(rad);
    
		r._00 = v.x*v.x + (1-v.x*v.x) * cs;
		r._01 = v.x*v.y * (1-cs) + v.z*sn;
		r._02 = v.x*v.z * (1-cs) - v.y*sn;
		r._10 = v.x*v.y * (1-cs) - v.z*sn;
		r._11 = v.y*v.y + (1-v.y*v.y) * cs;
		r._12 = v.y*v.z * (1-cs) + v.x*sn;
		r._20 = v.x*v.z * (1-cs) + v.y*sn;
		r._21 = v.y*v.z * (1-cs) - v.x*sn;
		r._22 = v.z*v.z + (1-v.z*v.z) * cs;

		return r;
	}
	*/
	static m3 rotr(const type rad, const type x, const type y, const type z) {
		const v3 v = ~v3(x, y, z);
		const type p = sin(rad);
		const type q = cos(rad);

		const type oq = 1 - q;
		const type aoq = v.x * oq;
		const type aoqc = aoq * v.z;
		const type aoqb = aoq * v.y;
		const type boqc = v.y * v.z * oq;
		const type ap = v.x * p;
		const type bp = v.y * p;
		const type cp = v.z * p;

		m3 r;
		r._00 = v.x * aoq + q;
		r._01 = aoqb - cp;
		r._02 = aoqc + bp;

		r._10 = aoqb + cp;
		r._11 = v.y * v.y * oq + q;
		r._12 = boqc - ap;

		r._20 = aoqc - bp;
		r._21 = boqc + ap;
		r._22 = v.z * v.z * oq + q;

		return r;
	}

	static m3 rotd(const type deg, const type x, const type y, const type z) {
		return rotr(deg2rad(deg), x, y, z);
	}

public:
	mtx3x3t() {}

	mtx3x3t(const type a11, const type a12, const type a13,
			const type a21, const type a22, const type a23,
			const type a31, const type a32, const type a33)
		: _00(a11), _01(a12), _02(a13)
		, _10(a21), _11(a22), _12(a23)
		, _20(a31), _21(a32), _22(a33)
	{
	}

	mtx3x3t(const m3& m) {
		memcpy(d, m.d, sizeof(m3));
	}

	m3& operator = (const m3& m) {
		memcpy(d, m.d, sizeof(m3));
		return *this;
	}

	type operator () (const size_t row, const size_t col) const { return d[row][col]; }
	type& operator () (const size_t row, const size_t col) { return d[row][col]; }
	operator const type* () const { return d; }
	operator type* () { return d; }

	type det() const {
		return _00*(_11*_22-_12*_21) - _01*(_10*_22-_12*_20) + _02*(_10*_21-_11*_20);
	}

	m3 operator ! () const {
		type dt = det();
		dt = is_zero(dt) ? T(1.0) : T(1.0)/dt;
		m3 r;

		#define DET2(r1, r2, c1, c2) (_##r1##c1*_##r2##c2-_##r2##c1*_##r1##c2)

		r._00 = +dt*DET2(1, 2, 1, 2);
		r._01 = -dt*DET2(0, 2, 1, 2);
		r._02 = +dt*DET2(0, 1, 1, 2);
		r._10 = -dt*DET2(1, 2, 0, 2);
		r._11 = +dt*DET2(0, 2, 0, 2);
		r._12 = -dt*DET2(0, 1, 0, 2);
		r._20 = +dt*DET2(1, 2, 0, 1);
		r._21 = -dt*DET2(0, 2, 0, 1);
		r._22 = +dt*DET2(0, 1, 0, 1);

		#undef DET2

		return r;
	}

	m3& operator += (const m3& m) {
		_00 += m._00; _01 += m._01; _02 += m._02;
		_10 += m._10; _11 += m._11; _12 += m._12;
		_20 += m._20; _21 += m._21; _22 += m._22;
		return *this;
	}

	m3& operator -= (const m3& m) {
		_00 -= m._00; _01 -= m._01; _02 -= m._02;
		_10 -= m._10; _11 -= m._11; _12 -= m._12;
		_20 -= m._20; _21 -= m._21; _22 -= m._22;
		return *this;
	}

	m3& operator *= (const m3& m) {
		return *this = *this * m;
	}

	m3 operator - () const {
		m3 r;
		r._00 = -_00; r._01 = -_01; r._02 = -_02;
		r._10 = -_10; r._11 = -_11; r._12 = -_12;
		r._20 = -_20; r._21 = -_21; r._22 = -_22;
		return r;
	}

	m3 operator ~ () const {
		m3 r;
		r._00 = _00; r._01 = _10; r._02 = _20;
		r._10 = _01; r._11 = _11; r._12 = _21;
		r._20 = _02; r._21 = _12; r._22 = _22;
		return r;
	}

	m3 operator + (const m3& m) const {
		m3 r = *this;
		return r += m;
	}

	m3 operator - (const m3& m) const {
		m3 r = *this;
		return r -= m;
	}

	m3 operator * (const m3& m) const {
		m3 r;
		r._00 = _00*m._00 + _01*m._10 + _02*m._20;
		r._01 = _00*m._01 + _01*m._11 + _02*m._21;
		r._02 = _00*m._02 + _01*m._12 + _02*m._22;
		r._10 = _10*m._00 + _11*m._10 + _12*m._20;
		r._11 = _10*m._01 + _11*m._11 + _12*m._21;
		r._12 = _10*m._02 + _11*m._12 + _12*m._22;
		r._20 = _20*m._00 + _21*m._10 + _22*m._20;
		r._21 = _20*m._01 + _21*m._11 + _22*m._21;
		r._22 = _20*m._02 + _21*m._12 + _22*m._22;
		return r;
	}

	v3 operator * (const v3& v) const {
		return v3(
			_00*v._0 + _01*v._1 + _02*v._2,
			_10*v._0 + _11*v._1 + _12*v._2,
			_20*v._0 + _21*v._1 + _22*v._2);
	}

	v4 operator * (const v4& v) const {
		return v4(
			_00*v._0 + _01*v._1 + _02*v._2,
			_10*v._0 + _11*v._1 + _12*v._2,
			_20*v._0 + _21*v._1 + _22*v._2,
			v._3);
	}

	friend vec3t<T> operator * (const vec3t<T>& v, const mtx3x3t<T>& m) {
		return vec3t<T>(
			v._0*m._00 + v._1*m._10 + v._2*m._20,
			v._0*m._01 + v._1*m._11 + v._2*m._21,
			v._0*m._02 + v._1*m._12 + v._2*m._22);
	}

	friend vec4t<T> operator * (const vec4t<T>& v, const mtx3x3t<T>& m) {
		return vec4t<T>(
			v._0*m._00 + v._1*m._10 + v._2*m._20,
			v._0*m._01 + v._1*m._11 + v._2*m._21,
			v._0*m._02 + v._1*m._12 + v._2*m._22,
			v._3);
	}

public:
	mx_align16 union {
		type d[3][3];
		type v[9];
		struct { type _00,_01,_02,_10,_11,_12,_20,_21,_22; };
	};
};

typedef mtx3x3t<f32> mtx3f;
typedef mtx3x3t<f64> mtx3d;
typedef mtx3x3t<f32> m3f;
typedef mtx3x3t<f64> m3d;

///

template <typename T>
class mtx4x4t : public type_traits<T> {
public:
	typedef T type;
	typedef vec3t<T> v3;
	typedef vec4t<T> v4;
	typedef mtx3x3t<T> m3;
	typedef mtx4x4t<T> m4;

public:
	static const m4& identity() {
		static const m4 unit(
			T(1.0), T(0.0), T(0.0), T(0.0),
			T(0.0), T(1.0), T(0.0), T(0.0),
			T(0.0), T(0.0), T(1.0), T(0.0),
			T(0.0), T(0.0), T(0.0), T(1.0));
		return unit;
	}
	/*
	static m3 rotate(const type rad, const v3& v) {
		m3 r;
		const type cs = cos(rad);
		const type sn = sin(rad);
    
		r._00 = v.x*v.x + (1.f-v.x*v.x) * cs;
		r._01 = v.x*v.y * (1.f-cs) + v.z*sn;
		r._02 = v.x*v.z * (1.f-cs) - v.y*sn;
		r._10 = v.x*v.y * (1.f-cs) - v.z*sn;
		r._11 = v.y*v.y + (1.f-v.y*v.y) * cs;
		r._12 = v.y*v.z * (1.f-cs) + v.x*sn;
		r._20 = v.x*v.z * (1.f-cs) + v.y*sn;
		r._21 = v.y*v.z * (1.f-cs) - v.x*sn;
		r._22 = v.z*v.z + (1.f-v.z*v.z) * cs;

		return r;
	}//*/

	static m4 rotr(const type rad, const type x, const type y, const type z) {
		const v3 v = ~v3(x, y, z);
		const type p = sin(rad);
		const type q = cos(rad);
		const type oq = 1-q;
		const type aoq = v.x*oq;
		const type aoqc = aoq*v.z;
		const type aoqb = aoq*v.y;
		const type boqc = v.y*oq*v.z;
		const type ap = v.x*p;
		const type bp = v.y*p;
		const type cp = v.z*p;

		m4 r;

		r._00 = v.x*aoq+q;
		r._01 = aoqb-cp;
		r._02 = aoqc+bp;
		r._03 = T(0.0);

		r._10 = aoqb+cp;
		r._11 = v.y*v.y*oq+q;
		r._12 = boqc-ap;
		r._13 = T(0.0);

		r._20 = aoqc-bp;
		r._21 = boqc+ap;
		r._22 = v.z*v.z*oq+q;
		r._23 = T(0.0);

		r._30 = T(0.0);
		r._31 = T(0.0);
		r._32 = T(0.0);
		r._33 = T(1.0);

		return r;
	}

	static m4 rotr(const type rad, const v3& v) {
		return rotr(rad, v.x, v.y, v.z);
	}

	static m4 rotd(const type deg, const type x, const type y, const type z) {
		return rotr(deg2rad(deg), x, y, z);
	}

	static m4 rotd(const type deg, const v3& v) {
		return rotr(deg2rad(deg), v.x, v.y, v.z);
	}

	static m4 tra(const type x, const type y, const type z) {
		m4 r = identity();
		r._03 = x;
		r._13 = y;
		r._23 = z;
		return r;
	}

	static m4 tra(const v3& v) {
		m4 r = identity();
		r._03 = v.x;
		r._13 = v.y;
		r._23 = v.z;
		return r;
	}

	static m4 sca(const type x, const type y, const type z) {
		m4 r = identity();
		r._00 = x;
		r._11 = y;
		r._22 = z;
		return r;
	}

	static m4 sca(const v3& v) {
		m4 r = identity();
		r._00 = v.x;
		r._11 = v.y;
		r._22 = v.z;
		return r;
	}

	static m4 viewport(const type width, const type height, const type x = T(0.0), const type y = T(0.0), const type znear = T(0.0), const type zfar = T(1.0)) {
		//X = (X + 1) * Viewport.Width * 0.5 + Viewport.TopLeftX
		//Y = (1 - Y) * Viewport.Height * 0.5 + Viewport.TopLeftY
		//Z = Viewport.MinDepth + Z * (Viewport.MaxDepth - Viewport.MinDepth) 
		m4 r = identity();
		r._00 = width * 0.5f;
		r._03 = x + width * 0.5f;
		r._11 = -height * 0.5f;
		r._13 = y + height * 0.5f;
		r._22 = (zfar - znear) * 0.5f;
		r._23 = (zfar + znear) * 0.5f;
		return r;
	}

	static m4 ortho(const type left, const type right, const type bottom, const type top, const type znear = T(-1.0), const type zfar = T(1.0)) {
		m4 r = identity();
		r._00 = T(2.0) / (right-left);
		r._03 = -(right + left) / (right - left);
		r._11 = T(2.0) / (top - bottom);
		r._13 = -(top + bottom) / (top - bottom);
		r._22 = T(-2.0) / (zfar - znear);
		r._23 = -(zfar + znear) / (zfar - znear);
		return r;
	}
	
	static m4 frustum(const type left, const type right, const type bottom, const type top, const type znear, const type zfar) {
		m4 r;

		r._00 = T(2.0) * znear / (right - left);
		r._01 = T(0.0);
		r._02 = (right + left) / (right - left);
		r._03 = T(0.0);

		r._10 = T(0.0);
		r._11 = T(2.0) * znear / (top - bottom);
		r._12 = (top + bottom) / (top - bottom);
		r._13 = T(0.0);

		r._20 = T(0.0);
		r._21 = T(0.0);
		r._22 = -(zfar + znear) / (zfar - znear);
		r._23 = T(-2.0) * zfar * znear/(zfar - znear);

		r._30 = T(0.0);
		r._31 = T(0.0);
		r._32 = T(-1.0);
		r._33 = T(0.0);

		return r;
	}

	static m4 perspective(const type fovy, const type aspect, const type znear, const type zfar) {
		// cot(fovy/2)/aspect           0            0           0
		//                  0 cot(fovy/2)            0           0
		//                  0           0  (N+F)/(N-F) 2*N*F/(N-F)
		//                  0           0           -1           0
		const type top = znear * tan(deg2rad(fovy * T(0.5))), right = top * aspect;
		return frustum(-right, right, -top, top, znear, zfar);
	}

	static m4 lookat(const v3& eye, const v3& at, const v3& up) {
		// v3 Pc = v3(ex, ey, ez)
		// v3 Zc = !(Pc - v3(cx, cy, cz))
		// v3 Yc = !v3(ux, uy, uz)
		// v3 Xc = !(Yc ^ Zc)
		// Yc = Zc ^ Xc
		// Xc.x Xc.y Xc.z -dot(Xc, Pc)
		// Yc.x Yc.y Yc.z -dot(Yc, Pc)
		// Zc.x Zc.y Zc.z -dot(Zc, Pc)
		//    0    0    0            1

		const vec3f fwd = ~(at - eye);
		const vec3f side = ~(fwd ^ up);
		const vec3f u = side ^ fwd;

		m4 r;

		r._00 = side.x;
		r._01 = u.x;
		r._02 = -fwd.x;
		r._03 = fwd.x*eye.z - side.x*eye.x - u.x*eye.y;

		r._10 = side.y;
		r._11 = u.y;
		r._12 = -fwd.y;
		r._13 = fwd.y*eye.z - side.y*eye.x - u.y*eye.y;

		r._20 = side.z;
		r._21 = u.z;
		r._22 = -fwd.z;
		r._23 = fwd.z*eye.z - side.z*eye.x - u.z*eye.y;

		r._30 = T(0.0);
		r._31 = T(0.0);
		r._32 = T(0.0);
		r._33 = T(1.0);

		return r;
	}

public:
	mtx4x4t() {}

	mtx4x4t(const type a11, const type a12, const type a13, const type a14,
			const type a21, const type a22, const type a23, const type a24,
			const type a31, const type a32, const type a33, const type a34,
			const type a41, const type a42, const type a43, const type a44)
		: _00(a11), _01(a12), _02(a13), _03(a14)
		, _10(a21), _11(a22), _12(a23), _13(a24)
		, _20(a31), _21(a32), _22(a33), _23(a34)
		, _30(a41), _31(a42), _32(a43), _33(a44)
	{
	}

	explicit mtx4x4t(const m3& m) {
		memcpy(&_00, &m._00, sizeof(type)*3);
		memcpy(&_10, &m._10, sizeof(type)*3);
		memcpy(&_20, &m._20, sizeof(type)*3);
		_03 = _13 = _23 = _30 = _31 = _32 = T(0.0);
		_33 = T(1.0);
	}

	mtx4x4t(const m4& m) {
		memcpy(v, m.v, sizeof(m4));
	}

	m4& operator = (const m4& m) {
		memcpy(v, m.v, sizeof(m4));
		return *this;
	}

	type operator () (const size_t row, const size_t col) const { return d[row][col]; }
	type& operator () (const size_t row, const size_t col) { return d[row][col]; }
	operator const type* () const { return &v[0]; }
	operator type* () { return &v[0]; }

	type det() const {
		return	(_00*_11 - _01*_10) * (_22*_33 - _23*_32) -
				(_00*_12 - _02*_10) * (_21*_33 - _23*_31) +
				(_00*_13 - _03*_10) * (_21*_32 - _22*_31) +
				(_01*_12 - _02*_11) * (_20*_33 - _23*_30) -
				(_01*_13 - _03*_11) * (_20*_32 - _22*_30) +
				(_02*_13 - _03*_12) * (_20*_31 - _21*_30);
	}

	m4 operator ! () const {
		type dt = det();
		dt = is_zero(dt) ? T(1.0) : T(1.0)/dt;

		#define DET3(r1, r2, r3, c1, c2, c3) \
			_##r1##c1 * (_##r2##c2*_##r3##c3 - _##r2##c3*_##r3##c2) - \
			_##r1##c2 * (_##r2##c1*_##r3##c3 - _##r2##c3*_##r3##c1) + \
			_##r1##c3 * (_##r2##c1*_##r3##c2 - _##r2##c2*_##r3##c1)
			
		m4 r;

		r._00 =  dt*DET3(1, 2, 3, 1, 2, 3);
		r._01 = -dt*DET3(0, 2, 3, 1, 2, 3);
		r._02 =  dt*DET3(0, 1, 3, 1, 2, 3);
		r._03 = -dt*DET3(0, 1, 2, 1, 2, 3);
		
		r._10 = -dt*DET3(1, 2, 3, 0, 2, 3);
		r._11 =  dt*DET3(0, 2, 3, 0, 2, 3);
		r._12 = -dt*DET3(0, 1, 3, 0, 2, 3);
		r._13 =  dt*DET3(0, 1, 2, 0, 2, 3);

		r._20 =  dt*DET3(1, 2, 3, 0, 1, 3);
		r._21 = -dt*DET3(0, 2, 3, 0, 1, 3);
		r._22 =  dt*DET3(0, 1, 3, 0, 1, 3);
		r._23 = -dt*DET3(0, 1, 2, 0, 1, 3);

		r._30 = -dt*DET3(1, 2, 3, 0, 1, 2);
		r._31 =  dt*DET3(0, 2, 3, 0, 1, 2);
		r._32 = -dt*DET3(0, 1, 3, 0, 1, 2);
		r._33 =  dt*DET3(0, 1, 2, 0, 1, 2);

		#undef DET3

		return r;
	}

	m4& operator += (const m4& m) {
		_00 += m._00; _01 += m._01; _02 += m._02; _03 += m._03;
		_10 += m._10; _11 += m._11; _12 += m._12; _13 += m._13;
		_20 += m._20; _21 += m._21; _22 += m._22; _23 += m._23;
		_30 += m._30; _31 += m._31; _32 += m._32; _33 += m._33;
		return *this;
	}

	m4& operator -= (const m4& m) {
		_00 -= m._00; _01 -= m._01; _02 -= m._02; _03 -= m._03;
		_10 -= m._10; _11 -= m._11; _12 -= m._12; _13 -= m._13;
		_20 -= m._20; _21 -= m._21; _22 -= m._22; _23 -= m._23;
		_30 -= m._30; _31 -= m._31; _32 -= m._32; _33 -= m._33;
		return *this;
	}

	m4& operator *= (const m4& m) {
		return *this = *this * m;
	}
	
	m4 operator - () const {
		m4 r;
		r._00 = -_00; r._01 = -_01; r._02 = -_02; r._03 = -_03;
		r._10 = -_10; r._11 = -_11; r._12 = -_12; r._13 = -_13;
		r._20 = -_20; r._21 = -_21; r._22 = -_22; r._23 = -_23;
		r._30 = -_30; r._31 = -_31; r._32 = -_32; r._33 = -_33;
		return r;
	}

	m4 operator ~ () const {
		m4 r;
		r._00 = _00; r._01 = _10; r._02 = _20; r._03 = _30;
		r._10 = _01; r._11 = _11; r._12 = _21; r._13 = _31;
		r._20 = _02; r._21 = _12; r._22 = _22; r._23 = _32;
		r._30 = _03; r._31 = _13; r._32 = _23; r._33 = _33;
		return r;
	}

	m4 operator + (const m4& m) const {
		m4 r(*this);
		return r += m;
	}

	m4 operator - (const m4& m) const {
		m4 r(*this);
		return r -= m;
	}

	m4 operator * (const m4& m) const {
		m4 r;

		r._00 = _00*m._00 + _01*m._10 + _02*m._20 + _03*m._30;
		r._01 = _00*m._01 + _01*m._11 + _02*m._21 + _03*m._31;
		r._02 = _00*m._02 + _01*m._12 + _02*m._22 + _03*m._32;
		r._03 = _00*m._03 + _01*m._13 + _02*m._23 + _03*m._33;
		r._10 = _10*m._00 + _11*m._10 + _12*m._20 + _13*m._30;
		r._11 = _10*m._01 + _11*m._11 + _12*m._21 + _13*m._31;
		r._12 = _10*m._02 + _11*m._12 + _12*m._22 + _13*m._32;
		r._13 = _10*m._03 + _11*m._13 + _12*m._23 + _13*m._33;
		r._20 = _20*m._00 + _21*m._10 + _22*m._20 + _23*m._30;
		r._21 = _20*m._01 + _21*m._11 + _22*m._21 + _23*m._31;
		r._22 = _20*m._02 + _21*m._12 + _22*m._22 + _23*m._32;
		r._23 = _20*m._03 + _21*m._13 + _22*m._23 + _23*m._33;
		r._30 = _30*m._00 + _31*m._10 + _32*m._20 + _33*m._30;
		r._31 = _30*m._01 + _31*m._11 + _32*m._21 + _33*m._31;
		r._32 = _30*m._02 + _31*m._12 + _32*m._22 + _33*m._32;
		r._33 = _30*m._03 + _31*m._13 + _32*m._23 + _33*m._33;

		return r;
	}

	v3 operator * (const v3& v) const {
		return v3(
			_00*v._0 + _01*v._1 + _02*v._2 + _03,
			_10*v._0 + _11*v._1 + _12*v._2 + _13,
			_20*v._0 + _21*v._1 + _22*v._2 + _23);
	}

	v4 operator * (const v4& v) const {
		v4 r = v4(
			_00*v._0 + _01*v._1 + _02*v._2 + _03*v._3,
			_10*v._0 + _11*v._1 + _12*v._2 + _13*v._3,
			_20*v._0 + _21*v._1 + _22*v._2 + _23*v._3,
			_30*v._0 + _31*v._1 + _32*v._2 + _33*v._3);
		return r;
	}

	friend vec3t<T> operator * (const vec3t<T>& v, const mtx4x4t<T>& m) {
		return vec3t<T>(
			v._0*m._00 + v._1*m._10 + v._2*m._20 + m._30,
			v._0*m._01 + v._1*m._11 + v._2*m._21 + m._31,
			v._0*m._02 + v._1*m._12 + v._2*m._22 + m._32);
	}

	friend vec4t<T> operator * (const vec4t<T>& v, const mtx4x4t<T>& m) {
		return vec4t<T>(
			v._0*m._00 + v._1*m._10 + v._2*m._20 + v._3*m._30,
			v._0*m._01 + v._1*m._11 + v._2*m._21 + v._3*m._31,
			v._0*m._02 + v._1*m._12 + v._2*m._22 + v._3*m._32,
			v._0*m._03 + v._1*m._13 + v._2*m._23 + v._3*m._33);
	}

public:
	mx_align16 union {
		type d[4][4];
		type v[16];
		struct { type _00,_01,_02,_03,_10,_11,_12,_13,_20,_21,_22,_23,_30,_31,_32,_33; };
	};
};

typedef mtx4x4t<f32> m4f;
typedef mtx4x4t<f64> m4d;

template <typename T>
class quatt : type_traits<T> {
public:
	typedef T type;
	typedef quatt<type> qt;
	typedef vec3t<type> v3;
	typedef vec4t<type> v4;
	typedef mtx3x3t<type> m3;
	typedef mtx4x4t<type> m4;

public:
	quatt() {}
	quatt(const type rad, const type vx, const type vy, const type vz) {
		const type hang = rad * T(0.5);
		const type sn = sin(hang);
		x = vx * sn;
		y = vy * sn;
		z = vz * sn;
		w = cos(hang);
	}
	quatt(const type rad, const v3& axis) {
		const type hang = rad * T(0.5);
		const type sn = sin(hang);
		x = axis.x * sn;
		y = axis.y * sn;
		z = axis.z * sn;
		w = cos(hang);
	}
	quatt(const qt& q) : x(q.x), y(q.y), z(q.z), w(q.w) {}
	quatt(const v4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	type operator ! () const { return sqrt(x*x + y*y + z*z + w*w); }
	qt operator - () const { return qt(-x, -y, -z, w); }

	type angle() const { return T(2.0) * acos(w); }
	v3 axis() const { v3 v(x, y, z); return v * v.rlength(); }

	operator v3() const { return v3(x, y, z); }
	operator v4() const { return v4(x, y, z, w); }
	operator const type* () const { return &x; }
	operator type* () { return &x; }

	qt& operator += (const qt& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
	qt& operator -= (const qt& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
	qt& operator *= (const qt& q) { return *this = *this ^ q; }
	qt& operator *= (const v3& v) { return *this = *this * v; }
	qt& operator *= (const v4& v) { return *this = *this * v; }

	qt operator + (const qt& q) const { return qt(x+q.x, y+q.y, z+q.z, w+q.w); }
	qt operator - (const qt& q) const { return qt(x+q.x, y+q.y, z+q.z, w+q.w); }
	qt operator * (const qt& q) const { return (*this) ^ q; }
	v3 operator * (const v3& v) const { return (*this) ^ v ^ -(*this); }
	v4 operator * (const v4& v) const { return (*this) ^ v ^ -(*this); }

private:
	qt operator ^ (const qt& q) const {
		return qt(
			w*q.x + x*q.w + y*q.z - z*q.y,
			w*q.y + y*q.w + z*q.x - x*q.z,
			w*q.z + z*q.w + x*q.y - y*q.x,
			w*q.w - x*q.x - y*q.y - z*q.z);
	}

	qt operator ^ (const v3& v) const {
		return qt( // v.w = 1.0
			w*v.x + x + y*v.z - z*v.y,
			w*v.y + y + z*v.x - x*v.z,
			w*v.z + z + x*v.y - y*v.x,
			w - x*v.x - y*v.y - z*v.z);
	}

	qt operator ^ (const v4& v) const {
		return qt(
			w*v.x + x*v.w + y*v.z - z*v.y,
			w*v.y + y*v.w + z*v.x - x*v.z,
			w*v.z + z*v.w + x*v.y - y*v.x,
			w*v.w - x*v.x - y*v.y - z*v.z);
	}

public:
	qt from_euler(const v3& rpy) const {
		//x-roll, y-pitch, z-yaw; radians
		const type cyaw		= cos(T(0.5)*rpy.z);
		const type cpitch	= cos(T(0.5)*rpy.y);
		const type croll	= cos(T(0.5)*rpy.x);
		const type syaw		= sin(T(0.5)*rpy.z);
		const type spitch	= sin(T(0.5)*rpy.y);
		const type sroll	= sin(T(0.5)*rpy.x);

		const type cyawcpitch = cyaw * cpitch;
		const type syawspitch = syaw * spitch;
		const type cyawspitch = cyaw * spitch;
		const type syawcpitch = syaw * cpitch;

		return qt(
			cyawcpitch*sroll - syawspitch*croll,
			cyawspitch*croll + syawcpitch*sroll,
			syawcpitch*croll - cyawspitch*sroll,
			cyawcpitch*croll + syawspitch*sroll);
	}
	
	v3 to_euler() const {
		const type q00 = w*w;
		const type q11 = x*x;
		const type q22 = y*y;
		const type q33 = z*z;

		const type r11 = q00 + q11 - q22 - q33;
		const type r21 = T(2.0) * (x*y + w*z);
		const type r31 = T(2.0) * (x*z - w*y);
		const type r32 = T(2.0) * (y*z + w*x);
		const type r33 = q00 - q11 - q22 + q33;
		const type r12 = T(0.0);
		const type r13 = T(0.0);

		const type tmp = fabs(r31);

		if (tmp > (T(1.0)-T(eps))) {
			const type r12 = T(2.0) * (x*y - w*z);
			const type r13 = T(2.0) * (x*z + w*y);

			return v3(T(0), -T(pi)*T(0.5) * r31/tmp, atan2(-r12, -r31*r13));
		}

		return v3(atan2(r32, r33), asin(-r31), atan2(r21, r11));
	}

	qt& from_spherical(const type lon, const type lat, const type rad) {
		// spherical coords to quaternion
		const type hang = rad * T(0.5);
		const type sin_ang = sin(hang);
		const type cos_ang = cos(hang);
		const type sin_lat = sin(lat);
		const type cos_lat = cos(lat);
		const type sin_lon = sin(lon);
		const type cos_lon = cos(lon);

		x = sin_ang * cos_lat * sin_lon;
		y = sin_ang * sin_lat;
		z = sin_ang * sin_lat * cos_lon;
		w = cos_ang;
		
		return *this;
	}

	m3 to3x3() const {
		const type x2 = T(2.0)*x, y2 = T(2.0)*y, z2 = T(2.0)*z;
		const type xx = x2*x, yy = y2*y, zz = z2*z;
		const type xy = x2*y, xz = z2*x, yz = y2*z;
		const type xw = x2*w, yw = y2*w, zw = z2*w;

		m3 r;

		r.d[0][0] = T(1.0)-(yy+zz);
		r.d[0][1] = xy-zw;
		r.d[0][2] = xz+yw;
		r.d[1][0] = xy+zw;
		r.d[1][1] = T(1.0)-(xx+zz);
		r.d[1][2] = yz-xw;
		r.d[2][0] = xz-yw;
		r.d[2][1] = yz+xw;
		r.d[2][2] = T(1.0)-(xx+yy);

		return r;
	}

	m4 to4x4() const {
		const type x2 = T(2.0)*x, y2 = T(2.0)*y, z2 = T(2.0)*z;
		const type xx = x2*x, yy = y2*y, zz = z2*z;
		const type xy = x2*y, xz = z2*x, yz = y2*z;
		const type xw = x2*w, yw = y2*w, zw = z2*w;

		m4 r;

		r.d[0][0] = T(1.0)-(yy+zz);
		r.d[0][1] = xy-zw;
		r.d[0][2] = xz+yw;
		r.d[0][3] = T(0.0);

		r.d[1][0] = xy+zw;
		r.d[1][1] = T(1.0)-(xx+zz);
		r.d[1][2] = yz-xw;
		r.d[1][3] = T(0.0);

		r.d[2][0] = xz-yw;
		r.d[2][1] = yz+xw;
		r.d[2][2] = T(1.0)-(xx+yy);
		r.d[2][3] = T(0.0);

		r.d[3][0] = T(0.0);
		r.d[3][1] = T(0.0);
		r.d[3][2] = T(0.0);
		r.d[3][3] = T(1.0);

		return r;
	}

	qt slerp(const qt& p, const type t) { // t = [0,1
		type cosom = x*p.x + y*p.y + z*p.z + w*p.w; // omega angle cosine
		qt r;

		if (cosom < T(0.0)) { 
			cosom = -cosom;
			r = -p;
		} else {
			r = p;
		}

		if ((T(1.0)-cosom) > T(eps)) { // standard case -> slerp
			type omega = acos(cosom);
			type sinom = sin(omega);
			type scale0 = sin((T(1.0) - t) * omega) / sinom;
			type scale1 = sin(t * omega) / sinom;
			return *this*scale0 + r*scale1;
		}

		// small angle -> linear interpolation
		return *this*(T(1.0) - t) + r*t;
	}

public:
	mx_align16 union {
		struct { type x, y, z, w; };
		struct { type _0, _1, _2, _3; };
		type v[4];
	};
};

typedef quatt<f32> quatf;
typedef quatt<f64> quatd;
typedef quatt<f32> qf;
typedef quatt<f64> qd;

///

void batch_mul_pure(vec4t<f32>* const r, const mtx4x4t<f32>& m, const size_t count, const vec4t<f32>* const v) {
	for (size_t i=0; i<count; ++i) {
		r[i] = m * v[i];
	}
}

void batch_mul_sse2i(vec4t<f32>* const r, const mtx4x4t<f32>& mtx, const size_t count, const vec4t<f32>* const v) {
	const mtx4x4t<f32> m = ~mtx;
	__m128* tm = (__m128*)&m;
	float* dst = (float*)r;

	for (size_t i=0; i<count; ++i) {
		__m128 _0 = _mm_load_ss(&v[i]._0);
		__m128 _1 = _mm_load_ss(&v[i]._1);
		__m128 _2 = _mm_load_ss(&v[i]._2);
		__m128 _3 = _mm_load_ss(&v[i]._3);

		__m128 a = _mm_add_ps(_mm_mul_ps(tm[0], _mm_shuffle_ps(_0, _0, 0)), _mm_mul_ps(tm[1], _mm_shuffle_ps(_1, _1, 0)));
		__m128 b = _mm_add_ps(_mm_mul_ps(tm[2], _mm_shuffle_ps(_2, _2, 0)), _mm_mul_ps(tm[3], _mm_shuffle_ps(_3, _3, 0)));

		_mm_stream_ps(dst, _mm_add_ps(a, b));
		dst += 4;
	}
}

void batch_mul_sse2a(vec4t<f32>* const r, const mtx4x4t<f32>& mtx, const size_t count, const vec4t<f32>* const v) {
	const mtx4x4t<f32> m = ~mtx;
	__asm {
		lea		edx, m
		mov		esi, dword ptr [v]
		mov		edi, dword ptr [r]
		movaps	xmm0, xmmword ptr [edx   ]
		movaps	xmm1, xmmword ptr [edx+16]
		movaps	xmm2, xmmword ptr [edx+32]
		movaps	xmm3, xmmword ptr [edx+48]

		xor		eax, eax
		mov		ecx, count

	label:
		prefetcht0 [esi+eax]
		movss	xmm4, dword ptr [esi+eax  ]
		movss	xmm5, dword ptr [esi+eax+4]
		movss	xmm6, dword ptr [esi+eax+8]
		movss	xmm7, dword ptr [esi+eax+12]
		shufps	xmm4, xmm4, 0
		shufps	xmm5, xmm5, 0
		shufps	xmm6, xmm6, 0
		shufps	xmm7, xmm7, 0
		mulps	xmm4, xmm0
		mulps	xmm5, xmm1
		mulps	xmm6, xmm2
		mulps	xmm7, xmm3

		addps	xmm4, xmm5
		addps	xmm6, xmm7
		addps	xmm4, xmm6

		//movaps	xmmword ptr [edi+eax], xmm4
		movntps	xmmword ptr [edi+eax], xmm4
		add		eax, 16
		dec		ecx
		jnz label
	}
}

void batch_mul_sse41i(vec4t<f32>* const r, const mtx4x4t<f32>& m, const size_t count, const vec4t<f32>* const v) {
	__m128* mtx = (__m128*)&m;
	__m128* src = (__m128*)v;
	__m128* dst = (__m128*)r;

	for (size_t i=0; i<count; ++i) {
		__m128 a = _mm_add_ps(_mm_dp_ps(mtx[0], src[i], 0xf1), _mm_dp_ps(mtx[1], src[i], 0xf2));
		__m128 b = _mm_add_ps(_mm_dp_ps(mtx[2], src[i], 0xf4), _mm_dp_ps(mtx[3], src[i], 0xf8));
		dst[i] = _mm_add_ps(a, b);
	}
}

void batch_mul_sse41a(vec4t<f32>* const r, const mtx4x4t<f32>& m, const size_t count, const vec4t<f32>* const v) {
	__asm {
		mov		eax, m
		mov		esi, dword ptr [v]
		mov		edi, dword ptr [r]

		movaps	xmm0, xmmword ptr [eax   ]
		movaps	xmm1, xmmword ptr [eax+16]
		movaps	xmm2, xmmword ptr [eax+32]
		movaps	xmm3, xmmword ptr [eax+48]

		xor		eax, eax
		mov		ecx, count
	label:
		movaps	xmm4, xmmword ptr [esi+eax]
		movaps	xmm5, xmmword ptr [esi+eax]
		movaps	xmm6, xmm4
		movaps	xmm7, xmm5
		dpps	xmm4, xmm0, 0xf1
		dpps	xmm5, xmm1, 0xf2
		dpps	xmm6, xmm2, 0xf4
		dpps	xmm7, xmm3, 0xf8
		addps	xmm4, xmm5
		addps	xmm6, xmm7
		addps	xmm4, xmm6
		movaps	xmmword ptr [edi+eax], xmm4
		add		eax, 16
		dec		ecx
		jnz label
	}
}

void batch_mul_pure(vec4t<f64>* const r, const mtx4x4t<f64>& m, const size_t count, const vec4t<f64>* const v) {
	for (size_t i=0; i<count; ++i) {
		r[i] = m * v[i];
	}
}

void batch_mul_sse2i(vec4t<f64>* const r, const mtx4x4t<f64>& mtx, const size_t count, const vec4t<f64>* const v) {
	const mtx4x4t<f64> m = ~mtx;
	__m128d* tm = (__m128d*)&m;
	__m128d* dst = (__m128d*)r;
	
	for (size_t i=0; i<count; ++i) {
		__m128d _0 = _mm_load_sd(&v[i]._0);
		__m128d _1 = _mm_load_sd(&v[i]._1);
		__m128d _2 = _mm_load_sd(&v[i]._2);
		__m128d _3 = _mm_load_sd(&v[i]._3);

		_0 = _mm_shuffle_pd(_0, _0, 0);
		_1 = _mm_shuffle_pd(_1, _1, 0);
		_2 = _mm_shuffle_pd(_2, _2, 0);
		_3 = _mm_shuffle_pd(_3, _3, 0);

		dst[(i<<1)  ] = _mm_add_pd(_mm_add_pd(_mm_mul_pd(tm[0], _0), _mm_mul_pd(tm[2], _1)), _mm_add_pd(_mm_mul_pd(tm[4], _2), _mm_mul_pd(tm[6], _3)));
		dst[(i<<1)+1] = _mm_add_pd(_mm_add_pd(_mm_mul_pd(tm[1], _0), _mm_mul_pd(tm[3], _1)), _mm_add_pd(_mm_mul_pd(tm[5], _2), _mm_mul_pd(tm[7], _3)));
	}
}

void batch_mul_avxi(vec4t<f64>* const r, const mtx4x4t<f64>& mtx, const size_t count, const vec4t<f64>* const v) {
	const mtx4x4t<f64> m = ~mtx;
	__m256d* tm = (__m256d*)&m;
	__m256d* dst = (__m256d*)r;

	for (size_t i=0; i<count; ++i) {
		__m256d a = _mm256_add_pd(_mm256_mul_pd(tm[0], _mm256_broadcast_sd(&v[i]._0)), _mm256_mul_pd(tm[1], _mm256_broadcast_sd(&v[i]._1)));
		__m256d b = _mm256_add_pd(_mm256_mul_pd(tm[2], _mm256_broadcast_sd(&v[i]._2)), _mm256_mul_pd(tm[3], _mm256_broadcast_sd(&v[i]._3)));
		dst[i] = _mm256_add_pd(a, b);
	}
}

///

template <class type>
class color4 {
public:
private:
	struct { type r, g, b, a; };
};

typedef color4<u8> c4ub;
typedef color4<f32> c4f;

///

template <int index, int count=1, typename type=unsigned long>
struct RegBit {
	int32_t raw;
	enum { mask = (1<<count)-1, inplace_mask = mask << index };
	template <class T> RegBit& operator = (const T v) {
		raw = (raw & ~inplace_mask) | ((v & mask) << index);
		return *this;
	}
	operator type () const { return (raw>>index) & mask; }
	RegBit& operator ++ () { return *this = *this + 1; }
	RegBit& operator -- () { return *this = *this - 1; }
	RegBit& operator += (const int i) { return *this = *this + i; }
	RegBit& operator -= (const int i) { return *this = *this - i; }

	//RegBit operator ++ (int) const {}
	//RegBit operator -- (int) const {}
};

struct CPUID_EAX {
	union {
		int raw;
		//eax=0
		unsigned long max_index;
		// EAX max cpuid index
		// EBX:EDX:ECX
		//eax=1
		RegBit< 0,4,u32> SteppingID;
		RegBit< 4,4,u32> Model;
		RegBit< 8,4,u32> Family;
		RegBit<12,2,u32> ProcessorType;
		RegBit<16,4,u32> ExtendedModel;
		//eax=4
		RegBit< 0, 5,u32> CacheTypeField;
		RegBit< 5, 3,u32> CacheLevel;
		RegBit< 8, 1,bool> SelfInitializingCache;
		RegBit< 9, 5,bool> FullyAssociativeCache;
		RegBit<14,12,u32> MaxLogicalProcessorsSharingThisCache;
		RegBit<26, 6,u32> PossibleCoresCount; // -1
	};
};

struct CPUID_EBX {
	union {
		int raw;
		//eax=0
		int s1;
		//eax=1
		RegBit< 0,8,u32> BrandIndex;
		RegBit< 8,8,u32> CLFLUSH_SIZE; // The size of the CLFLUSH cache line, in quadwords
		RegBit<16,8,u32> LogicalProcessorCount;
		RegBit<24,8,u32> InitialApicId;
	};
};

struct CPUID_ECX {
	union {
		int raw;
		// eax=0
		int s3;
		// eax=1
		RegBit< 0,1,bool> SSE3;
		RegBit< 1,1,bool> PCLMUL;
		RegBit< 2,1,bool> DTES64;
		RegBit< 3,1,bool> MON;
		RegBit< 4,1,bool> DSCPL;
		RegBit< 5,1,bool> VMX;
		RegBit< 6,1,bool> SMX;
		RegBit< 7,1,bool> EST;
		RegBit< 8,1,bool> TM2;
		RegBit< 9,1,bool> SSSE3;
		RegBit<10,1,bool> CID;
		RegBit<12,1,bool> FMA;
		RegBit<13,1,bool> CX16; // CMPXCHG16B
		RegBit<14,1,bool> ETPRD;
		RegBit<15,1,bool> PDCM;
		RegBit<18,1,bool> DCA;
		RegBit<19,1,bool> SSE41;
		RegBit<20,1,bool> SSE42;
		RegBit<21,1,bool> x2APIC;
		RegBit<22,1,bool> MOVBE;
		RegBit<23,1,bool> POPCNT;
		RegBit<25,1,bool> AES;
		RegBit<26,1,bool> XSAVE;
		RegBit<27,1,bool> OSXSAVE;
		RegBit<28,1,bool> AVX;
        RegBit<29,1,bool> F16C;
        // eax=7
		RegBit< 3,1,bool> BMI1;
		RegBit< 4,1,bool> HLE;
		RegBit< 5,1,bool> AVX2;
		RegBit< 8,1,bool> BMI2;
		RegBit<11,1,bool> RTM;
		//eax=80000001H
		RegBit< 0,1,bool> AHF64;
		RegBit< 1,1,bool> CMP;
		RegBit< 2,1,bool> SVM;
		RegBit< 3,1,bool> EAS;
		RegBit< 4,1,bool> CR8D;
		RegBit< 5,1,bool> LZCNT;
		RegBit< 6,1,bool> SSE4A;
		RegBit< 7,1,bool> MSSE;
		RegBit< 8,1,bool> AMD_3DNOW_PREFETCH;
		RegBit< 9,1,bool> OSVW;
		RegBit<10,1,bool> IBS;
		RegBit<12,1,bool> SKINIT;
		RegBit<13,1,bool> WDT;
	};
};

struct CPUID_EDX {
	union {
		int raw;
		// eax=0
		int s2;
		// eax=1
		RegBit< 0,1,bool> FPU;
		RegBit< 1,1,bool> VME;
		RegBit< 2,1,bool> DE;
		RegBit< 3,1,bool> PSE;
		RegBit< 4,1,bool> TSC;
		RegBit< 5,1,bool> MSR;
		RegBit< 6,1,bool> PAE;
		RegBit< 7,1,bool> MCE;
		RegBit< 8,1,bool> CX8;
		RegBit< 9,1,bool> APIC;
		RegBit<11,1,bool> SEP;
		RegBit<12,1,bool> MTRR;
		RegBit<13,1,bool> PGE;
		RegBit<14,1,bool> MCA;
		RegBit<15,1,bool> CMOV;
		RegBit<16,1,bool> PAT;
		RegBit<17,1,bool> PSE36;
		RegBit<18,1,bool> PSN;
		RegBit<19,1,bool> CLFL;
		RegBit<21,1,bool> DTES;
		RegBit<22,1,bool> ACPI;
		RegBit<23,1,bool> MMX;
		RegBit<24,1,bool> FXSR;
		RegBit<25,1,bool> SSE;
		RegBit<26,1,bool> SSE2;
		RegBit<27,1,bool> SS;
		RegBit<28,1,bool> HTT;
		RegBit<29,1,bool> TM1;
		RegBit<30,1,bool> IA64;
		RegBit<31,1,bool> PBE;
		//eax=80000001H
		//RegBit<0-9> from eax=1
		RegBit<11,1,bool> SYSCALL;
		RegBit<16,1,bool> FCMOV;
		RegBit<19,1,bool> MP;
		RegBit<20,1,bool> NX;
		RegBit<22,1,bool> MMX_AMD;
		RegBit<24,1,bool> MMX_CYRIX;
		RegBit<25,1,bool> FFXSR;
		RegBit<26,1,bool> PG1G;
		RegBit<27,1,bool> TSCP; // RDTSCP
		RegBit<29,1,bool> LM;
		RegBit<30,1,bool> AMD_3DNOW_PLUS;
		RegBit<31,1,bool> AMD_3DNOW;
	};
};

enum {
	CPU_SSE		= 0x0002,
	CPU_SSE2	= 0x0004,
	CPU_SSE3	= 0x0008,
	CPU_SSSE3	= 0x0010,
	CPU_SSE41	= 0x0020,
	CPU_SSE42	= 0x0040,
	CPU_AVX		= 0x0080,
	CPU_AVX2	= 0x0100,
	CPU_FMA		= 0x0200
};

int instruction_set() {
	static int flags = 0;

	if (flags)
		return flags;

	struct CPUID_REGS {
		union {
			int raw[4];
			struct { CPUID_EAX eax; CPUID_EBX ebx; CPUID_ECX ecx; CPUID_EDX edx; };
		};
	};

	struct CPUID_REGS regs[16];
	__cpuid(regs[0].raw, 0);
	for (size_t i=1; i<=regs[0].eax.max_index && i<sizeof(regs)/sizeof(CPUID_REGS); ++i)
		__cpuidex(regs[i].raw, i, 0);

	u32 SteppingID = regs[1].eax.SteppingID;
	u32 Family = regs[1].eax.Family;
	u32 Model = regs[1].eax.Model;
	u32 prcessorType = regs[1].eax.ProcessorType;
	u32 ExtendedModel = regs[1].eax.ExtendedModel;

	u32 BrandIndex = regs[1].ebx.BrandIndex;
	u32 CLFLUSH_SIZE = regs[1].ebx.CLFLUSH_SIZE;
	u32 LogicalProcessorCount = regs[1].ebx.LogicalProcessorCount;
	u32 InitialApicId = regs[1].ebx.InitialApicId;

	u32 PossibleCoresCount = regs[4].eax.PossibleCoresCount + 1;

	flags = 1 |
		(regs[1].edx.SSE ? CPU_SSE : 0) |
		(regs[1].edx.SSE2 ? CPU_SSE2 : 0) |
		(regs[1].ecx.SSE3 ? CPU_SSE3 : 0) |
		(regs[1].ecx.SSSE3 ? CPU_SSSE3 : 0) |
		(regs[1].ecx.SSE41 ? CPU_SSE41 : 0) |
		(regs[1].ecx.SSE42 ? CPU_SSE42 : 0) |
		(regs[1].ecx.AVX ? CPU_AVX : 0) |
		(regs[7].ecx.AVX2 ? CPU_AVX2 : 0) |
		(regs[7].ecx.FMA ? CPU_FMA : 0);
	
	return flags;
}

#define _FEATURE_GENERIC_IA32 0x00000001ULL
#define _FEATURE_FPU 0x00000002ULL
#define _FEATURE_CMOV 0x00000004ULL
#define _FEATURE_MMX 0x00000008ULL
#define _FEATURE_FXSAVE 0x00000010ULL
#define _FEATURE_SSE 0x00000020ULL
#define _FEATURE_SSE2 0x00000040ULL
#define _FEATURE_SSE3 0x00000080ULL
#define _FEATURE_SSSE3 0x00000100ULL
#define _FEATURE_SSE4_1 0x00000200ULL
#define _FEATURE_SSE4_2 0x00000400ULL
#define _FEATURE_MOVBE 0x00000800ULL
#define _FEATURE_POPCNT 0x00001000ULL
#define _FEATURE_PCLMULQDQ 0x00002000ULL
#define _FEATURE_AES 0x00004000ULL
#define _FEATURE_F16C 0x00008000ULL
#define _FEATURE_AVX 0x00010000ULL
#define _FEATURE_RDRND 0x00020000ULL
#define _FEATURE_FMA 0x00040000ULL
#define _FEATURE_BMI 0x00080000ULL
#define _FEATURE_LZCNT 0x00100000ULL
#define _FEATURE_HLE 0x00200000ULL
#define _FEATURE_RTM 0x00400000ULL
#define _FEATURE_AVX2 0x00800000ULL
#define _FEATURE_KNCNI 0x04000000ULL
#define _FEATURE_AVX512F 0x08000000ULL
#define _FEATURE_ADX 0x10000000ULL
#define _FEATURE_RDSEED 0x20000000ULL
#define _FEATURE_AVX512ER 0x100000000ULL
#define _FEATURE_AVX512PF 0x200000000ULL
#define _FEATURE_AVX512CD 0x400000000ULL
#define _FEATURE_SHA 0x800000000ULL
#define _FEATURE_MPX 0x1000000000ULL

//?#define _FEATURE_GENERIC_IA64 0x00000000ULL

int _may_i_use_cpu_feature(uint64_t flags) {
	struct CPUID_REGS {
		union {
			int raw[4];
			struct { CPUID_EAX eax; CPUID_EBX ebx; CPUID_ECX ecx; CPUID_EDX edx; };
		};
	};

	CPUID_REGS regs[16];
	__cpuid(regs[0].raw, 0);
	for (size_t i=1; i<=regs[0].eax.max_index && i<sizeof(regs)/sizeof(CPUID_REGS); ++i)
		__cpuidex(regs[i].raw, i, 0);

    int res = 1;
    //(_xgetbv(0) & 0x6) == 0x6; // check xmm&ymm state support
    if (flags & _FEATURE_GENERIC_IA32) ;
    else if (flags & _FEATURE_FPU) res = res && regs[1].edx.FPU != 0;
    else if (flags & _FEATURE_CMOV) res = res && regs[1].edx.CMOV != 0;
    else if (flags & _FEATURE_MMX) res = res && regs[1].edx.MMX != 0;
    else if (flags & _FEATURE_FXSAVE) ;
    else if (flags & _FEATURE_SSE) res = res && regs[1].edx.SSE != 0;
    else if (flags & _FEATURE_SSE2) res = res && regs[1].edx.SSE2 != 0;
    else if (flags & _FEATURE_SSE3) res = res && regs[1].ecx.SSE3 != 0;
    else if (flags & _FEATURE_SSSE3) res = res && regs[1].ecx.SSSE3 != 0;
    else if (flags & _FEATURE_SSE4_1) res = res && regs[1].ecx.SSE41 != 0;
    else if (flags & _FEATURE_SSE4_2) res = res && regs[1].ecx.SSE42 != 0;
    else if (flags & _FEATURE_MOVBE) res = res && regs[1].ecx.MOVBE != 0;
    else if (flags & _FEATURE_POPCNT) res = res && regs[1].ecx.POPCNT != 0;
    else if (flags & _FEATURE_PCLMULQDQ) res = res && regs[1].ecx.PCLMUL != 0; //?
    else if (flags & _FEATURE_AES) res = res && regs[1].ecx.AES != 0;
    else if (flags & _FEATURE_F16C) res = res && (regs[1].ecx.OSXSAVE && regs[1].ecx.AVX && regs[1].ecx.F16C && (_xgetbv(0) & 0x6) == 0x6);
    else if (flags & _FEATURE_AVX) res = res && regs[1].ecx.AVX != 0;
    else if (flags & _FEATURE_RDRND) ;
    else if (flags & _FEATURE_FMA) res = res && regs[7].ecx.FMA != 0;
    else if (flags & _FEATURE_BMI) res = res && (regs[7].ecx.BMI1 != 0 && regs[7].ecx.BMI2 != 0);
    else if (flags & _FEATURE_LZCNT) res = false;//res = res && regs[0x80000001].ecx.LZCNT != 0;
    else if (flags & _FEATURE_HLE) ;
    else if (flags & _FEATURE_RTM) ;
    else if (flags & _FEATURE_AVX2) res = res && regs[7].ecx.AVX2 != 0;
    else if (flags & _FEATURE_KNCNI) ;
    else if (flags & _FEATURE_AVX512F) res = false;
    else if (flags & _FEATURE_ADX) ;
    else if (flags & _FEATURE_RDSEED) ;
    else if (flags & _FEATURE_AVX512ER) res = false;
    else if (flags & _FEATURE_AVX512PF) res = false;
    else if (flags & _FEATURE_AVX512CD) res = false;
    else if (flags & _FEATURE_SHA) ;
	else if (flags & _FEATURE_MPX) res = false;

	return res;
}

///

typedef void (*batch_mul64_type)(vec4t<f64>* const r, const mtx4x4t<f64>& mtx, const size_t count, const vec4t<f64>* const v);

extern batch_mul64_type batch_mul64;

void batch_mul_dispatch(vec4t<f64>* const r, const mtx4x4t<f64>& mtx, const size_t count, const vec4t<f64>* const v) {
	const int flags = instruction_set();
	
	if (flags & CPU_AVX) {
		batch_mul64 = batch_mul_avxi;
	} else if (flags & CPU_SSE2) {
		batch_mul64 = batch_mul_sse2i;
	} else {
		batch_mul64 = batch_mul_pure;
	}

	batch_mul64(r, mtx, count, v);
}

batch_mul64_type batch_mul64 = batch_mul_dispatch;

///

template <typename T> class geocentrical;
template <typename T> class geodetical;

template <typename T>
class geocentrical {
public:
	typedef T type;
	typedef vec3t<type> v3;
	typedef vec4t<type> v4;

public:
	geocentrical& operator = (const geocentrical& gc) {
		lon = gc.lon; lat = gc.lat; h = gc.h;
		return *this;
	}

	geocentrical& operator = (const geodetical<T>& gd);

	geocentrical& operator = (const v3& xyz) {
		return *this;
	}

	operator v3 () const {
		return v3();
	}

	operator v4 () const {
		return v4();
	}

public:
	type lon, lat, h;
};

template <typename T>
class geodetical {
public:
	typedef T type;
	typedef vec3t<type> v3;
	typedef vec4t<type> v4;

public:
	geodetical& operator = (const geocentrical<T>& gc);

	geodetical& operator = (const geodetical& gd) {
		lon = gd.lon; lat = gd.lat; h = gd.h;
		return *this;
	}

	geodetical& operator = (const v3& xyz) {
		return *this;
	}

	operator v3 () const {
		return v3();
	}

	operator v4 () const {
		return v4();
	}

public:
	type lon, lat, h;
};

} // namespace mx
