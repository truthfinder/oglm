#ifndef __simd_h__
#define __simd_h__

#define __GX_SSE__			0x10
#define __GX_SSE2__			0x20
#define __GX_SSE3__			0x30
#define __GX_SSSE3__		0x31
#define __GX_SSE4_1__		0x41
#define __GX_SSE4_2__		0x42
#define __GX_AVX__			0xA1
#define __GX_AVX2__			0xA2
#define __GX_FMA__			0xF0

#define __GX_MAX_SIMD__		__GX_SSE3__

// s1(sse), s2(sse2), s3(sse3/ssse3), s4(sse4.1/2), a1(avx1), a2(avx2), fm(fma)

/*
#if defined __AVX__ || defined __AVX2__ || defined __FMA__
#include <immintrin.h> // AVX, AVX2, FMA, includes smmintrin.h for AES and PCLMUL
#elif defined __AES__ || defined __PCLMUL__
#include <wmmintrin.h>
#elif defined __SSE4_2__
#include <nmmintrin.h>
#elif defined __SSE4_1__
#include <smmintrin.h>
#elif defined __SSSE3__
#include <tmmintrin.h>
#elif defined __SSE3__
#include <pmmintrin.h>
#elif defined __SSE2__
#include <emmintrin.h>
#elif defined __SSE__
#include <xmmintrin.h>
#endif
//*/

#define _mm_swap_ps(v) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), _MM_SHUFFLE(0, 1, 2, 3)))
#define _mm_load_epi32(v) _mm_castps_si128(_mm_set_ss((float&)(ptr)))
#define _mm_shufd(v, i) _mm_shuffle_ps((v), (v), _MM_SHUFFLE((i), (i), (i), (i)))

#define _fm_madd_ps(a, b, c) _mm_fmadd_ps((a), (b), (c))
#define _fm_msub_ps(a, b, c) _mm_fmsub_ps((a), (b), (c))
#define _s1_madd_ps(a, b, c) _mm_add_ps(_mm_mul_ps((a), (b)), (c))
#define _s1_msub_ps(a, b, c) _mm_sub_ps(_mm_mul_ps((a), (b)), (c))

//_mm_splat_ps(v, i) _mm_shuffle_ps((v), (v), _MM_SHUFFLE((i),(i),(i),(i)))
//_mm_mux_ps(m, u, v) _mm_or_ps(_mm_and_ps((m), (u)), _mm_andnot_ps((m), (v))) //b&mask|b&~mask
//_mm_mux_epi32(m, u, v) _mm_or_si128(_mm_and_si128((m), (u)), _mm_andnot_si128((m), (v)))
//_mm_neg_ps(v) _mm_xor_ps((v), _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));

// Alternative method by Jim Conyngham/Wikipedia MD5 page, via
// http://markplusplus.wordpress.com/2007/03/14/fast-sse-select-operation/ [July 2012]
//#define _mm_mux_ps(m, a, b) _mm_xor_ps(a, _mm_and_ps(m, _mm_xor_ps(b, a))) // ((b^a)&mask)^a

#ifdef __AVX2__
#	define _mm_madd_ps(a, b, c) _fm_madd_ps((a), (b), (c))
#	define _mm_msub_ps(a, b, c) _fm_msub_ps((a), (b), (c))
#else
#	define _mm_madd_ps(a, b, c) _s1_madd_ps((a), (b), (c))
#	define _mm_msub_ps(a, b, c) _s1_msub_ps((a), (b), (c))
#endif

//_mm_movemask_ps(_mm_cmple_ps(x, edge)) == 0
//_sqrt,_rsqrt,_rcp,_sqrt_nr,_rsqrt_nr,_rcp_nr

