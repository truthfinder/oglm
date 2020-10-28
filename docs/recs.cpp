void spans_original(size_t id, size_t count, BaryCtx& ctx) {
	//_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	__m128 sy4 = _mm_set1_ps(ctx.sy);
	v4 yv = _mm_madd_ps(sy4, ctx.JV, ctx.FV);
	c4 yc = _mm_madd_ps(sy4, ctx.JC, ctx.FC);
	t4 yt = _mm_madd_ps(sy4, ctx.JT, ctx.FT);

	BlendTools::EnvFmtProc8 envFmtProc08 = ctx.tex0 && ctx.tex0->layer(0) ? BlendTools::envFmtTable8[ctx.texEnvMode[0]][ctx.tex0->layer(0)->intFmt_] : nullptr;
	BlendTools::EnvFmtProc8 envFmtProc18 = ctx.tex1 && ctx.tex1->layer(0) ? BlendTools::envFmtTable8[ctx.texEnvMode[1]][ctx.tex1->layer(0)->intFmt_] : nullptr;
	BlendTools::BlendProc8 srcBlendProc8 = BlendTools::blendProcTable8[ctx.sfactor];
	BlendTools::BlendProc8 dstBlendProc8 = BlendTools::blendProcTable8[ctx.dfactor];

	size_t divisor = ctx.height / count;
	for (i32 szy = ry - y; szy--; cptr += ctx.width
#		ifdef __WBUF__
		, wptr += ctx.width
#		else
		, zptr += ctx.width
#		endif
		, y++, ctx.l++, ctx.r++, yv += ctx.JV, yc += ctx.JC, yt += ctx.JT) {
		//if (y % count != id) continue;
		if (y / divisor != id) continue; // use this, check `y` regions with sy/ey
		//if (id != 1) continue;

		f32 sx = f32(i32(ctx.l.x + f0_49)) + .5f;
		i32 x = i32(sx), rx = i32(ctx.r.x + f0_49);
		if (x >= rx) continue;
		i32* cp = cptr + x;
#		ifdef __WBUF__
		f32* wp = wptr + x;
#		else
		f32* zp = zptr + x;
#		endif

		__m128 const sx4 = _mm_set1_ps(sx);
		v4 xv = _mm_madd_ps(sx4, ctx.IV, yv);
		c4 xc = _mm_madd_ps(sx4, ctx.IC, yc);
		t4 xt = _mm_madd_ps(sx4, ctx.IT, yt);

		for (i32 szx = rx - x; szx--; cp++
#			ifdef __WBUF__
			, wp++
#			else
			, zp++
#			endif
			, x++, xv += ctx.IV, xc += ctx.IC, xt += ctx.IT)
		{
#			ifdef __WBUF__
			if (ctx.bDepthTest && !ctx.depthOpW(1.f/xv.z, *wp)) continue;
#			else
			if (ctx.bDepthTest && !ctx.depthOpZ(xv.z, *zp)) continue;
#			endif

			c8 c;
			if (!ctx.tex0) {
				c = xc;
			} else {
				t4 xtw = xt * xv.wwww().rcp11(); // t4 xtw = xt * (1.f / xv.w);
				f32 lambda0 = 0.f, lambda1 = 0.f; // TODO: process tiles 2x2 where lmd0|1 are identical

				if ((ctx.tex0 && ctx.tex0->maxLevel_) || (ctx.tex1 && ctx.tex1->maxLevel_)) {
					t4 const xtwx = (xt + ctx.IT) / (xv + ctx.IV).w, xtwy = (xt + ctx.JT) / (xv + ctx.JV).w;
					t4 dFdx = (xtwx - xtw) * ctx.wh0wh1, dFdy = (xtwy - xtw) * ctx.wh0wh1;
					dFdx = _mm_mul_ps(dFdx, dFdx); // (u0_10-u0_00)^2, (v0_10-v0_00)^2, (u1_10-u1_00)^2, (v1_10-v1_00)^2
					dFdy = _mm_mul_ps(dFdy, dFdy); // (u0_01-u0_00)^2, (v0_01-v0_00)^2, (u1_01-u1_00)^2, (v1_01-v1_00)^2
					__m128 const temp = _mm_hadd_ps(dFdx, dFdy); //(u0_01-u0_00)^2+(v0_01-v0_00)^2, (u1_01-u1_00)^2+(v1_01-v1_00)^2, (u0_10-u0_00)^2+(v0_10-v0_00)^2, (u1_10-u1_00)^2+(v1_10-v1_00)^2
					__m128 rho2 = _mm_max_ps(temp, _mm_shuf_ps<1, 0, 3, 2>(temp)); //rho2_t0, rho2_t1, rho2_t0, rho2_t1
					rho2 = _mm_mul_ps(qlog2fv(rho2), _mm_set1_ps(.5f)); // qlog2fv <- critical point 1/2
					lambda0 = _s1_extract_ps(rho2, 1); // todo: v4f(rho).get<1>()
					lambda1 = ctx.tex1 && ctx.tex1->maxLevel_ ? _mm_cvtss_f32(rho2) : 0.f; // todo: v4f(rho).get<0>()
				}

				xtw -= _s1_floor_ps_(xtw);

				if (!ctx.tex1) {
					c8 const t = ctx.tex0->mapTex(xtw, lambda0); // fixme: not mag only
					c = envFmtProc08(xc, t, ctx.texEnvColor[0]);
				} else {
					c8 const t0 = ctx.tex0->mapTex(xtw, lambda0);
					c8 const t1 = ctx.tex1->mapTex(_mm_castsi128_ps(_mm_srli_si128(xtw.imm, 8)), lambda1);
					c = envFmtProc18(envFmtProc08(xc, t0, ctx.texEnvColor[0]), t1, ctx.texEnvColor[1]);
				}
			}

			if (ctx.bAlphaTest && !ctx.alphaOpI(c.a(), ctx.alphaRefI)) continue;
			if (ctx.bDepthMask) {
#				ifdef __WBUF__
				*wp = 1.f/xv.z;
#				else
				*zp = xv.z;
#				endif
			}

			if (ctx.bBlend) {
				c8 const d = _mx_i32_epi16(*cp);
				c = _mx_madd_epi16(_mm_unpacklo_epi16(c, d), _mm_unpacklo_epi16(srcBlendProc8(c, d), dstBlendProc8(c, d)));
			}

			*cp = c;
		}
	}
	//_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
};

