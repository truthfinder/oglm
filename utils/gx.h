#define gxAlign __declspec(align(16))

//Integer range checks can always be done with a single comparison : (unsigned)(x - min) <= (unsigned)(max - min)

//int abs(int x) { y = x >> 31; return (x ^ y) - y; }
//if (condition) x = -x; ===> x = (x ^ -condition) + condition

/*
Formula Operation / Effect Notes
x & (x - 1)		Clear lowest 1 bit.											If result is 0, then x is 2n.
x | (x + 1)		Set lowest 0 bit.
x | (x - 1)		Set all bits to right of lowest 1 bit.
x & (x + 1)		Clear all bits to right of lowest 0 bit.					If result is 0, then x is 2n - 1.
x & -x			Extract lowest 1 bit.
~x & (x + 1)	Extract lowest 0 bit(as a 1 bit).
~x | (x - 1)		Create mask for bits other than lowest 1 bit.
x | ~(x + 1)	Create mask for bits other than lowest 0 bit.
x | -x			Create mask for bits left of lowest 1 bit, inclusive.
x ^ -x			Create mask for bits left of lowest 1 bit, exclusive.
~x | (x + 1)	Create mask for bits left of lowest 0 bit, inclusive.
~x ^ (x + 1)	Create mask for bits left of lowest 0 bit, exclusive.		Also x == (x + 1).
x ^ (x - 1)		Create mask for bits right of lowest 1 bit, inclusive.		0 becomes -1.
~x & (x - 1)		Create mask for bits right of lowest 1 bit, exclusive.		0 becomes -1.
x ^ (x + 1)		Create mask for bits right of lowest 0 bit, inclusive.		remains -1.
x & (~x - 1)		Create mask for bits right of lowest 0 bit, exclusive.		remains -1.
*/

const float pi = 3.1415926535897932384626433832795f;
float deg2rad(float d) { return d*pi/180.f; }
float rad2deg(float d) { return d*180.f/pi; }
const float gxEps = 1.e-6f;

__forceinline float qfabs(float f) { *(int*)&f &= 0x7fffffff; return f; }
__forceinline int qmod3(int const i) { int const t[] = { 0, 1, 2, 0, 1, 2 }; return t[i]; }
__forceinline bool _less(f32 a, f32 b, f32 t = gxEps) { return a + t < b; }
__forceinline bool _greater(f32 a, f32 b, f32 t = gxEps) { return a - t > b; }
__forceinline bool _lequal(f32 a, f32 b, f32 t = gxEps) { return a + t <= b; }
__forceinline bool _gequal(f32 a, f32 b, f32 t = gxEps) { return a - t >= b; }
__forceinline bool _equal(f32 a, f32 b, f32 t = gxEps) { return fabs(a - b) < t; }
__forceinline bool _nequal(f32 a, f32 b, f32 t = gxEps) { return !_equal(a, b, t); }
__forceinline bool _zero(f32 a, f32 t = gxEps) { return _equal(a, 0.f, t); }
__forceinline bool _nzero(f32 a, f32 t = gxEps) { return !_nequal(a, 0.f, t); }
#if 0
inline f32 __fastcall _length_vec3_avx(f32 const* v) {
	assert(((int)v & 0xf) == 0);
	__m128 u = _mm_maskload_ps(v, _mm_set_epi32(0, -1, -1, -1));
	int r = _mm_extract_ps(_mm_sqrt_ss(_mm_dp_ps(u, u, 0x71)), 0);
	return *(f32*)&r;
}

inline void __fastcall _norm_vec3_avx(f32* r, f32 const* v) {
	assert(((int)v & 0xf) == 0);
	__m128 u = _mm_maskload_ps(v, _mm_set_epi32(0, -1, -1, -1));
	_mm_maskstore_ps(r, _mm_set_epi32(0, -1, -1, -1), _mm_div_ps(u, _mm_sqrt_ps(_mm_dp_ps(u, u, 0x77))));
}
#endif
inline void __vectorcall _transpose_mtx4_sse(__m128* r, __m128 const* m)
{
	// shufps a,b,i -> b3,b2,a1,a0
	// 00,01,02,03		10,11,00,01		00,10,20,30
	// 10,11,12,13		30,31,20,21		01,11,21,31
	// 20,21,22,23		12,13,02,03		02,12,22,32
	// 30,31,32,33		32,33,22,23		03,13,23,33

	assert(((int)r & 0xf) == 0);
	assert(((int)m & 0xf) == 0);

	__m128 tmp0 = _mm_shuf_ps<3, 2, 3, 2>(m[0], m[1]); // 0xEE
	__m128 tmp1 = _mm_shuf_ps<3, 2, 3, 2>(m[2], m[3]); // 0xEE
	__m128 tmp2 = _mm_shuf_ps<1, 0, 1, 0>(m[0], m[1]); // 0x44
	__m128 tmp3 = _mm_shuf_ps<1, 0, 1, 0>(m[2], m[3]); // 0x44

	r[0] = _mm_shuf_ps<1, 3, 1, 3>(tmp1, tmp0); // 0x77
	r[1] = _mm_shuf_ps<0, 2, 0, 2>(tmp1, tmp0); // 0x22
	r[2] = _mm_shuf_ps<1, 3, 1, 3>(tmp3, tmp2); // 0x77
	r[3] = _mm_shuf_ps<0, 2, 0, 2>(tmp3, tmp2); // 0x22
}