template<int a, int b, int c, int d> [[nodiscard]] __m128 __vectorcall _mm_shuf_ps(__m128 const& u, __m128 const& v)
{ return _mm_shuffle_ps(u, v, _MM_SHUFFLE(a, b, c, d)); }
template<int a, int b, int c, int d> [[nodiscard]] __m128 __vectorcall _mm_shuf_ps(__m128 const& v)
{ return _mm_shuffle_ps(v, v, _MM_SHUFFLE(a, b, c, d)); }
template<int a, int b, int c, int d> [[nodiscard]] __m128 __vectorcall _mm_shufd_ps(__m128 const& v)
{ return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), _MM_SHUFFLE(a, b, c, d))); }
template<int a, int b, int c, int d> [[nodiscard]] __m128i __vectorcall _mm_shufd_epi32(__m128i const& v)
{ return _mm_shuffle_epi32(v, _MM_SHUFFLE(a, b, c, d)); }
template<int i> [[nodiscard]] __m128 __vectorcall _mm_shuf_ps(__m128 const& u, __m128 const& v)
{ return _mm_shuffle_ps(u, v, _MM_SHUFFLE(i, i, i, i)); }
template<int i> [[nodiscard]] __m128 __vectorcall _mm_shuf_ps(__m128 const& v)
{ return _mm_shuffle_ps(v, v, _MM_SHUFFLE(i, i, i, i)); }
template<int i> [[nodiscard]] __m128 __vectorcall _mm_shufd_ps(__m128 const& v)
{ return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v), _MM_SHUFFLE(i, i, i, i))); }
template<int i> [[nodiscard]] __m128i __vectorcall _mm_shufd_epi32(__m128i const& v)
{ return _mm_shuffle_epi32(v, _MM_SHUFFLE(i, i, i, i)); }

#ifdef __AVX__
template<int i> float __vectorcall _sa_get_ps(__m128 v)
{ return *(float const* const)&_mm_extract_ps(v, i); }
template<> float __vectorcall _sa_get_ps<0>(__m128 v)
{ return _mm_cvtss_f32(v); }
template<int i> int __vectorcall _sa_get_epi32(__m128i v)
{ return _mm_extract_epi32(v, i); }
template <> int __vectorcall _sa_get_epi32<0>(__m128i x)
{ return _mm_cvtsi128_si32(x); }
#else
template<int i> [[nodiscard]] float __vectorcall _mm_get_ps(__m128 v)
{ return _mm_cvtss_f32(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(x), i * 4))); }
template<> [[nodiscard]] float __vectorcall _mm_get_ps<0>(__m128 v)
{ return _mm_cvtss_f32(v); }
template <int i> [[nodiscard]] int __vectorcall _s2_get_epi32(__m128i x)
{ return _mm_cvtsi128_si32(_mm_srli_si128(x, i * 4)); }
template <> [[nodiscard]] int __vectorcall _s2_get_epi32<0>(__m128i x)
{ return _mm_cvtsi128_si32(x); }
#endif

#if __AVX__
__m128 __vectorcall _mx_cmpeq_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmpeq_ps(a, b)); }
__m128 __vectorcall _mx_cmpne_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmpneq_ps(a, b)); }
__m128 __vectorcall _mx_cmplt_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmplt_ps(a, b)); }
__m128 __vectorcall _mx_cmple_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmple_ps(a, b)); }
__m128 __vectorcall _mx_cmpgt_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmpgt_ps(a, b)); }
__m128 __vectorcall _mx_cmpge_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_blendv_ps(y, x, _mm_cmpge_ps(a, b)); }
__m128 __vectorcall _mx_cmpeq_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmpeq_ps(a, b)); }
__m128 __vectorcall _mx_cmpne_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmpneq_ps(a, b)); }
__m128 __vectorcall _mx_cmplt_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmplt_ps(a, b)); }
__m128 __vectorcall _mx_cmple_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmple_ps(a, b)); }
__m128 __vectorcall _mx_cmpgt_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmpgt_ps(a, b)); }
__m128 __vectorcall _mx_cmpge_ps(__m128 const& a, __m128 const& b)
{ return _mm_blendv_ps(b, a, _mm_cmpge_ps(a, b)); }
#else
[[nodiscard]] __m128 __vectorcall _mx_cmpeq_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpeq_ps(a, b), x), _mm_andnot_ps(_mm_cmpeq_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpne_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpneq_ps(a, b), x), _mm_andnot_ps(_mm_cmpneq_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmplt_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmplt_ps(a, b), x), _mm_andnot_ps(_mm_cmplt_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmple_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmple_ps(a, b), x), _mm_andnot_ps(_mm_cmple_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpgt_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpgt_ps(a, b), x), _mm_andnot_ps(_mm_cmpgt_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpge_ps(__m128 const& a, __m128 const& b, __m128 const& x, __m128 const& y)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpge_ps(a, b), x), _mm_andnot_ps(_mm_cmpge_ps(a, b), y)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpeq_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpeq_ps(a, b), a), _mm_andnot_ps(_mm_cmpeq_ps(a, b), b)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpne_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpneq_ps(a, b), a), _mm_andnot_ps(_mm_cmpneq_ps(a, b), b)); }
[[nodiscard]] __m128 __vectorcall _mx_cmplt_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmplt_ps(a, b), a), _mm_andnot_ps(_mm_cmplt_ps(a, b), b)); }
[[nodiscard]] __m128 __vectorcall _mx_cmple_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmple_ps(a, b), a), _mm_andnot_ps(_mm_cmple_ps(a, b), b)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpgt_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpgt_ps(a, b), a), _mm_andnot_ps(_mm_cmpgt_ps(a, b), b)); }
[[nodiscard]] __m128 __vectorcall _mx_cmpge_ps(__m128 const& a, __m128 const& b)
{ return _mm_or_ps(_mm_and_ps(_mm_cmpge_ps(a, b), a), _mm_andnot_ps(_mm_cmpge_ps(a, b), b)); }
#endif