void spans(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	for (i32 qy = y; qy < ry; qy++, cptr += ctx.width, zptr += ctx.width) {
		f32 sx = f32(i32(std::min(ctx.l.s.v.x, ctx.l.e.v.x) + f0_49)) + .5f;
		i32 x = i32(sx), rx = i32(std::max(ctx.r.s.v.x, ctx.r.e.v.x) + f0_49);

		i32* cp = cptr + x;
#		ifdef __WBUF__
		f32* wp = wptr + x;
#		else
		f32* zp = zptr + x;
#		endif

		for (i32 qx = x; qx < rx; qx++, cp++, zp++) {
			v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; //Z/W mapping; X/Y-W?

			auto ls = ctx.l.s.v;
			auto le = ctx.l.e.v;
			auto rs = ctx.r.s.v;
			auto re = ctx.r.e.v;

			auto const& lc = ((le - ls) ^ (v - ls)).z;
			auto const& rc = ((v - rs) ^ (re - rs)).z;

			if (lc <= 0.f && rc < 0.f)
				*cp = ctx.l.s.c;
		}
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spansf(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	//_MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
	//_MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
	//f32 q = 701.5f + f0_49;
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	//f32 sx = f32(i32(std::min(ctx.l.s.v.x, ctx.l.e.v.x) + f0_49)) + .5f;
	f32 sx = ctx.sx, ex = ctx.ex;
	i32 x = i32(sx), rx = i32(ex); // rx = i32(std::max(ctx.r.s.v.x, ctx.r.e.v.x) + f0_49);

	v4 ls = ctx.l.s.v, le = ctx.l.e.v;
	v4 rs = ctx.r.s.v, re = ctx.r.e.v;
	v4 vt(0.f, 0.f, ctx.sy, ctx.sx);

	f32 w0i0 = ((le - ls) ^ (vt - ls)).z;
	f32 w1i0 = ((vt - rs) ^ (re - rs)).z;
	f32 w0dx = ls.y - le.y, w0dy = le.x - ls.x;
	f32 w1dx = re.y - rs.y, w1dy = rs.x - re.x;

	/*
	int wh = ctx.isy & ~MASK;
	int fr = ctx.isy & MASK;
	assert(ctx.iey >= wh);

	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr+=ctx.width) {
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		i32* cp = cptr + (x1 >> OFST);
		for (int x = x1; x < ctx.ir.x; x += _1_0, cp++) {
			//int rx = x >> OFST, ry = y >> OFST;
			//int ofs = ry * ctx.width + rx;
			//*(ctx.cbuf + ofs) = ctx.il.s.c;
			*cp = ctx.il.s.c;
		}
	}//*/
	//*
	//static LineIntervals lines;
	//lines.update_rect("rect.txt");
	f32 w0i = w0i0, w1i = w1i0;
	for (i32 qy = y, cy = 1; qy < ry; qy++, cptr += ctx.width, zptr += ctx.width, cy++) {
		i32* cp = cptr + x;
#		ifdef __WBUF__
		f32* wp = wptr + x;
#		else
		f32* zp = zptr + x;
#		endif

		f32 w0 = w0i;
		f32 w1 = w1i;

		//if (lines.check_y(qy)) {
		for (i32 qx = x, cx = 0; qx < rx; qx++, cp++, zp++, cx++) {
			//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; //Z/W mapping; X/Y-W?

			//f32 lc = ((le - ls) ^ (v - ls)).z;
			//f32 rc = ((v - rs) ^ (re - rs)).z;

			w0 = w0i + w0dx * cx;
			w1 = w1i + w1dx * cx;

			//if (lines.check_x(qx)) {
				//if (lc <= 0.f && rc < 0.f)
			if (w0 <= 0.f && w1 < 0.f)
				*cp = ctx.l.s.c;
			//}

			//w0 += w0dx;
			//w1 += w1dx;
		}
		//}
		//w0i += w0dy;
		//w1i += w1dy;
		w0i = w0i0 + w0dy * cy;
		w1i = w1i0 + w1dy * cy;
	}//*/
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spansi1(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr+=ctx.width) {
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		int ix = x1 >> OFST;
		if (x1 >= ctx.ir.x) continue;

		//int byte_ofs = ix & ~(16 - 1);
		//int byte_rst = ix & (16 - 1);
		//int byte_cnt = (((ctx.ir.x - x1 + _1_0) >> OFST) + 4) >> 2;

		//i32* cp = cptr + (x1 >> OFST);
		for (int x = x1; x < ctx.ir.x; x += _1_0, ix++) { // cp++
			//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; //Z/W mapping; X/Y-W?
			//int rx = x >> OFST, ry = y >> OFST;
			//int ofs = ry * ctx.width + rx;
			//*(ctx.cbuf + ofs) = ctx.il.s.c;
			//*cp = ctx.il.s.c;
			*(cptr+ix) = ctx.il.s.c;
		}
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spans128i(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	size_t divisor = ctx.height / count;
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		if ((y >> OFST) / divisor != id) continue; // use this, check `y` regions with sy/ey
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		int ix = x1 >> OFST;
		if (x1 >= ctx.ir.x) continue;

		//* int128
		const int pix = 4;
		int ofs = ix & ~(pix - 1);
		int rst = ix & (pix - 1);
		int cnt = (((ctx.ir.x - x1 + _1_0-1) >> OFST) + rst + (pix-1)) / pix;
		//int Xsz = (ctx.ir.x - x1 + _1_0-1 >> OFST); // CHECK

		__m128i const src4 = _mm_set1_epi32(ctx.il.s.c);
		__m128i const l4 = _mm_set1_epi32(ix - 1), r4 = _mm_set1_epi32(ix + (ctx.ir.x - x1 + _1_0 - 1 >> OFST));
		__m128i const inc4 = _mm_set1_epi32(pix);

		__m128i ofs4 = _mm_set_epi32(ofs + 3, ofs + 2, ofs + 1, ofs);
		__m128i lm4 = _mm_cmpgt_epi32(ofs4, l4), rm4 = _mm_cmplt_epi32(ofs4, r4);
		__m128i lrm4 = _mm_and_si128(lm4, rm4);

		for (; cnt--; ofs+=pix, ofs4=_mm_add_epi32(ofs4, inc4)) {
			__m128i dst4 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr + ofs));
			//__m128i dst4 = *reinterpret_cast<__m128i const*>(cptr + ofs);
			__m128i lrm4 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l4), _mm_cmplt_epi32(ofs4, r4));
			__m128i res4 = _mm_or_si128(_mm_andnot_si128(lrm4, dst4), _mm_and_si128(lrm4, src4));
			//*reinterpret_cast<__m128i*>(cptr + ofs) = res4;
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr + ofs), res4);
			//_mm_stream_si128(reinterpret_cast<__m128i*>(cptr + ofs), res4);
		}//*/
		//for (int x = x1; x < ctx.ir.x; x += _1_0, ix++) { *(cptr + ix) =  0xFF00FF00; }

		/*
		for (int x = x1; x < ctx.ir.x; x += _1_0, ix++) {
			*(cptr + ix) = ctx.il.s.c; // 0xFF00FF00;
		}//*/
		/*
		for (int sz = (ctx.ir.x - x1 + (_1_0-1) >> OFST); sz--; ix++) {
			*(cptr + ix) = 0xFF00FF00;//ctx.il.s.c;
		}//*/
		/*
		int cnt1 = 0;
		for (int x = x1; x < ctx.ir.x; x += _1_0, ix++) {
			*(cptr + ix) = ctx.il.s.c; cnt1++;
		}
		int cnt2 = ctx.ir.x - x1 + _1_0 >> OFST;
		//*/
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spans256i(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	size_t divisor = ctx.height / count;
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		if ((y >> OFST) / divisor != id) continue; // use this, check `y` regions with sy/ey
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		int ix = x1 >> OFST;
		if (x1 >= ctx.ir.x) continue;

		const int pix = 8;
		int ofs = ix & ~(pix - 1), rst = ix & (pix - 1);
		int sz = (ctx.ir.x - x1 + _1_0 - 1) >> OFST;
		int cnt = (sz + rst + (pix - 1)) / pix;

		__m256i const src8 = _mm256_set1_epi32(ctx.il.s.c);
		__m256i const l8 = _mm256_set1_epi32(ix - 1), r8 = _mm256_set1_epi32(ix + sz);
		__m256i const inc8 = _mm256_set1_epi32(pix);

		__m256i ofs8 = _mm256_set_epi32(ofs + 7, ofs + 6, ofs + 5, ofs + 4, ofs + 3, ofs + 2, ofs + 1, ofs);
		__m256i lm8 = _mm256_cmpgt_epi32(ofs8, l8), rm8 = _mm256_cmpgt_epi32(r8, ofs8);
		__m256i lrm8 = _mm256_and_si256(lm8, rm8);

		for (; cnt--; ofs += pix, ofs8 = _mm256_add_epi32(ofs8, inc8)) {
			__m256i dst8 = _mm256_load_si256(reinterpret_cast<__m256i const*>(cptr + ofs));
			__m256i lrm8 = _mm256_and_si256(_mm256_cmpgt_epi32(ofs8, l8), _mm256_cmpgt_epi32(r8, ofs8));
			__m256i res8 = _mm256_or_si256(_mm256_andnot_si256(lrm8, dst8), _mm256_and_si256(lrm8, src8));
			_mm256_store_si256(reinterpret_cast<__m256i*>(cptr + ofs), res8);
		}

		_mm256_zeroupper();
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spans_(size_t id, size_t count, BaryCtx& ctx) {
	//TODO: check clear atomic_flag[count] here and clear the rect?
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	const int pix = 4, msk = pix - 1;
	const __m128i _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(pix);

	size_t divisor = ctx.height / count;
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		if ((y>>OFST) / divisor != id) continue; // use this, check `y` regions with sy/ey
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		int ix = x1 >> OFST;
		if (x1 >= ctx.ir.x) continue;

		//* int128
		int ofs = ix & ~msk;
		int rst = ix & msk;
		int szx = (ctx.ir.x - x1 + _1_0 - 1) >> OFST;
		int cnt = (szx + rst + pix - 1) / pix;

		__m128i const src4 = _mm_set1_epi32(ctx.il.s.c);
		__m128i const l4 = _mm_set1_epi32(ix - 1), r4 = _mm_set1_epi32(ix + szx);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3
		__m128i lrm4 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l4), _mm_cmplt_epi32(ofs4, r4));

		for (; cnt--; ofs += pix, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; //Z/W mapping; X/Y-W?
			__m128i dst4 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr + ofs));
			__m128i lrm4 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l4), _mm_cmplt_epi32(ofs4, r4));
			__m128i res4 = _mm_or_si128(_mm_andnot_si128(lrm4, dst4), _mm_and_si128(lrm4, src4));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr + ofs), res4);
		}
		
		//for (int x = x1, iix = ix; x < ctx.ir.x; x += _1_0, *(cptr + iix++) = 0xFF00FF00);
		//for (int sz = (ctx.ir.x - x1 + _1_0 - 1 >> OFST), iix = ix; sz--; *(cptr + iix++) = ctx.il.s.c;);
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spansi2(size_t id, size_t count, BaryCtx& ctx) {
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = (ctx.isy >> OFST), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	const i32 _0_9 = _1_0 - 1;
	i32 szy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 par = szy >> 1;
	i32 sng = szy & 1;
	/*
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		if (x1 >= ctx.ir.x) continue;
		i32* cp = cptr + (x1 >> OFST);
		for (int x = x1; x < ctx.ir.x; x += _1_0, *cp++ = ctx.il.s.c);
	}//*/
	/*
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; szy--; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		if (x1 >= ctx.ir.x) continue;
		i32* cp = cptr + (x1 >> OFST);
		for (i32 szx = (ctx.ir.x - x1 + _1_0 - 1) >> OFST; szx--; *cp++ = ctx.il.s.c);
	}//*/
	//*

	size_t divisor = ctx.height / count;
	//if ((y >> OFST) / divisor != id) continue; // use this, check `y` regions with sy/ey

	cptr = ctx.cbuf + ctx.width * (ctx.isy >> OFST);
	for (; par--; cptr += ctx.width * 2) {
		i32 y0x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5); ++ctx.il;
		i32 y1x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5); ++ctx.il;		

		i32 y0szx = (y0x1 >= ctx.ir.x) ? 0 : (ctx.ir.x - y0x1 + _0_9) >> OFST; ++ctx.ir;
		i32 y1szx = (y1x1 >= ctx.ir.x) ? 0 : (ctx.ir.x - y1x1 + _0_9) >> OFST; ++ctx.ir;

		for (i32* cp = cptr + (y0x1 >> OFST)            ; y0szx--; *cp++ = ctx.il.s.c);
		for (i32* cp = cptr + (y1x1 >> OFST) + ctx.width; y1szx--; *cp++ = ctx.il.s.c);
	}

	if (sng) {
		i32 y0x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5); ++ctx.il;
		i32 y0szx = (y0x1 >= ctx.ir.x) ? 0 : (ctx.ir.x - y0x1 + _0_9) >> OFST; ++ctx.ir;
		for (i32* cp = cptr + (y0x1 >> OFST); y0szx--; *cp++ = ctx.il.s.c);
	}
	//*/

	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spansi3(size_t id, size_t count, BaryCtx& ctx) {
	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST; //assert(tszy > 0);
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1; //assert(iy1 >= ty1);
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2; //assert(iy2 <= ty2);
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2); //assert(iszy > 0 && iszy <= tszy);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);
	//i32 ilx2 = ilx + lk * (iszy - 1);
	//i32 irx2 = irx + rk * (iszy - 1);
	//i32 iw = irx - ilx, iw2 = irx2 - ilx2;

	i32 qys = iszy / 4;
	i32 qyr = iszy & (4 - 1);
	//*
	for (i32 y = iy1; qys--; y += 4, ilx += lk * 4, irx += rk * 4) {
		i32 qy0 = y, qy1 = y + 1, qy2 = y + 2, qy3 = y + 3;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1, lx2 = ilx + lk * 2, lx3 = ilx + lk * 3;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1, rx2 = irx + rk * 2, rx3 = irx + rk * 3;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x2 = (lx2 & ~MASK) + ((lx2 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x3 = (lx3 & ~MASK) + ((lx3 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;
		i32 s2 = (x2 >= rx2) ? 0 : (rx2 - x2 + _0_9) >> OFST;
		i32 s3 = (x3 >= rx3) ? 0 : (rx3 - x3 + _0_9) >> OFST;

		for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy1 * ctx.width + (x1 >> OFST); s1--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy2 * ctx.width + (x2 >> OFST); s2--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy3 * ctx.width + (x3 >> OFST); s3--; *cp++ = ctx.il.s.c);
	}

	for (i32 y = iy1 + (iszy & ~(4 - 1)); qyr--; y++, ilx += lk, irx += rk) {
		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);
	}//*/

	/* ref
	for (i32 y = iy1, szy=iszy; szy--; y++, ilx += lk, irx += rk) {
		//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV;
		i32 x = (ilx & ~MASK) + ((ilx & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 s = (x >= irx) ? 0 : (irx - x + _0_9) >> OFST;
		for (i32* cp = ctx.cbuf + y * ctx.width + (x >> OFST); s--; *cp++ = ctx.il.s.c);
	}//*/

	//
	// id == 0
	// id == count-1
}

void spans_(size_t id, size_t count, BaryCtx& ctx) {
	//TODO: check clear atomic_flag[count] here and clear the rect?
	_MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
	i32 y = i32(ctx.sy), ry = i32(ctx.ey + f0_49);
	if (y >= ry) return; // move to add

	i32* cptr = ctx.width * y + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * y + ctx.wbuf;
#	else
	f32* zptr = ctx.width * y + ctx.zbuf;
#	endif

	const int pix = 4, msk = pix - 1;
	const __m128i _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(pix);

	size_t divisor = ctx.height / count;
	cptr = ctx.cbuf + (ctx.isy >> OFST) * ctx.width;
	for (int y = ctx.isy; y < ctx.iey; y += _1_0, ++ctx.il, ++ctx.ir, cptr += ctx.width) {
		if ((y >> OFST) / divisor != id) continue; // use this, check `y` regions with sy/ey
		int x1 = (ctx.il.x & ~MASK) + ((ctx.il.x & MASK) <= _0_5 ? _0_5 : _1_5);
		int ix = x1 >> OFST;
		if (x1 >= ctx.ir.x) continue;

		//* int128
		int ofs = ix & ~msk;
		int rst = ix & msk;
		int szx = (ctx.ir.x - x1 + _1_0 - 1) >> OFST;
		int cnt = (szx + rst + pix - 1) / pix;

		__m128i const src4 = _mm_set1_epi32(ctx.il.s.c);
		__m128i const l4 = _mm_set1_epi32(ix - 1), r4 = _mm_set1_epi32(ix + szx);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3
		__m128i lrm4 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l4), _mm_cmplt_epi32(ofs4, r4));

		for (; cnt--; ofs += pix, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; //Z/W mapping; X/Y-W?
			__m128i dst4 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr + ofs));
			__m128i lrm4 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l4), _mm_cmplt_epi32(ofs4, r4));
			__m128i res4 = _mm_or_si128(_mm_andnot_si128(lrm4, dst4), _mm_and_si128(lrm4, src4));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr + ofs), res4);
		}

		//for (int x = x1, iix = ix; x < ctx.ir.x; x += _1_0, *(cptr + iix++) = 0xFF00FF00);
		//for (int sz = (ctx.ir.x - x1 + _1_0 - 1 >> OFST), iix = ix; sz--; *(cptr + iix++) = ctx.il.s.c;);
	}
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
}