inline __m128 __vectorcall _mul_mtx4_vec4(__m128 const* m, __m128 const& v)
{ // u = m * v
	//00, 10, 20, 30	0, 1, 2, 3
	//01, 11, 21, 31
	//02, 12, 22, 32
	//03, 13, 23, 33
	assert(((int)&v & 0xf) == 0);
	assert(((int)m & 0xf) == 0);
	return _mm_madd_ps(m[0], _mm_shufd(v, 3), _mm_madd_ps(m[1], _mm_shufd(v, 2), _mm_madd_ps(m[2], _mm_shufd(v, 1), _mm_mul_ps(m[3], _mm_shufd(v, 0)))));
}
#if 0
inline __m128 __vectorcall _mul_vec4_mtx4(__m128 const& v, __m128 const* m)
{ // u = m * v
	//0	00, 10, 20, 30
	//1	01, 11, 21, 31
	//2	02, 12, 22, 32
	//3	03, 13, 23, 33
	assert(((int)&v & 0xf) == 0);
	assert(((int)m & 0xf) == 0);
	return _mm_add_ps(_mm_add_ps(_mm_dp_ps(m[0], v, 0xf8), _mm_dp_ps(m[1], v, 0xf4)), _mm_add_ps(_mm_dp_ps(m[2], v, 0xf2), _mm_dp_ps(m[3], v, 0xf1)));
}
#endif
inline void __vectorcall _mul_mtx4_mtx4(__m128* r, const __m128* m, const __m128* n)
{
	assert(((int)m & 0xf) == 0);
	assert(((int)n & 0xf) == 0);
	assert(((int)r & 0xf) == 0);
	r[0] = _mm_madd_ps(m[0], _mm_shufd(n[0], 3), _mm_madd_ps(m[1], _mm_shufd(n[0], 2), _mm_madd_ps(m[2], _mm_shufd(n[0], 1), _mm_mul_ps(m[3], _mm_shufd(n[0], 0)))));
	r[1] = _mm_madd_ps(m[0], _mm_shufd(n[1], 3), _mm_madd_ps(m[1], _mm_shufd(n[1], 2), _mm_madd_ps(m[2], _mm_shufd(n[1], 1), _mm_mul_ps(m[3], _mm_shufd(n[1], 0)))));
	r[2] = _mm_madd_ps(m[0], _mm_shufd(n[2], 3), _mm_madd_ps(m[1], _mm_shufd(n[2], 2), _mm_madd_ps(m[2], _mm_shufd(n[2], 1), _mm_mul_ps(m[3], _mm_shufd(n[2], 0)))));
	r[3] = _mm_madd_ps(m[0], _mm_shufd(n[3], 3), _mm_madd_ps(m[1], _mm_shufd(n[3], 2), _mm_madd_ps(m[2], _mm_shufd(n[3], 1), _mm_mul_ps(m[3], _mm_shufd(n[3], 0)))));
}
#if 0
inline void __vectorcall _mul_mtx4_mtx4_avx(__m128* r, __m128 const* m, __m128 const* n) {
	assert(((int)m & 0x1f) == 0);
	assert(((int)n & 0x1f) == 0);
	assert(((int)r & 0x1f) == 0);
	__m256 b0 = _mm256_set_m128(m[0], m[0]);
	__m256 b1 = _mm256_set_m128(m[1], m[1]);
	__m256 b2 = _mm256_set_m128(m[2], m[2]);
	__m256 b3 = _mm256_set_m128(m[3], m[3]);
	__m256 y0 = _mm256_load_ps(&n[0].m128_f32[0]);
	__m256 y1 = _mm256_permute_ps(y0, 0x00);
	__m256 y2 = _mm256_permute_ps(y0, 0x55);
	__m256 y3 = _mm256_permute_ps(y0, 0xAA);
	__m256 y4 = _mm256_permute_ps(y0, 0xFF);
	y0 = _mm256_load_ps(&n[2].m128_f32[0]);
	__m256 y5 = _mm256_permute_ps(y0, 0x00);
	__m256 y6 = _mm256_permute_ps(y0, 0x55);
	__m256 y7 = _mm256_permute_ps(y0, 0xAA);
	__m256 y8 = _mm256_permute_ps(y0, 0xFF);
	y1 = _mm256_mul_ps(y1, b0);
	y2 = _mm256_mul_ps(y2, b1);
	y3 = _mm256_mul_ps(y3, b2);
	y4 = _mm256_mul_ps(y4, b3);
	y1 = _mm256_add_ps(y1, y2);
	y3 = _mm256_add_ps(y3, y4);
	y1 = _mm256_add_ps(y1, y3);
	y5 = _mm256_mul_ps(y5, b0);
	y6 = _mm256_mul_ps(y6, b1);
	y7 = _mm256_mul_ps(y7, b2);
	y8 = _mm256_mul_ps(y8, b3);
	y5 = _mm256_add_ps(y5, y6);
	y7 = _mm256_add_ps(y7, y8);
	y5 = _mm256_add_ps(y5, y7);
	_mm256_stream_ps(&r[0].m128_f32[0], y1);
	_mm256_stream_ps(&r[2].m128_f32[0], y5);
}
#endif
class vec2;
class vec4;

class vec2 {
public:
	vec2() {}
	vec2(f32 a) : x(a), y(a) {}
	vec2(f32 b, f32 a) : x(a), y(b) {}
	vec2(vec2 const& v) : x(v.x), y(v.y) {}
	f32 operator ! () const { return sqrtf(x*x+y*y); }
	vec2 operator - () const { return vec2(-x, -y); }
	vec2 operator ~ () const { f32 f = 1.f/sqrtf(x*x+y*y); return vec2(x*f, y*f); }
	vec2& operator += (vec2 const& v) { y+=v.y; x+=v.x; return *this; }
	vec2& operator -= (vec2 const& v) { y-=v.y; x-=v.x; return *this; }
	vec2& operator *= (vec2 const& v) { y*=v.y; x*=v.x; return *this; }
	vec2& operator /= (vec2 const& v) { y/=v.y; x/=v.x; return *this; }
	vec2& operator += (f32 f) { y+=f; x+=f; return *this; }
	vec2& operator -= (f32 f) { y-=f; x-=f; return *this; }
	vec2& operator *= (f32 f) { y*=f; x*=f; return *this; }
	vec2& operator /= (f32 f) { y/=f; x/=f; return *this; }
	vec2 operator + (vec2 const& v) const { return vec2(y+v.y, x+v.x); }
	vec2 operator - (vec2 const& v) const { return vec2(y-v.y, x-v.x); }
	vec2 operator * (vec2 const& v) const { return vec2(y*v.y, x*v.x); }
	vec2 operator / (vec2 const& v) const { return vec2(y/v.y, x/v.x); }
	vec2 operator + (f32 f) const { return vec2(y+f, x+f); }
	vec2 operator - (f32 f) const { return vec2(y-f, x-f); }
	vec2 operator * (f32 f) const { return vec2(y*f, x*f); }
	vec2 operator / (f32 f) const { return vec2(y/f, x/f); }
	f32 operator ^ (vec2 const& v) const { return x*v.y - y*v.x; }
	f32 operator | (vec2 const& v) const { return x*v.x + y*v.y; }
	operator f32 const* () const { return &y; }
	operator f32* () { return &y; }

	friend vec2 operator + (f32 f, vec2 const& v) { return vec2(f+v.y, f+v.x); }
	friend vec2 operator - (f32 f, vec2 const& v) { return vec2(f-v.y, f-v.x); }
	friend vec2 operator * (f32 f, vec2 const& v) { return vec2(f*v.y, f*v.x); }
	friend vec2 operator / (f32 f, vec2 const& v) { return vec2(f/v.y, f/v.x); }

public:
	f32 y, x;
};

std::ostream& operator << (std::ostream& os, vec2 const& v) {
	return os<<"("<<v.x<<", "<<v.y<<")";
}

class vec3 {
public:
	vec3() {}
	vec3(f32 a) : x(a), y(a), z(a) {}
	vec3(f32 c, f32 b, f32 a) : x(a), y(b), z(c) {}
	vec3(vec3 const& v) : x(v.x), y(v.y), z(v.z) {}
	vec3(vec4 const& v);

	f32 operator ! () const { return sqrtf(x*x + y*y + z*z); }
	vec3 operator - () const { return vec3(-z, -y, -x); }
	vec3 operator ~ () const { f32 f = 1.f/sqrtf(x*x+y*y+z*z); return vec3(z*f, y*f, x*f); }
	vec3& operator += (vec3 const& v) { z+=v.z; y+=v.y; x+=v.x; return *this; }
	vec3& operator -= (vec3 const& v) { z-=v.z; y-=v.y; x-=v.x; return *this; }
	vec3& operator *= (vec3 const& v) { z*=v.z; y*=v.y; x*=v.x; return *this; }
	vec3& operator /= (vec3 const& v) { z/=v.z; y/=v.y; x/=v.x; return *this; }
	vec3& operator += (f32 f) { z+=f; y+=f; x+=f; return *this; }
	vec3& operator -= (f32 f) { z-=f; y-=f; x-=f; return *this; }
	vec3& operator *= (f32 f) { z*=f; y*=f; x*=f; return *this; }
	vec3& operator /= (f32 f) { z/=f; y/=f; x/=f; return *this; }
	vec3 operator + (vec3 const& v) const { return vec3(z+v.z, y+v.y, x+v.x); }
	vec3 operator - (vec3 const& v) const { return vec3(z-v.z, y-v.y, x-v.x); }
	vec3 operator * (vec3 const& v) const { return vec3(z*v.z, y*v.y, x*v.x); }
	vec3 operator / (vec3 const& v) const { return vec3(z/v.z, y/v.y, x/v.x); }
	vec3 operator + (f32 f) const { return vec3(z+f, y+f, x+f); }
	vec3 operator - (f32 f) const { return vec3(z-f, y-f, x-f); }
	vec3 operator * (f32 f) const { return vec3(z*f, y*f, x*f); }
	vec3 operator / (f32 f) const { return vec3(z/f, y/f, x/f); }
	vec3 operator ^ (vec3 const& v) const { return vec3(x*v.y - y*v.x, z*v.x-x*v.z, y*v.z - z*v.y); }
	f32 operator | (vec3 const& v) const { return x*v.x + y*v.y + z*v.z; }
	operator f32 const* () const { return &z; }
	operator f32* () { return &z; }