[[nodiscard]] __m128i __vectorcall _mx_cmpeq_epi32(__m128i const& a, __m128i const& b, __m128i const& x, __m128i const& y)
{ return _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi32(a, b), x), _mm_andnot_si128(_mm_cmpeq_epi32(a, b), y)); }
[[nodiscard]] __m128i __vectorcall _mx_cmplt_epi32(__m128i const& a, __m128i const& b, __m128i const& x, __m128i const& y)
{ return _mm_or_si128(_mm_and_si128(_mm_cmplt_epi32(a, b), x), _mm_andnot_si128(_mm_cmplt_epi32(a, b), y)); }
[[nodiscard]] __m128i __vectorcall _mx_cmpgt_epi32(__m128i const& a, __m128i const& b, __m128i const& x, __m128i const& y)
{ return _mm_or_si128(_mm_and_si128(_mm_cmpgt_epi32(a, b), x), _mm_andnot_si128(_mm_cmpgt_epi32(a, b), y)); }
[[nodiscard]] __m128i __vectorcall _mx_cmpeq_epi32(__m128i const& a, __m128i const& b)
{ return _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi32(a, b), a), _mm_andnot_si128(_mm_cmpeq_epi32(a, b), b)); }
[[nodiscard]] __m128i __vectorcall _mx_cmplt_epi32(__m128i const& a, __m128i const& b)
{ return _mm_or_si128(_mm_and_si128(_mm_cmplt_epi32(a, b), a), _mm_andnot_si128(_mm_cmplt_epi32(a, b), b)); }
[[nodiscard]] __m128i __vectorcall _mx_cmpgt_epi32(__m128i const& a, __m128i const& b)
{ return _mm_or_si128(_mm_and_si128(_mm_cmpgt_epi32(a, b), a), _mm_andnot_si128(_mm_cmpgt_epi32(a, b), b)); }

[[nodiscard]] __m128 __vectorcall _s1_abs_ps(__m128 x) { return _mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)), x); }
[[nodiscard]] __m128 __vectorcall _s1_neg_ps(__m128 x) { return _mm_xor_ps(x, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))); }

[[nodiscard]] __m128 __vectorcall _s1_round_ps(__m128 x) { // even?, check for pow 23
	__m128 a = _mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), x);
	__m128 b = _mm_or_ps(a, _mm_set_ps1(8388608.f)); // two23 = 0x4B000000
	return _mm_sub_ps(_mm_add_ps(x, b), b);
}

[[nodiscard]] __m128 __vectorcall _s1_signum_ps(__m128 x) {
	__m128 a = _mm_and_ps(_mm_cmplt_ps(x, _mm_setzero_ps()), _mm_set1_ps(-1.f));
	return _mm_or_ps(a, _mm_and_ps(_mm_cmpgt_ps(x, _mm_setzero_ps()), _mm_set1_ps(1.f)));
}