void spansi4(size_t id, size_t count, BaryCtx& ctx) {
	//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; // W only
	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1;
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2;
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);

	i32 const Qy = 4, Qx = 4;
	__m128 const half4 = _mm_set1_ps(.5f), _255f = _mm_set1_ps(255.f);
	__m128i const _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(Qx);

	i32 qys = iszy / Qy;
	i32 qyr = iszy & (Qy - 1);
	//*
	for (i32 y = iy1; qys--; y += Qy, ilx += lk * Qy, irx += rk * Qy) {
		i32 qy0 = y, qy1 = y + 1, qy2 = y + 2, qy3 = y + 3;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1, lx2 = ilx + lk * 2, lx3 = ilx + lk * 3;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1, rx2 = irx + rk * 2, rx3 = irx + rk * 3;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x2 = (lx2 & ~MASK) + ((lx2 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x3 = (lx3 & ~MASK) + ((lx3 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;
		i32 s2 = (x2 >= rx2) ? 0 : (rx2 - x2 + _0_9) >> OFST;
		i32 s3 = (x3 >= rx3) ? 0 : (rx3 - x3 + _0_9) >> OFST;
		/*
		for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy1 * ctx.width + (x1 >> OFST); s1--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy2 * ctx.width + (x2 >> OFST); s2--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy3 * ctx.width + (x3 >> OFST); s3--; *cp++ = ctx.il.s.c);//*/

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST, ix2 = x2 >> OFST, ix3 = x3 >> OFST;

		i32 mx = std::min(x0, x3);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx3);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		//__m128i src4 = _mm_set1_epi32(ctx.il.s.c);

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);
		__m128i const l2 = _mm_set1_epi32(ix2 - 1), r2 = _mm_set1_epi32(ix2 + s2);
		__m128i const l3 = _mm_set1_epi32(ix3 - 1), r3 = _mm_set1_epi32(ix3 + s3);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);
		__m128 qy2f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy2)), half4);
		__m128 qy3f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy3)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;
		i32* cptr2 = ctx.cbuf + qy2 * ctx.width;
		i32* cptr3 = ctx.cbuf + qy3 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 xxxx1 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 xxxx2 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 xxxx3 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(3, 3, 3, 3));

			//__m128 wwww0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy0f4)), ctx.FV.wwww());
			//__m128 wwww1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy1f4)), ctx.FV.wwww());
			//__m128 wwww2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy2f4)), ctx.FV.wwww());
			//__m128 wwww3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy3f4)), ctx.FV.wwww());

			//__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
			//__m128 zzzz1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy1f4)), ctx.FV.zzzz());
			//__m128 zzzz2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy2f4)), ctx.FV.zzzz());
			//__m128 zzzz3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy3f4)), ctx.FV.zzzz());

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);
			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);
			__m128 c20 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy2f4)), ctx.FC);
			__m128 c21 = _mm_add_ps(c20, ctx.IC), c22 = _mm_add_ps(c21, ctx.IC), c23 = _mm_add_ps(c22, ctx.IC);
			__m128 c30 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy3f4)), ctx.FC);
			__m128 c31 = _mm_add_ps(c30, ctx.IC), c32 = _mm_add_ps(c31, ctx.IC), c33 = _mm_add_ps(c32, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic20 = _mm_cvtps_epi32(_mm_mul_ps(c20, _255f));
			__m128i ic21 = _mm_cvtps_epi32(_mm_mul_ps(c21, _255f));
			__m128i ic22 = _mm_cvtps_epi32(_mm_mul_ps(c22, _255f));
			__m128i ic23 = _mm_cvtps_epi32(_mm_mul_ps(c23, _255f));

			__m128i ic30 = _mm_cvtps_epi32(_mm_mul_ps(c30, _255f));
			__m128i ic31 = _mm_cvtps_epi32(_mm_mul_ps(c31, _255f));
			__m128i ic32 = _mm_cvtps_epi32(_mm_mul_ps(c32, _255f));
			__m128i ic33 = _mm_cvtps_epi32(_mm_mul_ps(c33, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic00, ic01), _mm_packs_epi32(ic02, ic03));
			__m128i ic13210 = _mm_packus_epi16(_mm_packs_epi32(ic10, ic11), _mm_packs_epi32(ic12, ic13));
			__m128i ic23210 = _mm_packus_epi16(_mm_packs_epi32(ic20, ic21), _mm_packs_epi32(ic22, ic23));
			__m128i ic33210 = _mm_packus_epi16(_mm_packs_epi32(ic30, ic31), _mm_packs_epi32(ic32, ic33));

			//__m128 t0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy0) + .5f))), ctx.FT);
			//__m128 t1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy1) + .5f))), ctx.FT);
			//__m128 t2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy2) + .5f))), ctx.FT);
			//__m128 t3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy3) + .5f))), ctx.FT);

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);

			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);

			__m128i dst2 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr2 + ofs));
			__m128i lrm2 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l2), _mm_cmplt_epi32(ofs4, r2));
			__m128i res2 = _mm_or_si128(_mm_andnot_si128(lrm2, dst2), _mm_and_si128(lrm2, ic23210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr2 + ofs), res2);

			__m128i dst3 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr3 + ofs));
			__m128i lrm3 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l3), _mm_cmplt_epi32(ofs4, r3));
			__m128i res3 = _mm_or_si128(_mm_andnot_si128(lrm3, dst3), _mm_and_si128(lrm3, ic33210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr3 + ofs), res3);
		}
	}//*/

	i32 y = iy1 + (iszy & ~(Qy - 1));
	if (qyr > 1) {
		qyr -= 2;

		i32 qy0 = y, qy1 = y + 1;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST;

		i32 mx = std::min(x0, x1);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx1);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;

		for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);
		for (i32* cp = ctx.cbuf + qy1 * ctx.width + (x1 >> OFST); s1--; *cp++ = ctx.il.s.c);

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 xxxx1 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC);
			__m128 c02 = _mm_add_ps(c01, ctx.IC);
			__m128 c03 = _mm_add_ps(c02, ctx.IC);

			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC);
			__m128 c12 = _mm_add_ps(c11, ctx.IC);
			__m128 c13 = _mm_add_ps(c12, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));
			__m128i ic13210 = _mm_packus_epi16(_mm_packs_epi32(ic13, ic12), _mm_packs_epi32(ic11, ic10));

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);

			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);
		}

		y += 2;
		ilx += lk * 2;
		irx += rk * 2;
	}

	if (qyr) {
		qyr--;

		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST;

		i32 mx = x0;
		i32 ix = mx >> OFST, rx = rx0;

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;

		//for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC);
			__m128 c02 = _mm_add_ps(c01, ctx.IC);
			__m128 c03 = _mm_add_ps(c02, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
		}
	}

	/*
	for (i32 y = iy1 + (iszy & ~(Qy - 1)); qyr--; y++, ilx += lk, irx += rk) {
		// calc 4, use 1-3 (maybe use extra mask with `less than iy2+1` for each row)
		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		for (i32* cp = ctx.cbuf + qy0 * ctx.width + (x0 >> OFST); s0--; *cp++ = ctx.il.s.c);
	}//*/
}