	friend vec3 operator + (f32 f, vec3 const& v) { return vec3(f+v.z, f+v.y, f+v.x); }
	friend vec3 operator - (f32 f, vec3 const& v) { return vec3(f-v.z, f-v.y, f-v.x); }
	friend vec3 operator * (f32 f, vec3 const& v) { return vec3(f*v.z, f*v.y, f*v.x); }
	friend vec3 operator / (f32 f, vec3 const& v) { return vec3(f/v.z, f/v.y, f/v.x); }

public:
	f32 z, y, x;
};

std::ostream& operator << (std::ostream& os, vec3 const& v) {
	return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

class mtx4;

#define PERM20(a, b) vec2 a##b() const { return vec2(a, b); }
#define PERM2(x, y) PERM20(x, x) PERM20(x, y) PERM20(y, x) PERM20(y, y)

#define PERM30(a, b, c) vec3 a##b##c() const { return vec3(a, b, c); }
#define PERM31(a, b) PERM30(a, b, x) PERM30(a, b, y) PERM30(a, b, z)
#define PERM32(a) PERM31(a, x) PERM31(a, y) PERM31(a, z)
#define PERM3(x, y, z) PERM32(x) PERM32(y) PERM32(z)

#define PERM40(a, b, c, d) vec4 a##b##c##d() const { return shufd<_##a, _##b, _##c, _##d>(); }
#define PERM41(a, b, c) PERM40(a, b, c, x) PERM40(a, b, c, y) PERM40(a, b, c, z) PERM40(a, b, c, w)
#define PERM42(a, b) PERM41(a, b, x) PERM41(a, b, y) PERM41(a, b, z) PERM41(a, b, w)
#define PERM43(a) PERM42(a, x) PERM42(a, y) PERM42(a, z) PERM42(a, w)
#define PERM4(x, y, z, w) PERM43(x) PERM43(y) PERM43(z) PERM43(w)

class gxAlign vec4 {
public:
	vec4() {}
	vec4(f32 a) : fmm(_mm_set1_ps(a)) { }
	vec4(f32 d, f32 c, f32 b, f32 a) : fmm(_mm_set_ps(a, b, c, d)) { }
	vec4(__m128 const& xmm) : fmm(xmm) { }
	vec4(__m128i const& xmm) : imm(xmm) { }
	
	vec4 operator - () const { return _mm_sub_ps(_mm_set1_ps(0.f), fmm); }
	//vec4 operator ~ () const { return _mm_div_ps(fmm, _mm_move_ss(_mm_sqrt_ps(_mm_dp_ps(fmm, fmm, 0xee)), _mm_set_ss(1.f))); }
	vec4& operator = (vec4 const& v) { fmm = v.fmm; return *this; }
	//vec4 qnorm() const { return _mm_mul_ps(fmm, _mm_move_ss(_mm_rsqrt_ps(_mm_dp_ps(fmm, fmm, 0xee)), _mm_set_ss(1.f))); }
	
	vec4& operator += (vec4 const& v) { return *this = _mm_add_ps(fmm, v.fmm); }
	vec4& operator -= (vec4 const& v) { return *this = _mm_sub_ps(fmm, v.fmm); }
	vec4& operator *= (vec4 const& v) { return *this = _mm_mul_ps(fmm, v.fmm); }
	vec4& operator /= (vec4 const& v) { return *this = _mm_div_ps(fmm, v.fmm); } //_mm_mul_ps(*this, _mm_rcp_ss(rv));
	vec4& operator += (__m128 const& v) { return *this = _mm_add_ps(fmm, v); }
	vec4& operator -= (__m128 const& v) { return *this = _mm_sub_ps(fmm, v); }
	vec4& operator *= (__m128 const& v) { return *this = _mm_mul_ps(fmm, v); }
	vec4& operator /= (__m128 const& v) { return *this = _mm_div_ps(fmm, v); } //_mm_mul_ps(*this, _mm_rcp_ss(rv));

	vec4 operator + (vec4 const& v) const { return _mm_add_ps(fmm, v.fmm); }
	vec4 operator - (vec4 const& v) const { return _mm_sub_ps(fmm, v.fmm); }
	vec4 operator * (vec4 const& v) const { return _mm_mul_ps(fmm, v.fmm); }
	vec4 operator / (vec4 const& v) const { return _mm_div_ps(fmm, v.fmm); } //_mm_mul_ps(*this, _mm_rcp_ss(rv));
	vec4 operator + (__m128 const& v) const { return _mm_add_ps(fmm, v); }
	vec4 operator - (__m128 const& v) const { return _mm_sub_ps(fmm, v); }
	vec4 operator * (__m128 const& v) const { return _mm_mul_ps(fmm, v); }
	vec4 operator / (__m128 const& v) const { return _mm_div_ps(fmm, v); } //_mm_mul_ps(*this, _mm_rcp_ss(rv));

	//? u*v.yzx-u.yzx*v
	//(u.yzxw*v.xyzw-u.xyzw*v.yzxw).yzxw
	//uy*vx-ux*vy
	// (U * V.yzx - U.yzx * V).yzx
	//vec4 operator ^ (vec4 const& v) const { return (this->shufd<2,1,3,0>() * v - *this * v.shufd<2,1,3,0>()).shufd<2,1,3,0>(); }
	vec4 operator ^ (vec4 const& v) const { return (*this * v.shufd<2,1,3,0>() - this->shufd<2,1,3,0>() * v).shufd<2,1,3,0>(); }
	vec4 operator % (vec4 const& v) const { return _mm_sub_ps(fmm, _mm_mul_ps(v.fmm, _s1_floor_ps(_mm_div_ps(fmm, v.fmm)))); }
	f32 operator | (vec4 const& v) const { return x*v.x + y*v.y + z*v.z; }

	vec4 operator + (f32 f) const { return _mm_add_ps(*this, _mm_set1_ps(f)); }
	vec4 operator - (f32 f) const { return _mm_sub_ps(*this, _mm_set1_ps(f)); }
	vec4 operator * (f32 f) const { return _mm_mul_ps(*this, _mm_set1_ps(f)); }
	vec4 operator / (f32 f) const { return _mm_div_ps(*this, _mm_set1_ps(f)); }

	friend vec4 operator + (f32 f, vec4 const& v) { return _mm_add_ps(_mm_set1_ps(f), v.fmm); }
	friend vec4 operator - (f32 f, vec4 const& v) { return _mm_sub_ps(_mm_set1_ps(f), v.fmm); }
	friend vec4 operator * (f32 f, vec4 const& v) { return _mm_mul_ps(_mm_set1_ps(f), v.fmm); }
	friend vec4 operator / (f32 f, vec4 const& v) { return _mm_div_ps(_mm_set1_ps(f), v.fmm); }
	friend vec4 operator + (__m128 const& u, vec4 const& v) { return _mm_add_ps(u, v.fmm); }
	friend vec4 operator - (__m128 const& u, vec4 const& v) { return _mm_sub_ps(u, v.fmm); }
	friend vec4 operator * (__m128 const& u, vec4 const& v) { return _mm_mul_ps(u, v.fmm); }
	friend vec4 operator / (__m128 const& u, vec4 const& v) { return _mm_div_ps(u, v.fmm); }

	vec4& operator *= (mtx4 const& m);
	vec4 operator * (mtx4 const& m) const;

	vec4 deg2rad() const { return _mm_mul_ps(fmm, _mm_set1_ps(f32(pi)/180.f)); }
	vec4 rad2deg() const { return _mm_mul_ps(fmm, _mm_set1_ps(180.f/f32(pi))); }

	operator __m128 const& () const { return fmm; }
	operator __m128& () { return fmm; }
	operator __m128i const& () const { return imm; }
	operator __m128i& () { return imm; }
	operator f32 const* () const { return &w; }
	operator f32* () { return &w; }

    template<int i> void set(f32 a);// { fmm = _mm_insert_ps(fmm, _mm_set_ss(a), i << 4); /*(&w)[i] = a;*/ }
    template<> void set<0>(f32 a) { fmm = _mm_move_ss(fmm, _mm_set_ss(a)); }
	template<int i> f32 get() const; // { return _mm_extract_ps(fmm, i); }
	template<> f32 get<0>() const { return _mm_cvtss_f32(fmm); }
	template<int i> vec4 mov(vec4 const& v) const;
	template<> vec4 mov<0>(vec4 const& v) const { return _mm_move_ss(fmm, v.fmm); }

	template<int i> vec4 shuf() const { return _mm_shuffle_ps(fmm, fmm, _MM_SHUFFLE(i, i, i, i)); }
	template<int a, int b, int c, int d> vec4 shuf() const { return _mm_shuffle_ps(fmm, fmm, _MM_SHUFFLE(a, b, c, d)); }
    template<> vec4 shuf<3,2,1,0>() const { return fmm; }
    
	template<int i> vec4 shuf(vec4 const& v) const { return _mm_shuffle_ps(fmm, v.fmm, _MM_SHUFFLE(i, i, i, i)); }
	template<int a, int b, int c, int d> vec4 shuf(vec4 const& v) const { return _mm_shuffle_ps(fmm, v.fmm, _MM_SHUFFLE(a, b, c, d)); }
	template<> vec4 shuf<3,2,1,0>(vec4 const& v) const { return fmm; }
	
	template<int i> vec4 shufd() const { return _mm_castsi128_ps(_mm_shuffle_epi32(imm, _MM_SHUFFLE(i, i, i, i))); }
	template<int a, int b, int c, int d> vec4 shufd() const { return _mm_castsi128_ps(_mm_shuffle_epi32(imm, _MM_SHUFFLE(a, b, c, d))); }
	template<> vec4 shufd<3,2,1,0>() const { return fmm; }

	template<int i> vec4 mov(f32 f) const; // { return _mm_insert_ps(fmm, _mm_extract_ps(fmm, i), i << 4); }
	template<> vec4 mov<0>(f32 f) const { return _mm_move_ss(fmm, _mm_set_ss(f)); }

	vec4 floor() const { __m128i w = _mm_cvttps_epi32(fmm); return _mm_cvtepi32_ps(_mm_add_epi32(w, _mm_castps_si128(_mm_cmplt_ps(fmm, _mm_cvtepi32_ps(w))))); }
	vec4  ceil() const { __m128i w = _mm_cvttps_epi32(fmm); return _mm_cvtepi32_ps(_mm_add_epi32(w, _mm_castps_si128(_mm_cmpgt_ps(fmm, _mm_cvtepi32_ps(w))))); }
	vec4 sqrt() const { return _mm_sqrt_ps(fmm); } // 24
	vec4 sqrt11() const { return _mm_rcp_ps(_mm_rsqrt_ps(fmm)); }
	vec4 rcp() const { return _mm_div_ps(_mm_set1_ps(1.f), fmm); }
	vec4 rcp11() const { return _mm_rcp_ps(fmm); }
	vec4 rcp22() const { vec4 r = rcp(); return (vec4(2.f)-*this*r)*r; } // 2 * rcpss(x) - (x * rcpss(x) * rcpss(x))
	vec4 rsqrt() const { return _mm_div_ps(_mm_set1_ps(1.f), _mm_sqrt_ps(fmm)); }
	vec4 rsqrt11() const { return _mm_rsqrt_ps(fmm); }
	vec4 rsqrt22() const { vec4 r = rsqrt(); return vec4(.5f)*r*(vec4(3.f)-*this*r*r); } // .5 * rsqrtss * (3 - x * rsqrtss(x) * rsqrtss(x))
	vec4 hsub(vec4 const& v) const { return _mm_hsub_ps(fmm, v.fmm); }
	vec4 min(vec4 const& v) const { return _mm_min_ps(fmm, v.fmm); }
	vec4 max(vec4 const& v) const { return _mm_max_ps(fmm, v.fmm); }
	vec4 and(vec4 const& v) const { return _mm_and_ps(fmm, v.fmm); }
	vec4 nand(vec4 const& v) const { return _mm_andnot_ps(fmm, v.fmm); }
	vec4  or(vec4 const& v) const { return _mm_or_ps(fmm, v.fmm); }
	vec4 xor(vec4 const& v) const { return _mm_xor_ps(fmm, v.fmm); }
	vec4 mix(vec4 const& v, vec4 const& f) const { return _mm_add_ps(_mm_mul_ps(_mm_sub_ps(v.fmm, fmm), f.fmm), fmm); }
	vec4 mux(vec4 const& u, vec4 const& v) const { return _mm_or_ps(_mm_and_ps(fmm, u), _mm_andnot_ps(fmm, v)); } // sse2:or || sse41:blend
	vec4 mad(vec4 const& f, vec4 const& v) const { return _mm_madd_ps(fmm, f, v); }
	//vec4 sin() const { return ; }
	//vec4 cos() const { return ; }
	//vec4 tan() const { return ; }
	//vec4 qlog2() const { return ; }

	PERM4(x, y, z, w);

	//static vec4 zero() const { return _mm_setzero_ps(); }
	//static vec4 one() const { return _mm_set1_ps(1.f); }
	//static vec4 _255() const { return _mm_set1_ps(255.f); }

public:
	enum { _w, _z, _y, _x };
	union gxAlign {
		struct { f32 w, z, y, x; };
		//struct { f32 q, p, t, s; };
		//struct { f32 a, r, g, b; };
		__m128 fmm;
		__m128i imm;
	};
};

vec3::vec3(vec4 const& v) : x(v.x), y(v.y), z(v.z) {}

std::ostream& operator << (std::ostream& os, vec4 const& v) {
	return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")" << std::endl;
}

class gxAlign mtx4 {
public:
	operator __m128 const* () const { return fmm; }
	operator __m128* () { return fmm; }

	mtx4() {}
#	if _MSC_VER < 1900
	mtx4(mtx4 const& m) { fmm[0] = m.fmm[0]; fmm[1] = m.fmm[1]; fmm[2] = m.fmm[2]; fmm[3] = m.fmm[3]; }
	mtx4(__m128 const m[4]) { fmm[0] = m[0]; fmm[1] = m[1]; fmm[2] = m[2]; fmm[3] = m[3]; }
	mtx4(__m128 const& a, __m128 const& b, __m128 const& c, __m128 const& d) { fmm[0] = a; fmm[1] = b; fmm[2] = c; fmm[3] = d; }
#	else
	mtx4(mtx4 const& m) : fmm{ m.fmm[0], m.fmm[1], m.fmm[2], m.fmm[3] } {}
	mtx4(__m128 const m[4]) : fmm{ m[0], m[1], m[2], m[3] } {}
	mtx4(__m128 const& a, __m128 const& b, __m128 const& c, __m128 const& d) : fmm{ a, b, c, d } {}
#	endif
	mtx4(f32 m30, f32 m20, f32 m10, f32 m00,
		f32 m31, f32 m21, f32 m11, f32 m01,
		f32 m32, f32 m22, f32 m12, f32 m02,
		f32 m33, f32 m23, f32 m13, f32 m03)
		: _00(m00), _10(m10), _20(m20), _30(m30)
		, _01(m01), _11(m11), _21(m21), _31(m31)
		, _02(m02), _12(m12), _22(m22), _32(m32)
		, _03(m03), _13(m13), _23(m23), _33(m33)
	{
	}
	mtx4& operator = (mtx4 const& m) {
		fmm[0] = m.fmm[0];
		fmm[1] = m.fmm[1];
		fmm[2] = m.fmm[2];
		fmm[3] = m.fmm[3];
		return *this;
	}
	mtx4& operator *= (mtx4 const& m) {
		mtx4 r;
		_mul_mtx4_mtx4(r, *this, m);
		return *this = r;
	}
	mtx4 operator * (mtx4 const& m) const {
		mtx4 r;
		_mul_mtx4_mtx4(r, *this, m);
		return r;
	}
	vec4 operator * (vec4 const& v) const {
		return _mul_mtx4_vec4(*this, v);
	}
	mtx4 operator ~ () const {
		mtx4 r;
		_transpose_mtx4_sse(r, *this);
		return r;
	}
	static mtx4 iden() {
		static mtx4 m(
			0.f, 0.f, 0.f, 1.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f, 
			1.f, 0.f, 0.f, 0.f);
		return m;
	}

	static mtx4 sca(f32 x, f32 y, f32 z) {
		return mtx4(
			0.f, 0.f, 0.f,   x,
			0.f, 0.f,   y, 0.f,
			0.f,   z, 0.f, 0.f,
			1.f, 0.f, 0.f, 0.f);
	}

	static mtx4 tra(f32 x, f32 y, f32 z) {
		return mtx4(
			0.f, 0.f, 0.f, 1.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			1.f,   z,   y,   x);
	}

	static mtx4 rot(f32 deg, f32 x, f32 y, f32 z) {
		vec3 v = ~vec3(z, y, x);
		const f32 rad = deg2rad(deg);
		const f32 p = sinf(rad);
		const f32 q = cosf(rad);
		const f32 oq = 1.f - q;
		const f32 aoq = v.x*oq;
		const f32 boq = v.y*oq;
		const f32 coq = v.z*oq;
		const f32 aoqc = aoq*v.z;
		const f32 aoqb = v.x*boq;
		const f32 boqc = v.y*coq;
		const f32 ap = v.x*p;
		const f32 bp = v.y*p;
		const f32 cp = v.z*p;

		return mtx4(
			0.f,   aoqc-bp,   aoqb+cp, v.x*aoq+q,
			0.f,   boqc+ap, v.y*boq+q,   aoqb-cp,
			0.f, v.z*coq+q,   boqc-ap,   aoqc+bp,
			1.f,       0.f,       0.f,       0.f);
	}
#	if 0
	static mtx4 rot2(f32 deg, f32 x, f32 y, f32 z) {
		const vec4 v = ~vec4(1.f, z, y, x);
		const f32 rad = deg2rad(deg), p = sinf(rad), q = cosf(rad);

		__m128 r = _mm_set_ps(q, p,-p, 0.f);
		__m128 u = _mm_move_ss(_mm_set1_ps(1.f-q), _mm_set_ss(0.f));
		//__m128 u = _mm_mul_ps(v, _mm_set_ps(1.f-q, 1.f-q, 1.f-q, 0.f));
		__m128 t0 = _mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.x)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0,1,2,0)), r));
		__m128 t1 = _mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.y)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1,0,3,0)), _mm_shuffle_ps(r, r, _MM_SHUFFLE(1,3,2,0))));
		__m128 t2 = _mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.z)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,0)), _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,1,3,0))));

		return mtx4(
			_mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.x)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0,1,2,0)), r)),
			_mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.y)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1,0,3,0)), _mm_shuffle_ps(r, r, _MM_SHUFFLE(1,3,2,0)))),
			_mm_add_ps(_mm_mul_ps(u, _mm_set1_ps(v.z)), _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,0)), _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,1,3,0)))),
			_mm_set_ps(0.f, 0.f, 0.f, 1.f));

		//return mtx4(
		//	0.f, v.z*v.x*oq - p*v.y, v.y*v.x*oq + p*v.z, v.x*v.x*oq + q*v.w,
		//	0.f, v.z*v.y*oq + p*v.x, v.y*v.y*oq + q*v.w, v.x*v.y*oq - p*v.z,
		//	0.f, v.z*v.z*oq + q*v.w, v.y*v.z*oq - p*v.x, v.x*v.z*oq + p*v.y,
		//	1.f,                0.f,                0.f,                0.f);
	}