[[nodiscard]] __m128 __vectorcall _s1_floor_ps_(__m128 x) {
	__m128 rnd = _s1_round_ps(x);
	return _mm_sub_ps(rnd, _mm_and_ps(_mm_cmplt_ps(x, rnd), _mm_set1_ps(1.f)));
}

[[nodiscard]] __m128 __vectorcall _s1_floor_ps(__m128 x) { // float(int(x) - (x < int(x)))
	//auto ceil = [](float x) -> float { return float(int(x) - (x > int(x))); };
	//auto ceil = [](float x) -> float { return -floor(-x); };
	__m128i w = _mm_cvttps_epi32(x);
	return _mm_cvtepi32_ps(_mm_add_epi32(w, _mm_castps_si128(_mm_cmplt_ps(x, _mm_cvtepi32_ps(w)))));
}
/*
[[nodiscard]] __m128 __vectorcall _s1_floor_ps(__m128 x) { // float(int(x) - (x < int(x)))
	__m128 roundtrip = _mm_cvtepi32_ps(_mm_cvttps_epi32(fVec));
	__m128 too_big = _mm_cmpgt_ps(roundtrip, fVec);
	return _mm_sub_ps(roundtrip, _mm_and_ps(too_big, _mm_set1_ps(1.0f)));
}
*/
[[nodiscard]] __m128 __vectorcall _s1_ceil_ps_(__m128 x) { // float(int(x) - (x > int(x))); -floor(-x)
	__m128i w = _mm_cvttps_epi32(x);
	return _mm_cvtepi32_ps(_mm_sub_epi32(w, _mm_castps_si128(_mm_cmpgt_ps(x, _mm_cvtepi32_ps(w)))));
}

[[nodiscard]] __m128 __vectorcall _s2_trunc_ps(__m128 x) {
	return _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
}

[[nodiscard]] __m128 __vectorcall _s1_ceil_ps(__m128 x) {
	__m128 rnd = _s1_round_ps(x);
	return _mm_add_ps(rnd, _mm_and_ps(_mm_cmpgt_ps(x, rnd), _mm_set1_ps(1.f)));
}

[[nodiscard]] __m128 __vectorcall _s1_frac_ps(__m128 x) {
	return _mm_sub_ps(x, _s1_floor_ps(x));
}

[[nodiscard]] __m128 __vectorcall _s1_mod_ps(__m128 x, __m128 y) {
	return _mm_sub_ps(x, _mm_mul_ps(y, _s1_floor_ps(_mm_div_ps(x, y))));
}

/*
[[nodiscard]] __m128 __vectorcall _s1_floor_ps(__m128 x) {
	__m128 one = _mm_castsi128_ps(_mm_set1_epi32(0x3F800000));
	__m128 two23 = _mm_castsi128_ps(_mm_set1_epi32(0x4B000000));
	__m128 f = _mm_sub_ps(_mm_add_ps(f, two23), two23);
	f = _mm_add_ps(_mm_sub_ps(x, two23), two23);
	f = _mm_sub_ps(f, _mm_and_ps(one, _mm_cmplt_ps(x, f)));
	//return f;
	__m128 msk = _mm_cmplt_ps(two23, _mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)), x));
	return _mm_or_ps(_mm_and_ps(msk, x), _mm_andnot_ps(msk, f));
}

[[nodiscard]] __m128 __vectorcall _s1_ceil_ps(__m128 x) {
	__m128 one = _mm_castsi128_ps(_mm_set1_epi32(0x3F800000));
	__m128 two23 = _mm_castsi128_ps(_mm_set1_epi32(0x4B000000));
	__m128 f = _mm_sub_ps(_mm_add_ps(f, two23), two23);
	f = _mm_add_ps(_mm_sub_ps(x, two23), two23);
	f = _mm_add_ps(f, _mm_and_ps(one, _mm_cmplt_ps(f, x)));
	//return f;
	__m128 msk = _mm_cmplt_ps(two23, _mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)), x));
	return _mm_or_ps(_mm_and_ps(msk, x), _mm_andnot_ps(msk, f));
}
//*/