void spansi5(size_t id, size_t count, BaryCtx& ctx) {
	//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; // W only
	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1;
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2;
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);

	i32 const Qy = 4, Qx = 4;
	__m128 const half4 = _mm_set1_ps(.5f), _255f = _mm_set1_ps(255.f);
	__m128i const _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(Qx);

	i32 qys = iszy / Qy;
	i32 qyr = iszy & (Qy - 1);

	for (i32 y = iy1; qys--; y += Qy, ilx += lk * Qy, irx += rk * Qy) {
		i32 qy0 = y, qy1 = y + 1, qy2 = y + 2, qy3 = y + 3;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1, lx2 = ilx + lk * 2, lx3 = ilx + lk * 3;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1, rx2 = irx + rk * 2, rx3 = irx + rk * 3;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x2 = (lx2 & ~MASK) + ((lx2 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x3 = (lx3 & ~MASK) + ((lx3 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;
		i32 s2 = (x2 >= rx2) ? 0 : (rx2 - x2 + _0_9) >> OFST;
		i32 s3 = (x3 >= rx3) ? 0 : (rx3 - x3 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST, ix2 = x2 >> OFST, ix3 = x3 >> OFST;

		i32 mx = std::min(x0, x3);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx3);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);
		__m128i const l2 = _mm_set1_epi32(ix2 - 1), r2 = _mm_set1_epi32(ix2 + s2);
		__m128i const l3 = _mm_set1_epi32(ix3 - 1), r3 = _mm_set1_epi32(ix3 + s3);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);
		__m128 qy2f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy2)), half4);
		__m128 qy3f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy3)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;
		i32* cptr2 = ctx.cbuf + qy2 * ctx.width;
		i32* cptr3 = ctx.cbuf + qy3 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 xxxx1 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 xxxx2 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 xxxx3 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(3, 3, 3, 3));

			//__m128 wwww0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy0f4)), ctx.FV.wwww());
			//__m128 wwww1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy1f4)), ctx.FV.wwww());
			//__m128 wwww2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy2f4)), ctx.FV.wwww());
			//__m128 wwww3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy3f4)), ctx.FV.wwww());

			//__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
			//__m128 zzzz1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy1f4)), ctx.FV.zzzz());
			//__m128 zzzz2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy2f4)), ctx.FV.zzzz());
			//__m128 zzzz3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy3f4)), ctx.FV.zzzz());

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);
			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);
			__m128 c20 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy2f4)), ctx.FC);
			__m128 c21 = _mm_add_ps(c20, ctx.IC), c22 = _mm_add_ps(c21, ctx.IC), c23 = _mm_add_ps(c22, ctx.IC);
			__m128 c30 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy3f4)), ctx.FC);
			__m128 c31 = _mm_add_ps(c30, ctx.IC), c32 = _mm_add_ps(c31, ctx.IC), c33 = _mm_add_ps(c32, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic20 = _mm_cvtps_epi32(_mm_mul_ps(c20, _255f));
			__m128i ic21 = _mm_cvtps_epi32(_mm_mul_ps(c21, _255f));
			__m128i ic22 = _mm_cvtps_epi32(_mm_mul_ps(c22, _255f));
			__m128i ic23 = _mm_cvtps_epi32(_mm_mul_ps(c23, _255f));

			__m128i ic30 = _mm_cvtps_epi32(_mm_mul_ps(c30, _255f));
			__m128i ic31 = _mm_cvtps_epi32(_mm_mul_ps(c31, _255f));
			__m128i ic32 = _mm_cvtps_epi32(_mm_mul_ps(c32, _255f));
			__m128i ic33 = _mm_cvtps_epi32(_mm_mul_ps(c33, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic00, ic01), _mm_packs_epi32(ic02, ic03));
			__m128i ic13210 = _mm_packus_epi16(_mm_packs_epi32(ic10, ic11), _mm_packs_epi32(ic12, ic13));
			__m128i ic23210 = _mm_packus_epi16(_mm_packs_epi32(ic20, ic21), _mm_packs_epi32(ic22, ic23));
			__m128i ic33210 = _mm_packus_epi16(_mm_packs_epi32(ic30, ic31), _mm_packs_epi32(ic32, ic33));

			//__m128 t0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy0) + .5f))), ctx.FT);
			//__m128 t1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy1) + .5f))), ctx.FT);
			//__m128 t2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy2) + .5f))), ctx.FT);
			//__m128 t3 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy3) + .5f))), ctx.FT);

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);

			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);

			__m128i dst2 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr2 + ofs));
			__m128i lrm2 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l2), _mm_cmplt_epi32(ofs4, r2));
			__m128i res2 = _mm_or_si128(_mm_andnot_si128(lrm2, dst2), _mm_and_si128(lrm2, ic23210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr2 + ofs), res2);

			__m128i dst3 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr3 + ofs));
			__m128i lrm3 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l3), _mm_cmplt_epi32(ofs4, r3));
			__m128i res3 = _mm_or_si128(_mm_andnot_si128(lrm3, dst3), _mm_and_si128(lrm3, ic33210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr3 + ofs), res3);
		}
	}

	i32 y = iy1 + (iszy & ~(Qy - 1));
	if (qyr > 1) {
		qyr -= 2;

		i32 qy0 = y, qy1 = y + 1;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST;

		i32 mx = std::min(x0, x1);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx1);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 xxxx1 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);

			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));
			__m128i ic13210 = _mm_packus_epi16(_mm_packs_epi32(ic13, ic12), _mm_packs_epi32(ic11, ic10));

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);

			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);
		}

		y += 2;
		ilx += lk * 2;
		irx += rk * 2;
	}

	if (qyr) {
		qyr--;

		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST;

		i32 mx = x0;
		i32 ix = mx >> OFST, rx = rx0;

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC);
			__m128 c02 = _mm_add_ps(c01, ctx.IC);
			__m128 c03 = _mm_add_ps(c02, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
		}
	}
}