#	endif
	static mtx4 ort(f32 l, f32 r, f32 b, f32 t, f32 n = -1.f, f32 f = 1.f) {
		return mtx4(
			0.f, 0.f, 0.f, 2.f/(r-l),
			0.f, 0.f, 2.f/(t-b), 0.f,
			0.f, 2.f/(n-f), 0.f, 0.f,
			1.f, (f+n)/(n-f), (t+b)/(b-t), (r+l)/(l-r));
	}

	static mtx4 fru(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
		return mtx4(
            0.f, 0.f, 0.f, 2.f*n/(r-l),
			0.f, 0.f, 2.f*n/(t-b), 0.f,
			-1.f, (n+f)/(n-f), (t+b)/(t-b), (r+l)/(r-l),
			0.f, 2.f*f*n/(n-f), 0.f, 0.f);
	}

	static mtx4 per(f32 fovy, f32 aspect, f32 n, f32 f) {
		return mtx4(
			0.f, 0.f, 0.f, 1.f/(tan(deg2rad(fovy)*.5f)*aspect),
			0.f, 0.f, 1.f/tan(deg2rad(fovy)*.5f), 0.f,
			-1.f, (n+f)/(n-f), 0.f, 0.f,
			0.f, 2.f*n*f/(n-f), 0.f, 0.f);
	}

	static mtx4 per2(f32 fovy, f32 aspect, f32 n, f32 f) {
		f32 t = n * tan(deg2rad(fovy) * .5f);
		f32 r = t * aspect;
		return fru(-r, r, -t, t, n, f);
	}

	static mtx4 lookat(vec3 const& eye, vec3 const& at, vec3 const& up) {
		// v3 Pc = v3(ex, ey, ez)
		// v3 Zc = !(Pc - v3(cx, cy, cz))
		// v3 Yc = !v3(ux, uy, uz)
		// v3 Xc = !(Yc ^ Zc)
		// Yc = Zc ^ Xc
		// Xc.x Xc.y Xc.z -dot(Xc, Pc)
		// Yc.x Yc.y Yc.z -dot(Yc, Pc)
		// Zc.x Zc.y Zc.z -dot(Zc, Pc)
		//    0    0    0            1
		const vec3 fwd = ~(at - eye);
		const vec3 side = ~(fwd ^ up);
		const vec3 u = side ^ fwd;
		return mtx4(
			0.f, side.z, side.y, side.x,
			0.f,    u.z,    u.y,    u.x,
			0.f, -fwd.z, -fwd.y, -fwd.x,
			1.f, fwd.z*eye.z-side.z*eye.x-u.z*eye.y, fwd.y*eye.z-side.y*eye.x-u.y*eye.y, fwd.x*eye.z-side.x*eye.x-u.x*eye.y);
	}

	static mtx4 lookat2(vec3 const& eye, vec3 const& at, vec3 const& up) {
		// v3 Pc = v3(ex, ey, ez)
		// v3 Zc = !(Pc - v3(cx, cy, cz))
		// v3 Yc = !v3(ux, uy, uz)
		// v3 Xc = !(Yc ^ Zc)
		// Yc = Zc ^ Xc
		const vec3 z = ~(eye - at);
		const vec3 y = ~up;
		const vec3 x = ~(y ^ z);
		return mtx4(
			0.f, z.x, y.x, x.x,
			0.f, z.y, y.y, x.y,
			0.f, z.z, y.z, x.z,
			1.f, -(z|eye), -(y|eye), -(x|eye));
	}

	f32 det() const {
		return
			(_00*_11 - _01*_10) * (_22*_33 - _23*_32) -
			(_00*_12 - _02*_10) * (_21*_33 - _23*_31) +
			(_00*_13 - _03*_10) * (_21*_32 - _22*_31) +
			(_01*_12 - _02*_11) * (_20*_33 - _23*_30) -
			(_01*_13 - _03*_11) * (_20*_32 - _22*_30) +
			(_02*_13 - _03*_12) * (_20*_31 - _21*_30);
	}

	mtx4 operator ! () const {
		f32 dt = det();
		dt = _zero(dt) ? 1.0f : 1.0f/dt;

		#define DET3(r1, r2, r3, c1, c2, c3) \
			_##r1##c1 * (_##r2##c2*_##r3##c3 - _##r2##c3*_##r3##c2) - \
			_##r1##c2 * (_##r2##c1*_##r3##c3 - _##r2##c3*_##r3##c1) + \
			_##r1##c3 * (_##r2##c1*_##r3##c2 - _##r2##c2*_##r3##c1)
			
		mtx4 r;

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

    f32 operator () (size_t i, size_t j) const { return m[i][j]; }
public:
	union gxAlign {
		__m128 fmm[4];
		f32 m[4][4];
		struct { f32
			_30, _20, _10, _00,
			_31, _21, _11, _01,
			_32, _22, _12, _02,
			_33, _23, _13, _03; };
	};
};
#if 0
vec4& vec4::operator *= (mtx4 const& m) {
	return *this = _mul_vec4_mtx4(*this, m);
}