//template<int i> float __vectorcall _s4_get_ps(__m128 const& v) { return *(float const* const)&_mm_extract_ps(v, i); }
//double ceil(double x) { if(x<0)return (int)x; return ((int)x)+1; }
//double floor(double x) { if(x>0)return (int)x; return (int)(x-0.9999999999999999); }
//double round(double arg) { return (int)(arg+0.5); }
//int trunc(double arg) { return (int)arg; }
//double fmod(double a,double b) { return (int)((((a/b)-((int)(a/b)))*b)+0.5); }

[[nodiscard]] __m128 __vectorcall _s1_mix_ps(__m128 x, __m128 y, __m128 a)
{ return _mm_madd_ps(_mm_sub_ps(y, x), a, x); }

[[nodiscard]] __m128 __vectorcall _fm_mix_ps(__m128 x, __m128 y, __m128 a)
{ return _mm_madd_ps(_mm_sub_ps(y, x), a, x); }

[[nodiscard]] __m128 __vectorcall __s1_cross_ps(__m128 u, __m128 v) // U.yzx * V.zxy - U.zxy * V.yzx
{ return _mm_sub_ps(_mm_mul_ps(_mm_shuf_ps<2,1,3,0>(u), _mm_shuf_ps<1,3,2,0>(v)), _mm_mul_ps(_mm_shuf_ps<1,3,2,0>(v), _mm_shuf_ps<2,1,3,0>(u))); }
[[nodiscard]] __m128 __vectorcall _s1_cross_ps(__m128 u, __m128 v) // (U * V.yzx - U.yzx * V).yzx
{ return _mm_shuf_ps<2,1,3,0>(_mm_sub_ps(_mm_mul_ps(u, _mm_shuf_ps<2,1,3,0>(v)), _mm_mul_ps(_mm_shuf_ps<2,1,3,0>(u), v))); }

#define _mm_frac_ps _s1_frac_ps

#if __GX_MAX_SIMD__ >= __GX_AVX__
#	define _s4_floor_ps _mm_floor_ps
#	define _mx_floor_ps _s4_floor_ps
#else
#	define _mx_floor_ps _s1_floor_ps
#endif
//*/
//fmod(a,b) returns a-b*floor(a/b) // nvidia
#define _s1_extract_ps(v, i) _mm_cvtss_f32(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(v), 4*(i))))
#define _s1_extract_epi32(v, i) _mm_cvtsi128_si32(_mm_srli_si128((v), 4*(i)))
//float _s4_extract_ps(__m128 v, int i) { int z = _mm_extract_ps(v, i); return *(float*)&z; }
#define _s4_extract_ps(v, i) ((float const&)_mm_extract_ps((v), (i)))
#define _s4_extract_epi32(v, i) (_mm_extract_epi32((v), (i)))
#define _mx_setone_ps() _mm_set1_ps(1.f)
#define _mx_min3_ps(a, b, c) _mm_min_ps(_mm_min_ps((a), (b)), (c))
#define _mx_max3_ps(a, b, c) _mm_max_ps(_mm_max_ps((a), (b)), (c))
//#define _mx_mix_ps(a, b, t) _mm_add_ps(_mm_mul_ps((a), _mm_sub_ps(_mx_setone_ps(), (t))), _mm_mul_ps((b), (t)))
#define _mx_mix_ps(a, b, t) _mm_madd_ps(_mm_sub_ps((b), (a)), (t), (a))
#define _mx_clamp_ps(x, a, b) _mm_min_ps(_mm_max_ps((x), (a)), (b))
#define _mx_wrap_ps _s1_frac_ps
#define _mx_fmod_ps(a, b) _mm_sub_ps((a), _mm_mul_ps((b), _mx_floor_ps(_mm_div_ps((a), (b)))))
//#define _mx_dp3_ps _s1_dp3/_s4_dp3
//#define _mx_dp4_ps _s1_dp4/_s4_dp4