void spansi6(size_t id, size_t count, BaryCtx& ctx) {
	//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; // W only
	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1;
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2;
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);

	i32 const Qy = 2, Qx = 4;
	__m128 const half4 = _mm_set1_ps(.5f), _255f = _mm_set1_ps(255.f);
	__m128i const _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(Qx);

	i32 qys = iszy / Qy;
	i32 qyr = iszy & (Qy - 1);

	for (i32 y = iy1; qys--; y += Qy, ilx += lk * Qy, irx += rk * Qy) {
		i32 qy0 = y, qy1 = y + 1;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST;

		i32 mx = std::min(x0, x1);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx1);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 xxxx1 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(1, 1, 1, 1));

			//__m128 wwww0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy0f4)), ctx.FV.wwww());
			//__m128 wwww1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy1f4)), ctx.FV.wwww());

			//__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
			//__m128 zzzz1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy1f4)), ctx.FV.zzzz());

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);
			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic00, ic01), _mm_packs_epi32(ic02, ic03));
			__m128i ic13210 = _mm_packus_epi16(_mm_packs_epi32(ic10, ic11), _mm_packs_epi32(ic12, ic13));

			//__m128 t0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy0) + .5f))), ctx.FT);
			//__m128 t1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy1) + .5f))), ctx.FT);

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);

			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);
		}
	}

	i32 y = iy1 + (iszy & ~(Qy - 1));
	if (qyr) {
		qyr--;

		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST;

		i32 mx = x0;
		i32 ix = mx >> OFST, rx = rx0;

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC);
			__m128 c02 = _mm_add_ps(c01, ctx.IC);
			__m128 c03 = _mm_add_ps(c02, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
		}
	}
}