vec4 vec4::operator * (mtx4 const& m) const {
	return _mul_vec4_mtx4(*this, m);
}
#endif
std::ostream& operator << (std::ostream& os, mtx4 const& m) {
	return os <<
		"(" << m._00 << ", " << m._01 << ", " << m._02 << ", " << m._03 << ")" << std::endl <<
		"(" << m._10 << ", " << m._11 << ", " << m._12 << ", " << m._13 << ")" << std::endl <<
		"(" << m._20 << ", " << m._21 << ", " << m._22 << ", " << m._23 << ")" << std::endl <<
		"(" << m._30 << ", " << m._31 << ", " << m._32 << ", " << m._33 << ")";
}

class gxAlign tex4 { // multitex coord
public:
	tex4() {}
	tex4(f32 _v0, f32 _u0) : fmm(_mm_set_ps(0.f, 0.f, _u0, _v0)) {}
	tex4(f32 _v1, f32 _u1, f32 _v0, f32 _u0) : fmm(_mm_set_ps(_u1, _v1, _u0, _v0)) {}
	tex4(vec2 const& t0) : fmm(_mm_set_ps(0.f, 0.f, t0.x, t0.y)) {}
	tex4(vec2 const& t1, vec2 const& t0) : fmm(_mm_set_ps(t1.x, t1.y, t0.x, t0.y)) {}
	tex4(__m128 const& t) : fmm(t) {}
	tex4(__m128i const& t) : imm(t) {}
	tex4(vec4 const& v) : fmm(v.fmm) {}