#define _mx_clamp_epi16(c) _mm_min_epi16(_mm_max_epi16((c), _mm_setzero_si128()), _mm_set1_epi16(255))
#define _mx_cvtsi32_si128(i) _mm_castps_si128(_mm_set_ss((float&)(i)))
#define _mx_madd_epi16(a, b) _mm_packs_epi32(_mm_srli_epi32(_mm_madd_epi16((a), (b)), 8), _mm_setzero_si128())
#define _mx_epi16_i32(r16) _mm_cvtsi128_si32(_mm_packus_epi16((r16), _mm_setzero_si128()))
#define _mx_i32_epi16(i) _mm_unpacklo_epi8(_mx_cvtsi32_si128(i), _mm_setzero_si128())
#define _mx_mix_epi16(x, y, f) _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(_mx_clamp_epi16(_mm_sub_epi32((y), (x))), (f)), 8), (x))

[[nodiscard]] inline __m128i _s2_mullo_epi32(__m128i a, __m128i b) {
	__m128i p02 = _mm_mul_epu32(a, b), p13 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
	return _mm_unpacklo_epi32(_mm_shuffle_epi32(p02, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(p13, _MM_SHUFFLE(0, 0, 2, 0)));
}

//#define _s4_mullo_epi32 _mm_mullo_epi32

//_mx_rcp11_ps
//_mx_rcp22_ps // newton raphson: 2 * rcpss(x) - (x * rcpss(x) * rcpss(x))
//rsqrt_ps_nr // newton raphson: 0.5 * rsqrtss * (3 - x * rsqrtss(x) * rsqrtss(x))
//_mx_fmod11_ps(a, b) _mm_sub_ps(a, _mm_mul_ps(b, _mx_floor_ps(_mm_mul_ps(a, _mm_rcp11_ps(b)))))

[[nodiscard]] inline float qlog2f(float val)
{ // approximation, the maximum error is below 0.007
  // http://www.flipcode.com/archives/Fast_log_Function.shtml
  // The proposed formula is a 3rd degree polynomial keeping first derivate continuity.
  // Higher degree could be used for more accuracy.For faster results, one can remove this line,
  // if accuracy is not the matter(it gives some linear interpolation between powers of 2)
  const int lg2 = (((int&)val >> 23) & 255) - 128; // floor(log2(N))
  (int&)val = ((int&)val & ~(255 << 23)) + (127 << 23);
  return lg2 + ((-1.f / 3.f) * val + 2.f) * val - (2.f / 3.f); // computes 1+log2(m), m ranging from 1 to 2
}

[[nodiscard]] inline __m128 qlog2fv(__m128 v)
{
	__m128 lg2 = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_and_si128(_mm_srai_epi32(_mm_castps_si128(v), 23), _mm_set1_epi32(255)), _mm_set1_epi32(128)));
	v = _mm_castsi128_ps(_mm_add_epi32(_mm_and_si128(_mm_castps_si128(v), _mm_set1_epi32(~(255 << 23))), _mm_set1_epi32(127 << 23)));
	return _mm_add_ps(lg2, _mm_sub_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(-1.f / 3.f), v), _mm_set1_ps(2.f)), v), _mm_set1_ps(2.f / 3.f)));
}

/*
float qlog2f(float val) {
	//MSVC + GCC compatible version that give XX.XXXXXXX +-0.0054545
	union { float val; int32_t x; } u = { val };
	register float log_2 = (float)(((u.x >> 23) & 255) - 128);
	u.x   &= ~(255 << 23);
	u.x   += 127 << 23;
	log_2 += ((-0.3358287811f) * u.val + 2.0f) * u.val  -0.65871759316667f;
	return log_2;
	// Slightly more accurate formula (maximum error ±0.00493976), optimized using Remez' algorithm:
	// ((-0.34484843f) * u.val + 2.02466578f) * u.val - 0.67487759f
}
float qlog(const float &val) { return qlog2(val) * .69314718f; } // *ln(2)
__m128 qlogv(__m128 v) { return _mm_mul_ps(qlog2v(v), _mm_set1_ps(.69314718f)); }
#define DOUBLE2INT(i, d) {double t = ((d) + 6755399441055744.0); i = *((int *)(&t));}
//*/
/*
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
//*/
//_may_i_use_cpu_feature(_FEATURE_SSE|_FEATURE_SSE2)
#endif // __simd_h__