void spansi7(size_t id, size_t count, BaryCtx& ctx) {
	//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; // W only
	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1;
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2;
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);

	i32 const Qy = 2, Qx = 4;
	__m128 const half4 = _mm_set1_ps(.5f), _255f = _mm_set1_ps(255.f);
	__m128i const _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(Qx);

	i32 qys = iszy / Qy;
	i32 qyr = iszy & (Qy - 1);

	for (i32 y = iy1; qys--; y += Qy, ilx += lk * Qy, irx += rk * Qy) {
		//if (y <= 360 || y > 370) continue;
		i32 qy0 = y, qy1 = y + 1;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST;

		i32 mx = std::min(x0, x1);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx1);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;
		f32* zptr0 = ctx.zbuf + qy0 * ctx.width;
		f32* zptr1 = ctx.zbuf + qy1 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			//if (ofs > 796) continue;
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
			__m128 zzzz1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy1f4)), ctx.FV.zzzz());
//*
#			ifdef __WBUF__
			if (ctx.bDepthTest && !ctx.depthOpW(1.f / xv.z, *wp)) continue;
#			else
			//if (ctx.bDepthTest && !ctx.depthOpZ4(xv.z, *zp)) {
			__m128 zdst0, zdst1, zmask0, zmask1;
			if (ctx.bDepthTest) {
				zdst0 = _mm_load_ps(zptr0 + ofs);
				zdst1 = _mm_load_ps(zptr1 + ofs);
				zmask0 = ctx.depthOpZ4(zzzz0, zdst0);
				zmask1 = ctx.depthOpZ4(zzzz1, zdst1);

				__m128i zmask10 = _mm_packs_epi32(_mm_castps_si128(zmask0), _mm_castps_si128(zmask1));
				if (!_mm_movemask_epi8(zmask10))
					continue;
			}
#			endif
//*/
			//__m128 wwww0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy0f4)), ctx.FV.wwww());
			//__m128 wwww1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy1f4)), ctx.FV.wwww());

			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);
			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic010 = _mm_packs_epi32(ic00, ic01);
			__m128i ic032 = _mm_packs_epi32(ic02, ic03);
			__m128i ic03210 = _mm_packus_epi16(ic010, ic032);

			__m128i ic110 = _mm_packs_epi32(ic10, ic11);
			__m128i ic132 = _mm_packs_epi32(ic12, ic13);
			__m128i ic13210 = _mm_packus_epi16(ic110, ic132);

			//__m128 t0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy0) + .5f))), ctx.FT);
			//__m128 t1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy1) + .5f))), ctx.FT);

			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));
			//*
			if (ctx.bAlphaTest) {
				__m128i amask0 = ctx.alphaOpI4(ic03210, ctx.alphaRefI4);
				__m128i amask1 = ctx.alphaOpI4(ic13210, ctx.alphaRefI4);
				__m128i amask10 = _mm_packs_epi32(amask0, amask1);
				if (!_mm_movemask_epi8(amask10))
					continue;
				lrm0 = _mm_and_si128(lrm0, amask0);
				lrm1 = _mm_and_si128(lrm1, amask1);
			}//*/
			//*
			if (ctx.bDepthTest) {
				lrm0 = _mm_and_si128(lrm0, _mm_castps_si128(zmask0));
				lrm1 = _mm_and_si128(lrm1, _mm_castps_si128(zmask1));

				if (ctx.bDepthMask) {
#				ifdef __WBUF__
					*wp = 1.f / xv.z;
#				else
					//*zp = xv.z;
					__m128 zres0 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm0), zdst0), _mm_and_ps(_mm_castsi128_ps(lrm0), zzzz0));
					__m128 zres1 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm1), zdst1), _mm_and_ps(_mm_castsi128_ps(lrm1), zzzz1));
					_mm_store_ps(zptr0 + ofs, zres0);
					_mm_store_ps(zptr1 + ofs, zres1);
#				endif
				}
			}//*/

			//if (ctx.bBlend) {
			//	c8 const d = _mx_i32_epi16(*cp);
			//	c = _mx_madd_epi16(_mm_unpacklo_epi16(c, d), _mm_unpacklo_epi16(srcBlendProc8(c, d), dstBlendProc8(c, d)));
			//}

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);
		}
	}

	i32 y = iy1 + (iszy & ~(Qy - 1));
	if (qyr) {
		qyr--;

		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST;

		i32 mx = x0;
		i32 ix = mx >> OFST, rx = rx0;

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		f32* zptr0 = ctx.zbuf + qy0 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
//*
#			ifdef __WBUF__
			if (ctx.bDepthTest && !ctx.depthOpW(1.f / xv.z, *wp)) continue;
#			else
			__m128 zdst0, zmask0;
			if (ctx.bDepthTest) {
				zdst0 = _mm_load_ps(zptr0 + ofs);
				zmask0 = ctx.depthOpZ4(zzzz0, zdst0);

				if (!_mm_movemask_epi8(_mm_castps_si128(zmask0)))
					continue;
			}
#			endif
//*/
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic03210 = _mm_packus_epi16(_mm_packs_epi32(ic03, ic02), _mm_packs_epi32(ic01, ic00));

			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			//*
			if (ctx.bAlphaTest) {
				__m128i amask0 = ctx.alphaOpI4(ic03210, ctx.alphaRefI4);
				if (!_mm_movemask_epi8(amask0))
					continue;
				lrm0 = _mm_and_si128(lrm0, amask0);
			}//*/
			//*
			if (ctx.bDepthTest) {
				lrm0 = _mm_and_si128(lrm0, _mm_castps_si128(zmask0));

				if (ctx.bDepthMask) {
#				ifdef __WBUF__
					*wp = 1.f / xv.z;
#				else
					//*zp = xv.z;
					__m128 zres0 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm0), zdst0), _mm_and_ps(_mm_castsi128_ps(lrm0), zzzz0));
					_mm_store_ps(zptr0 + ofs, zres0);
#				endif
				}
			}//*/

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
		}
	}
}