	tex4& operator = (tex4 const& t) { fmm = t.fmm; return *this; }
	tex4& operator = (__m128 const& xmm) { fmm = xmm; return *this; }

	tex4& operator += (tex4 const& t) { return *this = _mm_add_ps(fmm, t.fmm); }
	tex4& operator -= (tex4 const& t) { return *this = _mm_sub_ps(fmm, t.fmm); }
	tex4& operator *= (tex4 const& t) { return *this = _mm_mul_ps(fmm, t.fmm); }
	tex4& operator /= (tex4 const& t) { return *this = _mm_div_ps(fmm, t.fmm); }
	tex4& operator += (vec4 const& v) { return *this = _mm_add_ps(fmm, v.fmm); }
	tex4& operator -= (vec4 const& v) { return *this = _mm_sub_ps(fmm, v.fmm); }
	tex4& operator *= (vec4 const& v) { return *this = _mm_mul_ps(fmm, v.fmm); }
	tex4& operator /= (vec4 const& v) { return *this = _mm_div_ps(fmm, v.fmm); }
	tex4& operator += (__m128 const& v) { return *this = _mm_add_ps(fmm, v); }
	tex4& operator -= (__m128 const& v) { return *this = _mm_sub_ps(fmm, v); }
	tex4& operator *= (__m128 const& v) { return *this = _mm_mul_ps(fmm, v); }
	tex4& operator /= (__m128 const& v) { return *this = _mm_div_ps(fmm, v); }
	tex4& operator += (f32 const f) { return *this = _mm_add_ps(fmm, _mm_set1_ps(f)); }
	tex4& operator -= (f32 const f) { return *this = _mm_sub_ps(fmm, _mm_set1_ps(f)); }
	tex4& operator *= (f32 const f) { return *this = _mm_mul_ps(fmm, _mm_set1_ps(f)); }
	tex4& operator /= (f32 const f) { return *this = _mm_div_ps(fmm, _mm_set1_ps(f)); }

	tex4 operator + (tex4 const& t) const { return _mm_add_ps(fmm, t.fmm); }
	tex4 operator - (tex4 const& t) const { return _mm_sub_ps(fmm, t.fmm); }
	tex4 operator * (tex4 const& t) const { return _mm_mul_ps(fmm, t.fmm); }
	tex4 operator / (tex4 const& t) const { return _mm_div_ps(fmm, t.fmm); }
	tex4 operator + (vec4 const& v) const { return _mm_add_ps(fmm, v.fmm); }
	tex4 operator - (vec4 const& v) const { return _mm_sub_ps(fmm, v.fmm); }
	tex4 operator * (vec4 const& v) const { return _mm_mul_ps(fmm, v.fmm); }
	tex4 operator / (vec4 const& v) const { return _mm_div_ps(fmm, v.fmm); }
	tex4 operator + (__m128 const& v) const { return _mm_add_ps(fmm, v); }
	tex4 operator - (__m128 const& v) const { return _mm_sub_ps(fmm, v); }
	tex4 operator * (__m128 const& v) const { return _mm_mul_ps(fmm, v); }
	tex4 operator / (__m128 const& v) const { return _mm_div_ps(fmm, v); }
	tex4 operator + (f32 const f) const { return _mm_add_ps(fmm, _mm_set1_ps(f)); }
	tex4 operator - (f32 const f) const { return _mm_sub_ps(fmm, _mm_set1_ps(f)); }
	tex4 operator * (f32 const f) const { return _mm_mul_ps(fmm, _mm_set1_ps(f)); }
	tex4 operator / (f32 const f) const { return _mm_div_ps(fmm, _mm_set1_ps(f)); }

	friend tex4 operator + (f32 const f, tex4 const& t) { return _mm_add_ps(_mm_set1_ps(f), t.fmm); }
	friend tex4 operator - (f32 const f, tex4 const& t) { return _mm_sub_ps(_mm_set1_ps(f), t.fmm); }
	friend tex4 operator * (f32 const f, tex4 const& t) { return _mm_mul_ps(_mm_set1_ps(f), t.fmm); }
	friend tex4 operator / (f32 const f, tex4 const& t) { return _mm_div_ps(_mm_set1_ps(f), t.fmm); }
	operator __m128 const& () const { return fmm; }
	operator __m128& () { return fmm; }
	operator __m128i const& () const { return imm; }
	operator __m128i& () { return imm; }
	operator f32 const* () const { return &v1; }
	operator f32* () { return &v1; }
	operator i32 const* () const { return &iv1; }
	operator i32* () { return &iv1; }

	template <int i> f32 getf() { return _mm_cvtss_f32(_mm_castsi128_ps(_mm_srli_si128(imm, i * 4))); }
	template <> f32 getf<0>() { return _mm_cvtss_f32(fmm); }
	template <int i> i32 geti() { return _mm_cvtsi128_si32(_mm_srli_si128(imm, i * 4)); }
	template <> i32 geti<0>() { return _mm_cvtsi128_si32(imm); }

	friend tex4 operator + (__m128 const& u, tex4 const& v) { return _mm_add_ps(u, v.fmm); }
	friend tex4 operator - (__m128 const& u, tex4 const& v) { return _mm_sub_ps(u, v.fmm); }
	friend tex4 operator * (__m128 const& u, tex4 const& v) { return _mm_mul_ps(u, v.fmm); }
	friend tex4 operator / (__m128 const& u, tex4 const& v) { return _mm_div_ps(u, v.fmm); }
	friend tex4 operator + (vec4 const& u, tex4 const& v) { return _mm_add_ps(u.fmm, v.fmm); }
	friend tex4 operator - (vec4 const& u, tex4 const& v) { return _mm_sub_ps(u.fmm, v.fmm); }
	friend tex4 operator * (vec4 const& u, tex4 const& v) { return _mm_mul_ps(u.fmm, v.fmm); }
	friend tex4 operator / (vec4 const& u, tex4 const& v) { return _mm_div_ps(u.fmm, v.fmm); }

	tex4 mad(tex4 const& f, tex4 const& v) const { return _mm_madd_ps(fmm, f.fmm, v.fmm); }

public:
	//TODO: reg128:u1,v1,u0,v0 due to t4.hadd().get<0>()
	union gxAlign {
		struct { f32 v0, u0, v1, u1; };
		__m128 fmm; // xmm: u1, v1, u0, v0
		struct { i32 iv0, iu0, iv1, iu1; };
		__m128i imm;
		struct { f32 f0, f1, f2, f3; };
		struct { i32 i0, i1, i2, i3; };
	};
};

std::ostream& operator << (std::ostream& os, tex4 const& t) {
	return os << "(" << t.u0 << ", " << t.v0 << ", " << t.u1 << ", " << t.v1 << ")";
}

class gxAlign col4 { // swap argb for col8us
public:
	col4() {}
	col4(__m128 const& c) : fmm(c) {}
	explicit col4(int const c) : fmm(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(c), _mm_set1_epi32(0)), _mm_set1_epi32(0))), _mm_set1_ps(1.f / 255.f))) {}
	explicit col4(f32 _i) : fmm(_mm_set_ps(_i, _i, _i, _i)) {}
	explicit col4(f32 _r, f32 _g, f32 _b) : fmm(_mm_set_ps(1.f, _r, _g, _b)) {}
	explicit col4(f32 _a, f32 _r, f32 _g, f32 _b) : fmm(_mm_set_ps(_a, _r, _g, _b)) {}
	col4(u8 _r, u8 _g, u8 _b) : fmm(_mm_mul_ps(_mm_cvtepi32_ps(_mm_set_epi32(255, _r, _g, _b)), _mm_set1_ps(1.f / 255.f))) { }
	col4(u8 _a, u8 _r, u8 _g, u8 _b) : fmm(_mm_mul_ps(_mm_cvtepi32_ps(_mm_set_epi32(_a, _r, _g, _b)), _mm_set1_ps(1.f / 255.f))) { }
	col4& operator = (col4 const& c) { fmm = c.fmm; return *this; }
	col4& operator = (__m128 const& c) { fmm = c; return *this; }
	col4& operator = (f32 const f) { fmm = _mm_set1_ps(f); return *this; }
	
	col4& operator += (col4 const& c) { return *this = _mm_add_ps(fmm, c.fmm); }
	col4& operator -= (col4 const& c) { return *this = _mm_sub_ps(fmm, c.fmm); }
	col4& operator *= (col4 const& c) { return *this = _mm_mul_ps(fmm, c.fmm); }
	col4& operator /= (col4 const& c) { return *this = _mm_div_ps(fmm, c.fmm); }
	col4& operator += (__m128 const& v) { return *this = _mm_add_ps(fmm, v); }
	col4& operator -= (__m128 const& v) { return *this = _mm_sub_ps(fmm, v); }
	col4& operator *= (__m128 const& v) { return *this = _mm_mul_ps(fmm, v); }
	col4& operator /= (__m128 const& v) { return *this = _mm_div_ps(fmm, v); }
	col4& operator += (f32 const f) { return *this = _mm_add_ps(fmm, _mm_set1_ps(f)); }
	col4& operator -= (f32 const f) { return *this = _mm_sub_ps(fmm, _mm_set1_ps(f)); }
	col4& operator *= (f32 const f) { return *this = _mm_mul_ps(fmm, _mm_set1_ps(f)); }
	col4& operator /= (f32 const f) { return *this = _mm_div_ps(fmm, _mm_set1_ps(f)); }
	
	col4 operator + (col4 const& c) const { return _mm_add_ps(fmm, c.fmm); }
	col4 operator - (col4 const& c) const { return _mm_sub_ps(fmm, c.fmm); }
	col4 operator * (col4 const& c) const { return _mm_mul_ps(fmm, c.fmm); }
	col4 operator / (col4 const& c) const { return _mm_div_ps(fmm, c.fmm); }
	col4 operator + (__m128 const& v) const { return _mm_add_ps(fmm, v); }
	col4 operator - (__m128 const& v) const { return _mm_sub_ps(fmm, v); }
	col4 operator * (__m128 const& v) const { return _mm_mul_ps(fmm, v); }
	col4 operator / (__m128 const& v) const { return _mm_div_ps(fmm, v); }
	col4 operator + (f32 const f) const { return _mm_add_ps(fmm, _mm_set1_ps(f)); }
	col4 operator - (f32 const f) const { return _mm_sub_ps(fmm, _mm_set1_ps(f)); }
	col4 operator * (f32 const f) const { return _mm_mul_ps(fmm, _mm_set1_ps(f)); }
	col4 operator / (f32 const f) const { return _mm_div_ps(fmm, _mm_set1_ps(f)); }
	
	friend col4 operator + (f32 const f, col4 const& c) { return _mm_add_ps(_mm_set1_ps(f), c.fmm); }
	friend col4 operator - (f32 const f, col4 const& c) { return _mm_sub_ps(_mm_set1_ps(f), c.fmm); }
	friend col4 operator * (f32 const f, col4 const& c) { return _mm_mul_ps(_mm_set1_ps(f), c.fmm); }
	friend col4 operator / (f32 const f, col4 const& c) { return _mm_div_ps(_mm_set1_ps(f), c.fmm); }
	operator __m128 const& () const { return fmm; }
	operator __m128& () { return fmm; }
	operator f32 const* () const { return &b; }
	operator f32* () { return &b; }

	int irgba() const {
		__m128i fc = _mm_cvttps_epi32(_mm_mul_ps(fmm, _mm_set1_ps(255.f)));
		fc = _mm_packs_epi32(fc, fc); // sse2, from sse41::_mm_packus_epi32
		return _mm_cvtsi128_si32(_mm_packus_epi16(fc, fc));
	}

	operator u32() const { return (u32)irgba(); }
	operator i32() const { return irgba(); }

	static __m128 clampr(__m128 const& c) { return _mm_min_ps(c, _mm_set1_ps(1.f)); }
	static __m128 clampl(__m128 const& c) { return _mm_max_ps(c, _mm_set1_ps(0.f)); }
	static __m128 clamp(__m128 const& c) { return _mm_max_ps(_mm_min_ps(c, _mm_set1_ps(1.f)), _mm_set1_ps(0.f)); }
	static col4 check(col4 const& c) {
		assert(c.b>=0.f && c.g>=0.f && c.r>=0.f && c.a>=0.f);
		assert(c.b<=1.f && c.g<=1.f && c.r<=1.f && c.a<=1.f);
		return c;
	}

	friend col4 operator + (__m128 const& u, col4 const& v) { return _mm_add_ps(u, v.fmm); }
	friend col4 operator - (__m128 const& u, col4 const& v) { return _mm_sub_ps(u, v.fmm); }
	friend col4 operator * (__m128 const& u, col4 const& v) { return _mm_mul_ps(u, v.fmm); }
	friend col4 operator / (__m128 const& u, col4 const& v) { return _mm_div_ps(u, v.fmm); }

	col4 mad(col4 const& f, col4 const& v) const { return _mm_madd_ps(fmm, f.fmm, v.fmm); }