void spansi8(size_t id, size_t count, BaryCtx& ctx) {
	//v4 v = ctx.IV * (qx + .5f) + ctx.JV * (qy + .5f) + ctx.FV; // W only
	auto blend = [](cref<__m128i> a, cref<__m128i> b, cref<__m128i> p, cref<__m128i> q) noexcept -> __m128i {
		__m128i tl = _mm_unpacklo_epi16(a, b), sl = _mm_unpacklo_epi16(p, q);
		__m128i th = _mm_unpackhi_epi16(a, b), sh = _mm_unpackhi_epi16(p, q);
		__m128i rl = _mx_madd_epi16(tl, sl), rh = _mx_madd_epi16(th, sh);
		__m128i w = _mm_or_si128(rl, _mm_shuffle_epi32(rh, _MM_SHUFFLE(1, 0, 3, 2)));
		return w;
	};

	if (ctx.isy >= ctx.iey) return; // move to add

	i32* cptr = ctx.width * (ctx.isy >> OFST) + ctx.cbuf;
#	ifdef __WBUF__
	f32* wptr = ctx.width * (ctx.isy >> OFST) + ctx.wbuf;
#	else
	f32* zptr = ctx.width * (ctx.isy >> OFST) + ctx.zbuf;
#	endif

	// setup window
	int wh = ctx.height / count + ((ctx.height & (count - 1)) ? 1 : 0);
	int wy1 = wh * id;
	int wy2 = wy1 + (wh - 1);
	if (wy2 > ctx.height - 1)
		wy2 = ctx.height - 1;

	// setup triangle
	i32 const _0_9 = _1_0 - 1;
	i32 const tszy = (ctx.iey - ctx.isy + _1_0 - 1) >> OFST;
	i32 const ty1 = ctx.isy >> OFST;
	i32 const ty2 = ty1 + tszy - 1;
	if (ty1 > wy2 || ty2 < wy1)
		return;
	i32 const iy1 = ty1 >= wy1 ? ty1 : wy1;
	i32 const iy2 = ty2 <= wy2 ? ty2 : wy2;
	i32 const iszy = tszy - (iy1 - ty1) - (ty2 - iy2);
	i32 const lk = ctx.il.k, rk = ctx.ir.k;
	i32 ilx = ctx.il.x + lk * (iy1 - ty1);
	i32 irx = ctx.ir.x + rk * (iy1 - ty1);

	i32 const Qy = 2, Qx = 4;
	__m128 const half4 = _mm_set1_ps(.5f), _255f = _mm_set1_ps(255.f);
	__m128i const _0123 = _mm_set_epi32(3, 2, 1, 0), inc4 = _mm_set1_epi32(Qx);

	i32 qys = iszy / Qy;
	i32 qyr = iszy & (Qy - 1);

	BlendTools::BlendProc8 srcBlendProc8 = BlendTools::blendProcTable8[ctx.sfactor];
	BlendTools::BlendProc8 dstBlendProc8 = BlendTools::blendProcTable8[ctx.dfactor];

	for (i32 y = iy1; qys--; y += Qy, ilx += lk * Qy, irx += rk * Qy) {
		//if (y <= 360 || y > 370) continue;
		i32 qy0 = y, qy1 = y + 1;
		i32 lx0 = ilx + lk * 0, lx1 = ilx + lk * 1;
		i32 rx0 = irx + rk * 0, rx1 = irx + rk * 1;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);
		i32 x1 = (lx1 & ~MASK) + ((lx1 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;
		i32 s1 = (x1 >= rx1) ? 0 : (rx1 - x1 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST, ix1 = x1 >> OFST;

		i32 mx = std::min(x0, x1);
		i32 ix = mx >> OFST, rx = std::max(rx0, rx1);

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);
		__m128i const l1 = _mm_set1_epi32(ix1 - 1), r1 = _mm_set1_epi32(ix1 + s1);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);
		__m128 qy1f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy1)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		i32* cptr1 = ctx.cbuf + qy1 * ctx.width;
		f32* zptr0 = ctx.zbuf + qy0 * ctx.width;
		f32* zptr1 = ctx.zbuf + qy1 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			//if (ofs > 796) continue;
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
			__m128 zzzz1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy1f4)), ctx.FV.zzzz());
#			ifdef __WBUF__
			if (ctx.bDepthTest && !ctx.depthOpW(1.f / xv.z, *wp)) continue;
#			else
			//if (ctx.bDepthTest && !ctx.depthOpZ4(xv.z, *zp)) {
			__m128 zdst0, zdst1, zmask0, zmask1;
			if (ctx.bDepthTest) {
				zdst0 = _mm_load_ps(zptr0 + ofs);
				zdst1 = _mm_load_ps(zptr1 + ofs);
				zmask0 = ctx.depthOpZ4(zzzz0, zdst0);
				zmask1 = ctx.depthOpZ4(zzzz1, zdst1);

				__m128i zmask10 = _mm_packs_epi32(_mm_castps_si128(zmask0), _mm_castps_si128(zmask1));
				if (!_mm_movemask_epi8(zmask10))
					continue;
			}
#			endif

			//__m128 wwww0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy0f4)), ctx.FV.wwww());
			//__m128 wwww1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.wwww(), x0x1x2x3), _mm_mul_ps(ctx.JV.wwww(), qy1f4)), ctx.FV.wwww());

			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);
			__m128 c10 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy1f4)), ctx.FC);
			__m128 c11 = _mm_add_ps(c10, ctx.IC), c12 = _mm_add_ps(c11, ctx.IC), c13 = _mm_add_ps(c12, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic10 = _mm_cvtps_epi32(_mm_mul_ps(c10, _255f));
			__m128i ic11 = _mm_cvtps_epi32(_mm_mul_ps(c11, _255f));
			__m128i ic12 = _mm_cvtps_epi32(_mm_mul_ps(c12, _255f));
			__m128i ic13 = _mm_cvtps_epi32(_mm_mul_ps(c13, _255f));

			__m128i ic010 = _mm_packs_epi32(ic00, ic01);
			__m128i ic032 = _mm_packs_epi32(ic02, ic03);
			__m128i ic03210 = _mm_packus_epi16(ic010, ic032);

			__m128i ic110 = _mm_packs_epi32(ic10, ic11);
			__m128i ic132 = _mm_packs_epi32(ic12, ic13);
			__m128i ic13210 = _mm_packus_epi16(ic110, ic132);

			//__m128 t0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy0) + .5f))), ctx.FT);
			//__m128 t1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IT, x0x1x2x3), _mm_mul_ps(ctx.JT, _mm_set1_ps(f32(qy1) + .5f))), ctx.FT);

			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			__m128i lrm1 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l1), _mm_cmplt_epi32(ofs4, r1));

			if (ctx.bAlphaTest) {
				__m128i amask0 = ctx.alphaOpI4(ic03210, ctx.alphaRefI4);
				__m128i amask1 = ctx.alphaOpI4(ic13210, ctx.alphaRefI4);
				__m128i amask10 = _mm_packs_epi32(amask0, amask1);
				if (!_mm_movemask_epi8(amask10))
					continue;
				lrm0 = _mm_and_si128(lrm0, amask0);
				lrm1 = _mm_and_si128(lrm1, amask1);
			}

			if (ctx.bDepthTest) {
				lrm0 = _mm_and_si128(lrm0, _mm_castps_si128(zmask0));
				lrm1 = _mm_and_si128(lrm1, _mm_castps_si128(zmask1));

				if (ctx.bDepthMask) {
#				ifdef __WBUF__
					*wp = 1.f / xv.z;
#				else
					//*zp = xv.z;
					__m128 zres0 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm0), zdst0), _mm_and_ps(_mm_castsi128_ps(lrm0), zzzz0));
					__m128 zres1 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm1), zdst1), _mm_and_ps(_mm_castsi128_ps(lrm1), zzzz1));
					_mm_store_ps(zptr0 + ofs, zres0);
					_mm_store_ps(zptr1 + ofs, zres1);