public:
	union gxAlign {
		__m128 fmm;
		struct { f32 b, g, r, a; };
	};
};

class gxAlign col8us {
public:
	static const col8us cmask, rmask, gmask, bmask, amask, one, zero;

	union gxAlign {
        __m128i imm;
		struct { i16 b0, g0, r0, a0, b1, g1, r1, a1; };
    };

	col8us() {}
	col8us(__m128i const& v) : imm(v) {}
	col8us(col8us const& c) : imm(c.imm) {}
	col8us(col4 const& c) : imm(_mm_packs_epi32(_mm_cvttps_epi32(_mm_mul_ps(c, _mm_set1_ps(255.f))), _mm_setzero_si128())) {}
	col8us(i16 a0, i16 r0, i16 g0, i16 b0) : imm(_mm_set_epi16(0,0,0,0,a0,r0,g0,b0)) {}
	col8us(i16 a0, i16 r0, i16 g0, i16 b0, i16 a1, i16 r1, i16 g1, i16 b1) : imm(_mm_set_epi16(a1,r1,g1,b1,a0,r0,g0,b0)) {}
	col8us(i32 c) : imm(_mm_unpacklo_epi8(i2imm(c), _mm_setzero_si128())) {} // r:0xbbggrraa
	col8us(i32 c0, i32 c1) : imm(_mm_unpacklo_epi8(_mm_set_epi32(0, 0, c1, c0), _mm_setzero_si128())) {} // r:0xbbggrraa

	operator __m128i () const { return imm; }
	operator __m128i& () { return imm; }
	operator u32 () const { return (u32 const&)as_int<0>(); }
	operator i32 () const { return as_int<0>(); }
	template <int i> i32 as_int() const { return _mm_cvtsi128_si32(_mm_srli_si128(_mm_packus_epi16(imm, _mm_setzero_si128()), i * 4)); }
	template <> i32 as_int<0>() const { return _mm_cvtsi128_si32(_mm_packus_epi16(imm, _mm_setzero_si128())); }

	col8us& operator += (col8us const& c) { return *this = *this + c; }
	col8us& operator -= (col8us const& c) { return *this = *this - c; }
	col8us& operator *= (col8us const& c) { return *this = *this * c; }

	col8us operator + (col8us const& c) const { return clamp(_mm_add_epi16(imm, c.imm)); }
	col8us operator - (col8us const& c) const { return clamp(_mm_sub_epi16(imm, c.imm)); }
	col8us operator * (col8us const& c) const { return _mm_srli_epi16(_mm_mullo_epi16(imm, c.imm), 8); }

	static col8us i2imm(int i) { return _mm_castps_si128(_mm_set_ss(*(float*)&i)); } // _mm_cvtsi32_si128, but movss is faster, frees port 5
	static col8us clamp(col8us const& c) { return _mm_min_epi16(_mm_max_epi16(c.imm, _mm_setzero_si128()), _mm_set1_epi16(255)); }
	template <int i> col8us mov(col8us const& c) const;
	template <> col8us mov<0>(col8us const& c) const { return bmask.mux(c.imm, imm); }
	template <> col8us mov<1>(col8us const& c) const { return gmask.mux(c.imm, imm); }
	template <> col8us mov<2>(col8us const& c) const { return rmask.mux(c.imm, imm); }
	template <> col8us mov<3>(col8us const& c) const { return amask.mux(c.imm, imm); }
	template <int i> col8us mul(col8us const& c) const { return mov<i>(*this * c); }

	template <int i> col8us& set(int a) { return *this = _mm_insert_epi16(imm, a, i); }

	template <int i> short get() const { return _mm_extract_epi16(imm, i); }

	template <int i> col8us shuf() const { return _mm_shufflelo_epi16(imm, _MM_SHUFFLE(i, i, i, i)); }
	template <int a, int b, int c, int d> col8us shuf() const { return _mm_shufflelo_epi16(imm, _MM_SHUFFLE(a, b, c, d)); }
	template <> col8us shuf<3, 2, 1, 0>() const { return imm; }

	col8us min(col8us const& c) const { return _mm_min_epi16(imm, c.imm); }
	col8us max(col8us const& c) const { return _mm_max_epi16(imm, c.imm); }
	col8us mix(col8us const& c, col8us const& f) const { return (c - *this) * f + *this; } // mad(c - *this, f, *this)
	col8us mad(col8us const& f, col8us const& c) const { return *this * f + c; }
	col8us mad(col8us const& c) const { return _mm_packs_epi32(_mm_srli_epi16(_mm_madd_epi16(imm, c.imm), 8), _mm_setzero_si128()); }
	col8us mux(col8us const& a, col8us const& b) const { return this->and(a).or(this->nand(b)); }
	col8us clamp() const { return clamp(imm); }

	col8us abs() const { return _mm_abs_epi16(imm); }
	col8us xor(col8us const& c) const { return _mm_xor_si128(imm, c.imm); }
	col8us or(col8us const& c) const { return _mm_or_si128(imm, c.imm); }
	col8us and(col8us const& c) const { return _mm_and_si128(imm, c.imm); }
	col8us nand(col8us const& c) const { return _mm_andnot_si128(imm, c.imm); }

	col8us operator & (col8us const& c) const { return and(c); }
	col8us operator | (col8us const& c) const { return or(c); }
	col8us operator ^ (col8us const& c) const { return xor(c); }

	i16 a() const { return get<3>(); }
	i16 r() const { return get<2>(); }
	i16 g() const { return get<1>(); }
	i16 b() const { return get<0>(); }
};

col8us const col8us::cmask = _mm_set_epi16(  0, 255, 255, 255,   0, 255, 255, 255);
col8us const col8us::bmask = _mm_set_epi16(  0,   0,   0, 255,   0,   0,   0, 255);
col8us const col8us::gmask = _mm_set_epi16(  0,   0, 255,   0,   0,   0, 255,   0);
col8us const col8us::rmask = _mm_set_epi16(  0, 255,   0,   0,   0, 255,   0,   0);
col8us const col8us::amask = _mm_set_epi16(255,   0,   0,   0, 255,   0,   0,   0);
col8us const col8us::one   = _mm_set_epi16(255, 255, 255, 255, 255, 255, 255, 255);
col8us const col8us::zero  = _mm_setzero_si128();

typedef col8us c8us;
typedef col8us c8;

std::ostream& operator << (std::ostream& os, col8us const& c) {
    return os
        << "c0(" << c.r0 << "," << c.g0 << "," << c.b0 << "," << c.a0 << "); "
        << "c1(" << c.r1 << "," << c.g1 << "," << c.b1 << "," << c.a1 << ")";
}