#				endif
				}
			}

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));
			__m128i dst1 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr1 + ofs));

			if (ctx.bBlend) {
				/*
				__m128i a = _mm_set_epi16(255, 000, 000, 255, 255, 000, 255, 000);
				__m128i b = _mm_set_epi16(255, 255, 000, 000, 255, 255, 000, 000);
				__m128i p = _mm_set_epi16( 64,  64,  64,  64,  64,  64,  64,  64);
				__m128i q = _mm_set_epi16(192, 192, 192, 192, 192, 192, 192, 192);

				__m128i tl = _mm_unpacklo_epi16(a, b);
				__m128i sl = _mm_unpacklo_epi16(p, q);

				__m128i th = _mm_unpackhi_epi16(a, b);
				__m128i sh = _mm_unpackhi_epi16(p, q);

				__m128i rl = _mx_madd_epi16(tl, sl);
				__m128i rh = _mx_madd_epi16(th, sh);

				__m128i u = _mm_packus_epi16(rl, rh);
				__m128i v = _mm_shuffle_epi32(u, _MM_SHUFFLE(3, 3, 2, 0));
				__m128i w = _mm_or_si128(rl, _mm_shuffle_epi32(rh, _MM_SHUFFLE(1, 0, 3, 2)));
				__m128i w2 = blend(a, b, p, q);
				__m128i w3 = _mm_packus_epi16(w2, w2);
				//*/

				/*
				__m128i a2 = _mm_set_epi8(255, 255, 0, 255, 255, 255, 255, 0, 255,   0, 0, 255, 255,   0, 255, 0);
				__m128i b2 = _mm_set_epi8(255, 255, 0,   0, 255, 255,   0, 0, 255, 255, 0,   0, 255, 255,   0, 0);
				__m128i p2 = _mm_set_epi8(64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64);
				__m128i q2 = _mm_set_epi8(192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192);

				__m128i tl2 = _mm_unpacklo_epi8(a2, b2);
				__m128i sl2 = _mm_unpacklo_epi8(p2, q2);

				__m128i th2 = _mm_unpackhi_epi8(a2, b2);
				__m128i sh2 = _mm_unpackhi_epi8(p2, q2);

				__m128i rl2 = _mm_maddubs_epi16(tl2, sl2);
				__m128i rh2 = _mm_maddubs_epi16(th2, sh2);

				__m128i rl3 = _mm_srli_epi16(rl2, 8);
				__m128i rh3 = _mm_srli_epi16(rh2, 8);
				__m128i rh4 = _mm_packus_epi16(rl3, rh3);//*/

				__m128i d010 = _mm_unpacklo_epi8(dst0, _mm_setzero_si128());
				__m128i d032 = _mm_unpackhi_epi8(dst0, _mm_setzero_si128());
				__m128i d110 = _mm_unpacklo_epi8(dst1, _mm_setzero_si128());
				__m128i d132 = _mm_unpackhi_epi8(dst1, _mm_setzero_si128());

				__m128i r010 = blend(ic010, d010, srcBlendProc8(ic010, d010), dstBlendProc8(ic010, d010));
				__m128i r032 = blend(ic032, d032, srcBlendProc8(ic032, d032), dstBlendProc8(ic032, d032));
				__m128i r110 = blend(ic110, d110, srcBlendProc8(ic110, d110), dstBlendProc8(ic110, d110));
				__m128i r132 = blend(ic132, d132, srcBlendProc8(ic132, d132), dstBlendProc8(ic132, d132));

				//c8 const d = _mx_i32_epi16(*cp);
				//c = _mx_madd_epi16(_mm_unpacklo_epi16(c, d), _mm_unpacklo_epi16(srcBlendProc8(c, d), dstBlendProc8(c, d)));
				ic03210 = _mm_packus_epi16(r010, r032);
				ic13210 = _mm_packus_epi16(r110, r132);
			}

			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			__m128i res1 = _mm_or_si128(_mm_andnot_si128(lrm1, dst1), _mm_and_si128(lrm1, ic13210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr1 + ofs), res1);
		}
	}

	i32 y = iy1 + (iszy & ~(Qy - 1));
	if (qyr) {
		qyr--;

		i32 qy0 = y;
		i32 lx0 = ilx + lk * 0;
		i32 rx0 = irx + rk * 0;

		i32 x0 = (lx0 & ~MASK) + ((lx0 & MASK) <= _0_5 ? _0_5 : _1_5);

		i32 s0 = (x0 >= rx0) ? 0 : (rx0 - x0 + _0_9) >> OFST;

		i32 ix0 = x0 >> OFST;

		i32 mx = x0;
		i32 ix = mx >> OFST, rx = rx0;

		int ofs = ix & ~(Qx - 1);
		int rst = ix & (Qx - 1);
		int szx = (rx - mx + _0_9) >> OFST;
		int cnt = (szx + rst + Qx - 1) / Qx;

		__m128i const l0 = _mm_set1_epi32(ix0 - 1), r0 = _mm_set1_epi32(ix0 + s0);

		__m128i ofs4 = _mm_add_epi32(_mm_set1_epi32(ofs), _0123); //x0x1x2x3

		__m128 qy0f4 = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(qy0)), half4);

		i32* cptr0 = ctx.cbuf + qy0 * ctx.width;
		f32* zptr0 = ctx.zbuf + qy0 * ctx.width;

		for (; cnt--; ofs += Qx, ofs4 = _mm_add_epi32(ofs4, inc4)) {
			__m128 x0x1x2x3 = _mm_add_ps(_mm_cvtepi32_ps(ofs4), half4);
			__m128 zzzz0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IV.zzzz(), x0x1x2x3), _mm_mul_ps(ctx.JV.zzzz(), qy0f4)), ctx.FV.zzzz());
//*
#			ifdef __WBUF__
			if (ctx.bDepthTest && !ctx.depthOpW(1.f / xv.z, *wp)) continue;
#			else
			__m128 zdst0, zmask0;
			if (ctx.bDepthTest) {
				zdst0 = _mm_load_ps(zptr0 + ofs);
				zmask0 = ctx.depthOpZ4(zzzz0, zdst0);

				if (!_mm_movemask_epi8(_mm_castps_si128(zmask0)))
					continue;
			}
#			endif
//*/
			__m128 xxxx0 = _mm_shuffle_ps(x0x1x2x3, x0x1x2x3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 c00 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ctx.IC, xxxx0), _mm_mul_ps(ctx.JC, qy0f4)), ctx.FC);
			__m128 c01 = _mm_add_ps(c00, ctx.IC), c02 = _mm_add_ps(c01, ctx.IC), c03 = _mm_add_ps(c02, ctx.IC);

			__m128i ic00 = _mm_cvtps_epi32(_mm_mul_ps(c00, _255f));
			__m128i ic01 = _mm_cvtps_epi32(_mm_mul_ps(c01, _255f));
			__m128i ic02 = _mm_cvtps_epi32(_mm_mul_ps(c02, _255f));
			__m128i ic03 = _mm_cvtps_epi32(_mm_mul_ps(c03, _255f));

			__m128i ic010 = _mm_packs_epi32(ic00, ic01);
			__m128i ic032 = _mm_packs_epi32(ic02, ic03);
			__m128i ic03210 = _mm_packus_epi16(ic010, ic032);

			__m128i lrm0 = _mm_and_si128(_mm_cmpgt_epi32(ofs4, l0), _mm_cmplt_epi32(ofs4, r0));
			//*
			if (ctx.bAlphaTest) {
				__m128i amask0 = ctx.alphaOpI4(ic03210, ctx.alphaRefI4);
				if (!_mm_movemask_epi8(amask0))
					continue;
				lrm0 = _mm_and_si128(lrm0, amask0);
			}//*/
			//*
			if (ctx.bDepthTest) {
				lrm0 = _mm_and_si128(lrm0, _mm_castps_si128(zmask0));

				if (ctx.bDepthMask) {
#				ifdef __WBUF__
					*wp = 1.f / xv.z;
#				else
					//*zp = xv.z;
					__m128 zres0 = _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(lrm0), zdst0), _mm_and_ps(_mm_castsi128_ps(lrm0), zzzz0));
					_mm_store_ps(zptr0 + ofs, zres0);
#				endif
				}
			}//*/

			__m128i dst0 = _mm_load_si128(reinterpret_cast<__m128i const*>(cptr0 + ofs));

			if (ctx.bBlend) {
				__m128i d010 = _mm_unpacklo_epi8(dst0, _mm_setzero_si128());
				__m128i d032 = _mm_unpackhi_epi8(dst0, _mm_setzero_si128());

				__m128i r010 = blend(ic010, d010, srcBlendProc8(ic010, d010), dstBlendProc8(ic010, d010));
				__m128i r032 = blend(ic032, d032, srcBlendProc8(ic032, d032), dstBlendProc8(ic032, d032));

				ic03210 = _mm_packus_epi16(r010, r032);
			}

			__m128i res0 = _mm_or_si128(_mm_andnot_si128(lrm0, dst0), _mm_and_si128(lrm0, ic03210));
			_mm_store_si128(reinterpret_cast<__m128i*>(cptr0 + ofs), res0);
		}
	}
}
