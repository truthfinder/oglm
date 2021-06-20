/*
PostBuildEvent: "xcopy $(TargetPath) d:\games\quake1\opengl32.dll /R /Y"
LocalDebuggerCommand: "d:\games\quake1\glquake.exe"
LocalDebuggerWorkingDirectory: "d:\games\quake1\"
LocalDebuggerCommandArguments: "-window -width 640 -height 480"
*/

/*
HLOCAL hlocal = NULL;
BOOL fOk = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	NULL, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
	(LPTSTR) &hlocal, 0, NULL);
if (hlocal != NULL) {
	SetDlgItemText(hwnd, IDC_ERRORTEXT, (PCTSTR) LocalLock(hlocal));
	LocalFree(hlocal);
}
*/

//#define __WBUF__
#ifdef _DEBUG
#define CNT(cnt) cnt++
#else
#define CNT(cnt)
#endif

//#define _CRT_SECURE_NO_WARNINGS
#define NOGDI
#define WIN32_LEAN_AND_MEAN
//#define WIN32
#define _SECURE_SCL 0
//#define NDEBUG
//#define _WINDOWS
//#define _USRDLL

// TODO: SoA
// TODO: mipmap
// TODO: cut in clip+for
// TODO: byte4 color
// TODO: integer rasterization
// TODO: draw pixels

#pragma warning (disable: 4244)

#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <sstream>
#include <memory>

#include <array>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <unordered_map>
#include <set>
//#include <algorithm>
//#include <thread>
//#include <mutex>

#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>

#include <thread>
#include <mutex>

#include <intrin.h>

#include "../utils/types.h"
#include "../utils/mm_alloc.h"
#include "../utils/simd.h"
#include "../utils/gx.h"
#include "../utils/timer.h"
#include "../utils/helper.h"
#include "../utils/pnglite.h"

//GL_NO_ERROR
//GL_INVALID_ENUM
//GL_INVALID_VALUE
//GL_INVALID_OPERATION
//GL_STACK_OVERFLOW
//GL_STACK_UNDERFLOW
//GL_OUT_OF_MEMORY

template <typename T> using cref = T const&;

constexpr bool isPowerOfTwo(int v) {
	return (v > 0) && ((v & -v) == v);
}

#define macro_expand(x) x
#define verify1(expr) assert(expr); if (!(expr)) return
#define verify2(expr, ret) assert(expr); if (!(expr)) return (ret)
#define verify_macro(_1,_2,NAME,...) NAME
#define check_value(...) macro_expand(verify_macro(__VA_ARGS__, verify2, verify1))macro_expand((__VA_ARGS__))

template <typename T, typename P>
constexpr bool check(cref<T> v, cref<P> p) {
    return v == p;
}

template <typename T, typename P, typename... A>
constexpr bool check(cref<T> v, cref<P> p, A&&... a) {
    return (v == p) ? true : check(v, std::forward<A>(a)...);
}

template <typename S, typename... A>
constexpr bool _check_dbg(S s, A&&... a) {
    if (!check(std::forward<A>(a)...)) {
        OutputDebugStringA("parameter not in: ");
        OutputDebugStringA(s);
        OutputDebugStringA("\n");
        DebugBreak();
        return false;
    }
    return true;
}

template <typename... A>
constexpr bool _check(A&&... a) {
    if (!check(std::forward<A>(a)...)) {
        return false;
    }
    return true;
}

//#define __check(v, ...) assert(_check(#__VA_ARGS__, v, __VA_ARGS__) && "; not in " ## #__VA_ARGS__)
#ifdef _DEBUG
#define check_enum(v, ...) _check_dbg(#__VA_ARGS__, v, __VA_ARGS__)
#else
#define check_enum(v, ...) _check(v, __VA_ARGS__)
#endif

template <typename F>
struct FinalObject {
	FinalObject(F f) : f_(f) {}
	~FinalObject() { if (e_) f_(); }
	void disable() { e_ = false; }
	F f_;
	bool e_{ true };
};

template <typename F>
FinalObject<F> finally(F f) { return FinalObject<F>(f); };

typedef vec2 v2;
typedef vec3 v3;
typedef vec4 v4;
typedef mtx4 m4;
typedef tex4 t4;
typedef tex4 t4i;
typedef col4 c4;

#if 0//defined _DEBUG
bool logging = true;
#define LOG 1
#else
bool logging = false;
#define LOG 0
#endif

#if LOG == 1
#define log(fmt, ...) if (logging) { fprintf(f, fmt, __VA_ARGS__); fflush(f); }
#define dump(fmt, ...) fprintf(f, fmt, __VA_ARGS__); fflush(f)
#else
#define log(fmt, ...)
#define dump(fmt, ...)
#endif

template <class T, size_t sz = 16384>
class RingQueue {
	enum { Size = sz, Mask = Size - 1 };

public:
	typedef T value_type;

public:
	RingQueue() : items_(nullptr), head_(0), tail_(0) {
		static_assert(!(sz & (sz-1)));
		items_ = static_cast<value_type*>(_mm_malloc(sizeof(value_type)*Size, 32));
		for (size_t i = 0; i < Size; i++)
			new (&items_[i]) value_type();
	}
	~RingQueue() {
		assert(items_);
		if (items_) {
			_mm_free(items_);
			items_ = nullptr;
		}
	}
	
	int remain() {
		return (head_ - tail_ - 1) & Mask;
	}
	
	int size() {
		return (tail_ - head_) & Mask;
	}
	
	void push(cref<value_type> val) {
		if (!remain()) {
			assert(0 && "RingQueue::push: not remain");
			return;
		}
		items_[tail_] = val;
		tail_ = (tail_ + 1) & Mask;
	}
	
	value_type pop() {
		if (!size()) {
			assert(0 && "RingQueue::pop: !size()");
			return value_type();
		}
		const value_type val = items_[head_];
		head_ = (head_ + 1) & Mask;
		return val;
	}

	value_type& _top() {
		if (!size()) {
			assert(0 && "RingQueue::pop::size()");
			static value_type empty;
			return empty;
		}
		return items_[head_];
	}

private:
	int head_;
	int tail_;
	value_type* items_;
};

#ifdef __cplusplus
extern "C" {
#endif

#define DLL_EXPORT

#undef WINGDIAPI
#define WINGDIAPI DLL_EXPORT
#include <gl/gl.h>

#undef WINGDIAPI
#define WINGDIAPI
#undef WINAPI
#define WINAPI __stdcall

#define GL_CLAMP_TO_EDGE                    0x812F

#define GL_ACTIVE_TEXTURE                   0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE            0x84E1
#define GL_MAX_TEXTURE_COORDS               0x8871
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1

static const char* g_TexList[] = { "TEXTURE0", "TEXTURE1" };

#define FLAG_ON(f, i) ((f)|=(1<<(i)))
#define FLAG_OFF(f, i) ((f)&=~(1<<(i)))
#define FLAG_BIT(f, i) (((f)&(1<<(i)))!=0)
#define FLAG_MASK(f, m) (((f)&(m))==(m))

#define IRGB(r, g, b) (0xff000000 + ((r)<<16) + ((g)<<8) + (b))
#define IRGBA(r, g, b, a) (((a)<<24) + ((r)<<16) + ((g)<<8) + (b))
#define FRGB(r, g, b) (0xff000000 + (u32((r)*255)<<16) + (u32((g)*255)<<8) + (u32((b)*255)))
#define FRGBA(r, g, b, a) ((u32((a)*255)<<24) + (u32((r)*255)<<16) + (u32((g)*255)<<8) + (u32((b)*255)))

#define DIB_RGB_COLORS      0 // color table in RGBs

// Device Parameters for GetDeviceCaps()
#define DRIVERVERSION 0     // Device driver version
#define TECHNOLOGY    2     // Device classification
#define HORZSIZE      4     // Horizontal size in millimeters
#define VERTSIZE      6     // Vertical size in millimeters
#define HORZRES       8     // Horizontal width in pixels
#define VERTRES       10    // Vertical height in pixels
#define BITSPIXEL     12    // Number of bits per pixel
#define PLANES        14    // Number of planes
#define NUMBRUSHES    16    // Number of brushes the device has
#define NUMPENS       18    // Number of pens the device has
#define NUMMARKERS    20    // Number of markers the device has
#define NUMFONTS      22    // Number of fonts the device has
#define NUMCOLORS     24    // Number of colors the device supports
#define PDEVICESIZE   26    // Size required for device descriptor
#define CURVECAPS     28    // Curve capabilities
#define LINECAPS      30    // Line capabilities
#define POLYGONALCAPS 32    // Polygonal capabilities
#define TEXTCAPS      34    // Text capabilities
#define CLIPCAPS      36    // Clipping capabilities
#define RASTERCAPS    38    // Bitblt capabilities
#define ASPECTX       40    // Length of the X leg
#define ASPECTY       42    // Length of the Y leg
#define ASPECTXY      44    // Length of the hypotenuse

#define LOGPIXELSX    88    // Logical pixels/inch in X
#define LOGPIXELSY    90    // Logical pixels/inch in Y

#define SIZEPALETTE  104    // Number of entries in physical palette
#define NUMRESERVED  106    // Number of reserved entries in palette
#define COLORRES     108    // Actual color resolution

// constants for the biCompression field
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

typedef struct tagBITMAPINFOHEADER{
    DWORD      biSize;
    LONG       biWidth;
    LONG       biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    LONG       biXPelsPerMeter;
    LONG       biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef long            FXPT16DOT16;
typedef long            FXPT2DOT30;

// ICM Color Definitions
// The following two structures are used for defining RGB's in terms of CIEXYZ.

typedef struct tagCIEXYZ
{
        FXPT2DOT30 ciexyzX;
        FXPT2DOT30 ciexyzY;
        FXPT2DOT30 ciexyzZ;
} CIEXYZ;

typedef struct tagICEXYZTRIPLE
{
        CIEXYZ  ciexyzRed;
        CIEXYZ  ciexyzGreen;
        CIEXYZ  ciexyzBlue;
} CIEXYZTRIPLE;

typedef struct {
	DWORD        bV4Size;
	LONG         bV4Width;
	LONG         bV4Height;
	WORD         bV4Planes;
	WORD         bV4BitCount;
	DWORD        bV4V4Compression;
	DWORD        bV4SizeImage;
	LONG         bV4XPelsPerMeter;
	LONG         bV4YPelsPerMeter;
	DWORD        bV4ClrUsed;
	DWORD        bV4ClrImportant;
	DWORD        bV4RedMask;
	DWORD        bV4GreenMask;
	DWORD        bV4BlueMask;
	DWORD        bV4AlphaMask;
	DWORD        bV4CSType;
	CIEXYZTRIPLE bV4Endpoints;
	DWORD        bV4GammaRed;
	DWORD        bV4GammaGreen;
	DWORD        bV4GammaBlue;
} BITMAPV4HEADER, *PBITMAPV4HEADER;

typedef struct tagRGBQUAD {
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef int (APIENTRY * PFNGETDEVICECAPSPROC)(HDC hDC, int nIndex);

typedef int (APIENTRY * PFNSETDIBITSTODEVICEPROC)(
  _In_  HDC hdc,
  _In_  int XDest,
  _In_  int YDest,
  _In_  DWORD dwWidth,
  _In_  DWORD dwHeight,
  _In_  int XSrc,
  _In_  int YSrc,
  _In_  UINT uStartScan,
  _In_  UINT cScanLines,
  _In_  const VOID *lpvBits,
  _In_  const BITMAPINFO *lpbmi,
  _In_  UINT fuColorUse
);

PFNGETDEVICECAPSPROC GetDeviceCaps = 0;
PFNSETDIBITSTODEVICEPROC SetDIBitsToDevice = 0;

typedef struct tagPIXELFORMATDESCRIPTOR
{
	WORD  nSize;
	WORD  nVersion;
	DWORD dwFlags;
	BYTE  iPixelType;
	BYTE  cColorBits;
	BYTE  cRedBits;
	BYTE  cRedShift;
	BYTE  cGreenBits;
	BYTE  cGreenShift;
	BYTE  cBlueBits;
	BYTE  cBlueShift;
	BYTE  cAlphaBits;
	BYTE  cAlphaShift;
	BYTE  cAccumBits;
	BYTE  cAccumRedBits;
	BYTE  cAccumGreenBits;
	BYTE  cAccumBlueBits;
	BYTE  cAccumAlphaBits;
	BYTE  cDepthBits;
	BYTE  cStencilBits;
	BYTE  cAuxBuffers;
	BYTE  iLayerType;
	BYTE  bReserved;
	DWORD dwLayerMask;
	DWORD dwVisibleMask;
	DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;

// pixel types
#define PFD_TYPE_RGBA        0
#define PFD_TYPE_COLORINDEX  1

// layer types
#define PFD_MAIN_PLANE       0
#define PFD_OVERLAY_PLANE    1
#define PFD_UNDERLAY_PLANE   (-1)

// PIXELFORMATDESCRIPTOR flags
#define PFD_DOUBLEBUFFER            0x00000001
#define PFD_STEREO                  0x00000002
#define PFD_DRAW_TO_WINDOW          0x00000004
#define PFD_DRAW_TO_BITMAP          0x00000008
#define PFD_SUPPORT_GDI             0x00000010
#define PFD_SUPPORT_OPENGL          0x00000020
#define PFD_GENERIC_FORMAT          0x00000040
#define PFD_NEED_PALETTE            0x00000080
#define PFD_NEED_SYSTEM_PALETTE     0x00000100
#define PFD_SWAP_EXCHANGE           0x00000200
#define PFD_SWAP_COPY               0x00000400
#define PFD_SWAP_LAYER_BUFFERS      0x00000800
#define PFD_GENERIC_ACCELERATED     0x00001000
#define PFD_SUPPORT_DIRECTDRAW      0x00002000
#define PFD_DIRECT3D_ACCELERATED    0x00004000
#define PFD_SUPPORT_COMPOSITION     0x00008000

// PIXELFORMATDESCRIPTOR flags for use in ChoosePixelFormat only
#define PFD_DEPTH_DONTCARE          0x20000000
#define PFD_DOUBLEBUFFER_DONTCARE   0x40000000
#define PFD_STEREO_DONTCARE         0x80000000

FILE* f = nullptr;
HMODULE g_hModule = 0, g_hDLL = 0;

#pragma warning (disable: 4091)
#include <DbgHelp.h>
#pragma warning (default: 4091)
#pragma comment (lib, "DbgHelp.lib")
typedef USHORT(WINAPI *CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
CaptureStackBackTraceType func = (CaptureStackBackTraceType)(GetProcAddress(LoadLibrary(L"kernel32.dll"), "RtlCaptureStackBackTrace"));
LPTOP_LEVEL_EXCEPTION_FILTER PreviousUnhandledExceptionFilter = nullptr;

static void printStack()
{
	FILE* ff = fopen("dump.txt", "a");
	unsigned int   i;
	void*          stack[100];
	unsigned short frames;
	SYMBOL_INFO*   symbol;
	HANDLE         process;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	frames = func(0, 100, stack, NULL);
	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (i = 0; i < frames; i++) {
		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
		fprintf(ff, "%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address);
	}

	free(symbol);
	fclose(ff);
}

LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{
	HANDLE hFile;

	hFile = CreateFile(TEXT("minidump.dmp"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		return EXCEPTION_EXECUTE_HANDLER;

	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = GetCurrentThreadId();
	eInfo.ExceptionPointers = pExInfo;
	eInfo.ClientPointers = FALSE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &eInfo, NULL, NULL);
	CloseHandle(hFile);

	return EXCEPTION_EXECUTE_HANDLER;
}

void start();
void stop();

int WINAPI DllMain(HINSTANCE hDLL, DWORD fdwReason, LPVOID) {
	static_assert(sizeof(v4) == sizeof(float) * 4, "sizeof v4f != 16");
	static_assert(sizeof(m4) == sizeof(float) * 16, "sizeof m4f != 64");

	g_hDLL = hDLL;
	
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: {
		DisableThreadLibraryCalls(g_hDLL);
#		if LOG == 1
		if (!f) {
			f = fopen("opengl32.log", "w");
			fprintf(f, "opengl32.cpp\n");
			fprintf(f, "*PROCESS ATTACH\n");
			fflush(f);
		}
#		endif

		//_set_purecall_handler(PurecallHandler);
		PreviousUnhandledExceptionFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

		if (!g_hModule) {
			g_hModule = LoadLibraryA("gdi32.dll");
			GetDeviceCaps = (PFNGETDEVICECAPSPROC)GetProcAddress(g_hModule, "GetDeviceCaps");
			SetDIBitsToDevice = (PFNSETDIBITSTODEVICEPROC)GetProcAddress(g_hModule, "SetDIBitsToDevice");
			start();
		}
		break; }
	case DLL_PROCESS_DETACH:
		if (PreviousUnhandledExceptionFilter)
			SetUnhandledExceptionFilter(PreviousUnhandledExceptionFilter);

		if (g_hModule) {
			stop();
			FreeLibrary(g_hModule);
			g_hModule = 0;
		}
		if (f) {
			fprintf(f, "*PROCESS DETACH\n");
			fclose(f);
			f = nullptr;
		}
		break;
	case DLL_THREAD_ATTACH:
		//fprintf(f, "*THREAD ATTACH\n");
		break;
	case DLL_THREAD_DETACH:
		//fprintf(f, "*THREAD DETACH\n");
		break;
	}

	return TRUE;
}


#pragma pack(push, 1)
struct color_argb_t {
	u8 b, g, r, a;
};
#pragma pack(pop)

struct gx_align Vertex {
	v4 v;
	t4 t;
	c4 c;
	v4 n; // 4, not 3 for alignment to 16
};

static const size_t TMU_COUNT = 2;

enum TextureInternalFormat {
	TEX_INT_FMT_UNKNOWN = -1, // todo: replace with 0
	TEX_INT_FMT_ALPHA = 0,
	TEX_INT_FMT_INTENSITY,
	TEX_INT_FMT_LUMINANCE,
	TEX_INT_FMT_LUMINANCE_ALPHA,
	TEX_INT_FMT_RGB,
	TEX_INT_FMT_RGBA
};

enum TextureEnvMode {
	TEX_ENV_MODULATE,
	TEX_ENV_REPLACE,
	TEX_ENV_BLEND,
	TEX_ENV_DECAL
};

enum TextureWrapMode {
	TEX_WRAP_CLAMP,
	TEX_WRAP_REPEAT,
	TEX_WRAP_CLAMP_TO_EDGE
};

enum TextureFilterMode {
	TEX_FILTER_NEAREST,
	TEX_FILTER_LINEAR,
	TEX_FILTER_NEAREST_MIPMAP_NEAREST,
	TEX_FILTER_LINEAR_MIPMAP_NEAREST,
	TEX_FILTER_NEAREST_MIPMAP_LINEAR,
	TEX_FILTER_LINEAR_MIPMAP_LINEAR,
};

constexpr f32 frac(f32 x) { return x - i32(x); } // x - floorf(x)

struct alignas(16) Layer {
	void* operator new (size_t sz) { return _aligned_malloc(sz, 16); }
	void operator delete (void* ptr) { _aligned_free(ptr); }

	typedef t4i(Layer::* WrapST)(cref<t4i> t) const;

	static const GLsizei internalComponents_ = 4;

	GLubyte* data_ = nullptr;
	GLsizei width_ = 0;
	GLsizei height_ = 0;
	GLsizei size_ = 0;
	GLsizei stride_ = 0;
	GLsizei align_ = 4; // 4 due to internalComponents=4
	TextureInternalFormat intFmt_ = TEX_INT_FMT_UNKNOWN;
	TextureWrapMode wrapModeS_ = TEX_WRAP_REPEAT, wrapModeT_ = TEX_WRAP_REPEAT;

	WrapST wrapST_ = &Layer::wrapRR; // RR, CC, RC, CR

	__m128i isizes_ = _mm_set1_epi32(0);
	__m128 fsizes_ = _mm_set1_ps(0.f);
	__m128i isizes1_ = _mm_set1_epi32(0);
	__m128 fsizes1_ = _mm_set1_ps(0.f);
	__m128i i1w1w_ = _mm_set1_epi32(0);
	__m128i i01ww1_ = _mm_set1_epi32(0);

	Layer(cref<Layer> l)
		: width_(l.width_)
		, height_(l.height_)
		, size_(l.size_)
		, stride_(l.stride_)
		, align_(l.align_)
		, intFmt_(l.intFmt_)
		, wrapModeS_(l.wrapModeS_)
		, wrapModeT_(l.wrapModeT_)
		, wrapST_(l.wrapST_)
		, isizes_(l.isizes_)
		, fsizes_(l.fsizes_)
		, isizes1_(l.isizes1_)
		, fsizes1_(l.fsizes1_)
		, i1w1w_(l.i1w1w_)
		, i01ww1_(l.i01ww1_)
	{
		data_ = reinterpret_cast<GLubyte*>(_aligned_malloc(size_, 32));
		memcpy(data_, l.data_, l.size_); // todo: mt
	}

	Layer(GLsizei width, GLsizei height, GLenum format, GLenum intFmt, GLsizei unpackAlign, GLbyte const* pixels)
	{
		switch (intFmt) {
		case GL_ALPHA: case GL_ALPHA8: intFmt_ = TEX_INT_FMT_ALPHA; break;
		case GL_INTENSITY: case GL_INTENSITY8: intFmt_ = TEX_INT_FMT_INTENSITY; break;
		case 1: case GL_LUMINANCE: case GL_LUMINANCE8: intFmt_ = TEX_INT_FMT_LUMINANCE; break;
		case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8: intFmt_ = TEX_INT_FMT_LUMINANCE_ALPHA; break;
		case 3: case GL_RGB: case GL_RGB8: intFmt_ = TEX_INT_FMT_RGB; break;
		case 4: case GL_RGBA: case GL_RGBA8: intFmt_ = TEX_INT_FMT_RGBA; break;
		}

		GLsizei components = 0;

		switch (format) {
		case GL_ALPHA: case GL_ALPHA8: components = 1; break;
		case GL_INTENSITY: case GL_INTENSITY8: components = 1; break;
		case 1: case GL_LUMINANCE: case GL_LUMINANCE8: components = 1; break;
		case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8: components = 2; break;
		case 3: case GL_RGB: case GL_RGB8: case GL_BGR_EXT: components = 3; break;
		case 4: case GL_RGBA: case GL_RGBA8: case GL_BGRA_EXT: components = 4; break;
		default: assert(0 && "Unsupported external image format.");
		}

		GLsizei BPP = components * sizeof(__int8);

		width_ = width;
		height_ = height;
		isizes_ = _mm_set_epi32(width, height, width, height);
		fsizes_ = _mm_cvtepi32_ps(isizes_);
		isizes1_ = _mm_sub_epi32(isizes_, _mm_set1_epi32(1));
		fsizes1_ = _mm_cvtepi32_ps(isizes1_);
		i1w1w_ = _mm_set_epi32(1, width, 1, width);
		i01ww1_ = _mm_set_epi32(0, 1, width, width + 1);
		align_ = 4; // internal align: 1, 2, <4>, 8

		GLubyte const* src = nullptr;
		size_t dstAlignDiff = 0;
		size_t srcAlign = unpackAlign, srcStride = width * BPP;
		srcStride = (srcStride + (srcAlign - 1)) & ~(srcAlign - 1);
		size_t srcAlignDiff = srcStride - width * BPP;

		GLsizei dBPP = 4;
		stride_ = (width_ * dBPP + (align_ - 1)) & ~(align_ - 1);
		size_ = stride_ * height_;

		if (data_) _aligned_free(data_);
		data_ = reinterpret_cast<GLubyte*>(_aligned_malloc(size_, 32));
		src = reinterpret_cast<GLubyte const*>(pixels);
		GLubyte* dst = data_;
		dstAlignDiff = stride_ - width_ * dBPP;

		assert(srcAlignDiff == 0);
		assert(dstAlignDiff == 0);

		switch (format) {
		case GL_ALPHA: case GL_ALPHA8: // m:0xaa
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 1, dst += 4) {
					dst[3] = src[0]; dst[0] = dst[1] = dst[2] = 255; // m:0xffffffaa
				}
			}
			break;
		case GL_INTENSITY: case GL_INTENSITY8: // m:0xii
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 1, dst += 4) {
					dst[0] = dst[1] = dst[2] = dst[3] = src[0]; // m:0xiiiiiiii
				}
			}
			break;
		case 1: case GL_LUMINANCE: case GL_LUMINANCE8: // m:0xll
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 1, dst += 4) {
					dst[3] = 255; dst[0] = dst[1] = dst[2] = src[0]; // m:0xllllllff
				}
			}
			break;
		case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8: // m:0xllaa
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 2, dst += 4) {
					dst[3] = src[1]; dst[0] = dst[1] = dst[2] = src[0]; // m:0xllllllaa
				}
			}
			break;
		case 3: case GL_RGB: case GL_RGB8: // m:0xrrggbb
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 3, dst += 4) {
					dst[3] = 255; dst[2] = src[0]; dst[1] = src[1]; dst[0] = src[2]; // m:0xbbggrrff
				}
			}
			break;
		case GL_BGR_EXT: // m:0xbbggrr
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 3, dst += 4) {
					dst[3] = 255; dst[2] = src[2]; dst[1] = src[1]; dst[0] = src[0]; // m:0xbbggrrff
				}
			}
			break;
		case 4: case GL_RGBA: case GL_RGBA8: // m:0xrrggbbaa
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 4, dst += 4) {
					dst[3] = src[3]; dst[2] = src[0]; dst[1] = src[1]; dst[0] = src[2]; // m:0xbbggrraa
				}
			}
			break;
		case GL_BGRA_EXT: // m:0xbbggrraa
			for (GLsizei y = 0; y < height; ++y, src += srcAlignDiff, dst += dstAlignDiff) {
				for (GLsizei x = 0; x<width; ++x, src += 4, dst += 4) {
					dst[3] = src[3]; dst[2] = src[2]; dst[1] = src[1]; dst[0] = src[0]; // m:0xbbggrraa
				}
			}
			break;
		}
	}

	~Layer() {
		if (data_) {
			_aligned_free(data_);
			data_ = nullptr;
		}
	}

	bool update(i32 xoffset, i32 yoffset, i32 width, i32 height, GLenum format, GLsizei unpackAlign, GLubyte const* pixels, uint32_t _texName, i32 _level)
	{
		assert(height_ >= yoffset + height);
		assert(width_ >= xoffset + width);

		GLsizei BPP = 0;

		switch (format) {
		case GL_ALPHA: case GL_ALPHA8: case GL_INTENSITY: case GL_INTENSITY8: case 1: case GL_LUMINANCE: case GL_LUMINANCE8: BPP = 1; break;
		case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8: BPP = 2; break;
		case 3: case GL_RGB: case GL_RGB8: case GL_BGR_EXT: BPP = 3; break;
		case 4: case GL_RGBA: case GL_RGBA8: case GL_BGRA_EXT: BPP = 4; break;
		default: assert(0 && "Unsupported external image format.");
		}

		GLsizei srcAlign = unpackAlign;
		check_enum(srcAlign, 1, 2, 4, 8);
		GLsizei srcStride = (width * BPP + (srcAlign - 1)) & ~(srcAlign - 1), srcSize = srcStride * height;

		GLubyte const* src = nullptr;
		size_t srcAlignDiff = srcStride - width * BPP, dstAlignDiff;
		assert(srcAlignDiff == 0);

		assert(data_);
		src = reinterpret_cast<GLubyte const*>(pixels);
		GLubyte* dst = data_ + (yoffset*width_ + xoffset) * internalComponents_; // always 4 internal components
		GLsizei dBPP = internalComponents_;

		switch (format) {
		case GL_ALPHA: case GL_ALPHA8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 1, dptr += 4) {
					dptr[3] = sptr[0]; dptr[2] = dptr[1] = dptr[0] = 255;
				}
			}
			break;
		case GL_INTENSITY: case GL_INTENSITY8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 1, dptr += 4) {
					dptr[3] = dptr[2] = dptr[1] = dptr[0] = sptr[0];
				}
			}
			break;
		case 1: case GL_LUMINANCE: case GL_LUMINANCE8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 1, dptr += 4) {
					dptr[3] = 255; dptr[2] = dptr[1] = dptr[0] = sptr[0];
				}
			}
			break;
		case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 2, dptr += 4) {
					dptr[3] = sptr[1]; dptr[2] = dptr[1] = dptr[0] = sptr[0];
				}
			}
			break;
		case 3: case GL_RGB: case GL_RGB8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 3, dptr += 4) {
					dptr[3] = 255; dptr[2] = sptr[0]; dptr[1] = sptr[1]; dptr[0] = sptr[2];
				}
			}
			break;
		case GL_BGR_EXT:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 3, dptr += 4) {
					dptr[3] = 255; dptr[2] = sptr[2]; dptr[1] = sptr[1]; dptr[0] = sptr[0];
				}
			}
			break;
		case 4: case GL_RGBA: case GL_RGBA8:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 4, dptr += 4) {
					dptr[3] = sptr[3]; dptr[2] = sptr[0]; dptr[1] = sptr[1]; dptr[0] = sptr[2];
				}
			}
			break;
		case GL_BGRA_EXT:
			for (GLsizei y = 0; y < height; ++y, src += srcStride, dst += width_*internalComponents_) {
				GLubyte const* sptr = src;
				GLubyte* dptr = dst;
				for (GLsizei x = 0; x<width; ++x, sptr += 4, dptr += 4) {
					dptr[3] = sptr[3]; dptr[2] = sptr[2]; dptr[1] = sptr[1]; dptr[0] = sptr[0];
				}
			}
			break;
		}

		return true;
	}

	TextureWrapMode getWrapS() const { return wrapModeS_; }
	TextureWrapMode getWrapT() const { return wrapModeT_; }

	void setWrapS(TextureWrapMode mode) {
		wrapModeS_ = mode;
		wrapST_ = wrapTable_[wrapModeS_][wrapModeT_];
	}

	void setWrapT(TextureWrapMode mode) {
		wrapModeT_ = mode;
		wrapST_ = wrapTable_[wrapModeS_][wrapModeT_];
	}

	t4 wrapCC(cref<t4> t) const {
		return _mm_min_epi32(_mm_max_epi32(t, _mm_setzero_si128()), isizes1_);
	}

	t4i wrapRR(cref<t4i> t) const {
		return _mm_and_si128(t, isizes1_); // t += size for -0.5?_mm_and_si128(_mm_add_epi32(t, isizes_), isizes1_)
	}

	t4 wrapCR(cref<t4> t) const {
		__m128i mm = _mm_set_epi32(0, -1, 0, -1);
		__m128i rr = _mm_and_si128(t, isizes1_);
		__m128i cc = _mm_min_epi32(_mm_max_epi32(t, _mm_setzero_si128()), isizes1_);
		return _mm_or_si128(_mm_and_si128(mm, rr), _mm_andnot_si128(mm, cc));
	}

	t4 wrapRC(cref<t4> t) const {
		__m128i mm = _mm_set_epi32(-1, 0, -1, 0);
		__m128i rr = _mm_and_si128(t, isizes1_);
		__m128i cc = _mm_min_epi32(_mm_max_epi32(t, _mm_setzero_si128()), isizes1_);
		return _mm_or_si128(_mm_and_si128(mm, cc), _mm_andnot_si128(mm, rr));
	}

	c8 mapNearest(cref<t4> st) const {
		t4i iuiv = _s2_mullo_epi32((this->*wrapST_)(_mm_cvttps_epi32(st * fsizes_)), i1w1w_);
		return ((int const*)data_)[_mm_cvtsi128_si32(_mm_add_epi32(iuiv, _mm_shufd_epi32<2, 3, 0, 1>(iuiv)))]; // j * width_ + i
	}

	static __forceinline __m128i _mm_mix_linear_epi16(int c00, int c10, int c01, int c11, int a, int b) {
		static const __m128i w255 = _mm_set1_epi16(255);
		__m128i a4 = _mm_set1_epi16((short&)a), b4 = _mm_set1_epi16((short&)b);
		__m128i c0010 = _mm_unpacklo_epi8(_mm_unpacklo_epi8(_mx_cvtsi32_si128(c00), _mx_cvtsi32_si128(c10)), _mm_setzero_si128());
		__m128i c0111 = _mm_unpacklo_epi8(_mm_unpacklo_epi8(_mx_cvtsi32_si128(c01), _mx_cvtsi32_si128(c11)), _mm_setzero_si128());
		__m128i _f4a4 = _mm_unpacklo_epi16(_mm_sub_epi16(w255, a4), a4), _g4b4 = _mm_unpacklo_epi16(_mm_sub_epi16(w255, b4), b4);
		return _mx_madd_epi16(_mm_unpacklo_epi16(_mx_madd_epi16(c0010, _f4a4), _mx_madd_epi16(c0111, _f4a4)), _g4b4);
	}

	c8 mapLinear(cref<t4> st) const {
		t4 ab = st * fsizes_ - _mm_set1_ps(.5f);
		t4 uv = _mx_floor_ps(ab);
		ab = _mm_sub_ps(ab, uv); // a, b
		uv = _mm_cvttps_epi32(uv); // iu,iv
		uv = _mm_shufd_epi32<1, 0, 1, 0>(uv); // iu,iv,iu,iv
		uv = _mm_add_epi32(uv, _mm_set_epi32(1, 1, 0, 0)); // iu+1,iv+1,iu,iv
		uv = (this->*wrapST_)(uv); // wrapS(iu+1),wrapT(iv+1),wrapS(iu),wrapT(iv)
		uv = _s2_mullo_epi32(uv, i1w1w_); // (wrapS(iu + 1), wrapT(iv + 1)*width, wrapS(iu), wrapT(iv)*width) ~ (i1, j1, i0, j0) [+ (j1,i0,j0,i1)]
		uv = _mm_add_epi32(uv, _mm_shufd_epi32<2, 1, 0, 3>(uv)); // i1+j1, i0+j1, i0+j0, i1+j0
		i32 _c00 = ((int const*)data_)[uv.geti<1>()], _c10 = ((int const*)data_)[uv.geti<0>()];
		i32 _c01 = ((int const*)data_)[uv.geti<2>()], _c11 = ((int const*)data_)[uv.geti<3>()];
		ab = _mm_cvttps_epi32(_mm_mul_ps(ab, _mm_set1_ps(255.f))); // ?,?,ia,ib
		// critical point 2/2
		return _mx_epi16_i32(_mm_mix_linear_epi16(_c00, _c10, _c01, _c11, ab.geti<1>(), ab.geti<0>()));
	}

	static const inline WrapST wrapTable_[2][2]{ &Layer::wrapCC, &Layer::wrapCR, &Layer::wrapRC, &Layer::wrapRR };
};

struct alignas(16) Texture {
	void* operator new (size_t sz) { return _aligned_malloc(sz, 16); }
	void operator delete (void* ptr) { _aligned_free(ptr); }

	typedef c8(Texture::* MapTex)(cref<t4> t, f32 lod) const;
	enum { MaxLayerCount = 32 };
	static const GLsizei internalComponents = 4;

	std::array<std::shared_ptr<Layer>, MaxLayerCount> layers_{ nullptr };

	GLsizei id_ = 0;
	GLsizei width_ = 0, height_ = 0;
	GLfloat filterC_ = .5f;
	GLfloat maxLevel_ = .0f;
	TextureFilterMode minFilter_, magFilter_;
	MapTex mapMin_ = nullptr, mapMag_ = nullptr;

	Texture(cref<Texture> t)
		: layers_(t.layers_)
		, id_(t.id_)
		, width_(t.width_)
		, height_(t.height_)
		, filterC_(t.filterC_)
		, maxLevel_(t.maxLevel_)
		, minFilter_(t.minFilter_)
		, magFilter_(t.magFilter_)
		, mapMin_(t.mapMin_)
		, mapMag_(t.mapMag_)
	{
	}

	Texture(size_t const _id)
		: id_(_id)
		, minFilter_(TEX_FILTER_NEAREST_MIPMAP_LINEAR)//def
		, magFilter_(TEX_FILTER_LINEAR)//def
		, mapMin_(&Texture::mapNearestMipmapLinear)
		, mapMag_(&Texture::mapLinear)
		, filterC_(.5f)
	{ }

	~Texture() {
		for (auto& l : layers_)
			l.reset();
	}
	
	Layer* layer(size_t level = 0) { return layers_[level].get(); }
	
	TextureFilterMode getMinFilter() const { return minFilter_; }
	TextureFilterMode getMagFilter() const { return magFilter_; }
	TextureWrapMode getWrapS() const { return layers_[0]->getWrapS(); }
	TextureWrapMode getWrapT() const { return layers_[0]->getWrapT(); }
	GLsizei width() const { return width_; }
	GLsizei height() const { return height_; }
	GLsizei width(i32 level) const { return layers_[level]->width_; }
	GLsizei height(i32 level) const { return layers_[level]->height_; }
	GLfloat filterFactor() const { return filterC_; }

	void updateMipMapC()
	{
		filterC_ = (magFilter_ == TEX_FILTER_LINEAR &&
			(minFilter_ == TEX_FILTER_NEAREST_MIPMAP_NEAREST || minFilter_ == TEX_FILTER_NEAREST_MIPMAP_LINEAR))
			? .5f : .0f;
	}

	Layer* addLayer(i32 level, i32 width, i32 height, GLenum format, GLenum intFmt, GLsizei unpackAlign, GLbyte const* pixels)
	{
		if (level == 0) {
			width_ = width;
			height_ = height;
		}
		maxLevel_ = (GLfloat)level;
		layers_[level] = std::make_shared<Layer>(width, height, format, intFmt, unpackAlign, pixels);
		return layers_[level].get();
	}

	bool updateLayer(i32 level, i32 xoffset, i32 yoffset, i32 width, i32 height, GLenum format, GLsizei unpackAlign, GLubyte const* pixels, uint32_t _texName)
	{
		Layer* l = layers_[level].get();
		if (!l) return false;
		return l->update(xoffset, yoffset, width, height, format, unpackAlign, pixels, _texName, level);
	}

	void setMinFilter(TextureFilterMode mode) {
		static const MapTex table[] = {
			&Texture::mapNearest,
			&Texture::mapLinear,
			&Texture::mapNearestMipmapNearest,
			&Texture::mapLinearMipmapNearest,
			&Texture::mapNearestMipmapLinear,
			&Texture::mapLinearMipmapLinear
		};

		minFilter_ = mode;
		mapMin_ = table[mode];
		updateMipMapC();
	}

	void setMagFilter(TextureFilterMode mode) {
		static const MapTex table[] = { &Texture::mapNearest, &Texture::mapLinear };
		magFilter_ = mode;
		mapMag_ = table[mode];
		updateMipMapC();
	}

	c8 mapNearest(cref<t4> t, f32) const {
		return layers_[0]->mapNearest(t);
	}

	c8 mapLinear(cref<t4> t, f32) const { // bilinear
		return layers_[0]->mapLinear(t);
	}

	c8 mapNearestMipmapNearest(cref<t4> t, f32 lod) const {
		lod = lod <= maxLevel_ ? lod : maxLevel_;
		return layers_[int(lod)]->mapNearest(t);
	}

	c8 mapNearestMipmapLinear(cref<t4> t, f32 lod) const {
		lod = lod <= maxLevel_ - 1 ? lod : maxLevel_ - 1;
		return _mx_mix_epi16(layers_[int(lod)]->mapNearest(t), layers_[int(lod)+1]->mapNearest(t), _mm_set1_epi16(int(frac(lod)*255.f)));
	}

	c8 mapLinearMipmapNearest(cref<t4> t, f32 lod) const {
		lod = lod <= maxLevel_ ? lod : maxLevel_;
		return layers_[int(lod)]->mapLinear(t);
	}

	c8 mapLinearMipmapLinear(cref<t4> t, f32 lod) const { // trilinear
		lod = lod <= maxLevel_ - 1 ? lod : maxLevel_ - 1;
		return _mx_mix_epi16(layers_[int(lod)]->mapLinear(t), layers_[int(lod)+1]->mapLinear(t), _mm_set1_epi16(int(frac(lod)*255.f)));
	}

	c8 mapTex(cref<t4> t, f32 lod) {
		// use ilod here for filterC cmp? how to calc frac after?
		// get lod from upper level with sse for mtex?
		return (!maxLevel_ || lod <= filterC_) ? (this->*mapMag_)(t, lod) : (this->*mapMin_)(t, lod);
	}

	//i32 ilod(f32 rho2) { return (((int&)rho2)-(127<<23))>>24; } // for mipmap nearest, note: ilod=-1 since flod=-epsilon, important for filterC

	std::shared_ptr<Texture> shared_copy() {
		return std::make_shared<Texture>(*this);
	}

	Layer* makeLayerCopy(i32 const level) {
		layers_[level] = std::make_shared<Layer>(*layers_[level]);
		return layers_[level].get();
	}
};

enum MatrixMode {
	MM_PROJECTION,
	MM_MODELVIEW,
	MM_TEXTURE
};

enum ShapeMode {
	SHAPE_NONE,
	SHAPE_POINTS,
	SHAPE_LINES,
	SHAPE_LINE_STRIP,
	SHAPE_LINE_LOOP,
	SHAPE_TRIANGLES,
	SHAPE_TRIANGLE_STRIP,
	SHAPE_TRIANGLE_FAN,
	SHAPE_QUADS,
	SHAPE_QUAD_STRIP,
	SHAPE_POLYGON
};

enum OpFunc {
	OP_NEVER = GL_NEVER,
	OP_LESS = GL_LESS,
	OP_EQUAL = GL_EQUAL,
	OP_LEQUAL = GL_LEQUAL,
	OP_GREATER = GL_GREATER,
	OP_NOTEQUAL = GL_NOTEQUAL,
	OP_GEQUAL = GL_GEQUAL,
	OP_ALWAYS = GL_ALWAYS
};

enum BlendFactor {
	BLEND_ZERO = 0,
	BLEND_ONE,
	BLEND_SRC_COLOR,
	BLEND_DST_COLOR,
	BLEND_SRC_ALPHA,
	BLEND_DST_ALPHA,
	BLEND_ONE_MINUS_SRC_COLOR,
	BLEND_ONE_MINUS_DST_COLOR,
	BLEND_ONE_MINUS_SRC_ALPHA,
	BLEND_ONE_MINUS_DST_ALPHA,
	BLEND_SRC_ALPHA_SATURATE
};

enum DrawBufferMode {
	FRONT_LEFT_BUFFER = GL_FRONT_LEFT,
	FRONT_RIGHT_BUFFER = GL_FRONT_RIGHT,
	BACK_LEFT_BUFFER = GL_BACK_LEFT,
	BACK_RIGHT_BUFFER = GL_BACK_RIGHT,
	FRONT_BUFFER = GL_FRONT,
	BACK_BUFFER = GL_BACK,
	LEFT_BUFFER = GL_LEFT,
	RIGHT_BUFFER = GL_RIGHT,
	FRONT_AND_BACK_BUFFER = GL_FRONT_AND_BACK,
	AUX0_BUFFER = GL_AUX0,
	AUX1_BUFFER = GL_AUX1,
	AUX2_BUFFER = GL_AUX2,
	AUX3_BUFFER = GL_AUX3
};

enum CullFaceMode {
	CULL_FACE_FRONT = GL_FRONT,
	CULL_FACE_BACK = GL_BACK,
	CULL_FACE_FRONT_AND_BACK = GL_FRONT_AND_BACK
};

enum FrontFaceMode {
	FRONT_FACE_CCW = GL_CCW,
	FRONT_FACE_CW = GL_CW
};

bool __fastcall OpFuncNever   (f32 val, f32 ref) { return false; }
bool __fastcall OpFuncLess    (f32 val, f32 ref) { return val < ref; }
bool __fastcall OpFuncEqual   (f32 val, f32 ref) { return val == ref; }
bool __fastcall OpFuncLEqual  (f32 val, f32 ref) { return val <= ref; }
bool __fastcall OpFuncGreater (f32 val, f32 ref) { return val > ref; }
bool __fastcall OpFuncNotEqual(f32 val, f32 ref) { return val != ref; }
bool __fastcall OpFuncGEqual  (f32 val, f32 ref) { return val >= ref; }
bool __fastcall OpFuncAlways  (f32 val, f32 ref) { return true; }

bool __fastcall OpFuncNeverI   (i16 val, i16 ref) { return false; }
bool __fastcall OpFuncLessI    (i16 val, i16 ref) { return val < ref; }
bool __fastcall OpFuncEqualI   (i16 val, i16 ref) { return val == ref; }
bool __fastcall OpFuncLEqualI  (i16 val, i16 ref) { return val <= ref; }
bool __fastcall OpFuncGreaterI (i16 val, i16 ref) { return val > ref; }
bool __fastcall OpFuncNotEqualI(i16 val, i16 ref) { return val != ref; }
bool __fastcall OpFuncGEqualI  (i16 val, i16 ref) { return val >= ref; }
bool __fastcall OpFuncAlwaysI  (i16 val, i16 ref) { return true; }

typedef bool(__fastcall *OpFuncF) (f32 val, f32 ref);
typedef bool(__fastcall *OpFuncI) (i16 val, i16 ref);

struct alignas(16) State {
	void* operator new (size_t sz) { return _aligned_malloc(sz, 16); }
	void operator delete (void* ptr) { _aligned_free(ptr); }

	MatrixMode modeMatrix;
	ShapeMode modeShape;
	GLsizei vpX, vpY, vpWidth, vpHeight;
	m4 mPVM, mProjection, mModelView, mTexture;
	bool bUpdateTotal;
	std::stack<m4, std::deque<m4, _mm_allocator<m4>>> stackProjection;
	std::stack<m4, std::deque<m4, _mm_allocator<m4>>> stackModelView;
	std::stack<m4, std::deque<m4, _mm_allocator<m4>>> stackTexture;
	c4 clearColor;
	c4 curColor;
	v2 curTexCoord[2];
	v4 curNormal;
	int frame;

	std::vector<v4> vertices;
	std::vector<t4> texcoords;
	std::vector<c4> colors;
	std::vector<v4> normals;

	static const GLint normalArraySize = 3;
	GLint vertexArraySize, colorArraySize, texcoordArraySize; // 3,4; 3,4; 2
	GLenum vertexArrayType, colorArrayType, texcoordArrayType, normalArrayType; // GL_FLOAT; GL_FLOAT, GL_UNSIGNED_BYTE; GL_FLOAT; GL_FLOAT
	GLsizei vertexArrayStride, colorArrayStride, texcoordArrayStride, normalArrayStride;
	GLubyte const *vertexArrayPointer, *colorArrayPointer, *texcoordArrayPointer, *normalArrayPointer;

	GLuint tex2d[TMU_COUNT];
	std::shared_ptr<Texture> tex[TMU_COUNT]; // todo: shared_ptr
	GLuint texActive, clientTexActive;
	TextureFilterMode texMinFilter[2], texMagFilter[2];
	TextureWrapMode texWrapS[2], texWrapT[2];

	bool bAlphaTest;
	bool bBlend;
	bool bCullFace;
	bool bDepthTest;
	bool bStencilTest;
	int bTexture;
	bool bVertexArray;
	bool bColorArray;
	bool bNormalArray;
	int bTextureCoordArray;
	std::set<GLuint> gen_textures_ids;
	std::unordered_map<GLuint, std::shared_ptr<Texture>> textures;
	int drawBuffer;
	f32 projNear, projFar;
	f32 depthNear, depthFar; //glDepthRange, ndc[-1,1] -> [depthNear, depthFar]
	OpFunc depthFunc;
#	ifdef __WBUF__
	OpFuncF depthOpW;
#	else
	OpFuncF depthOpZ;
#	endif
	bool bDepthMask;
	i16 alphaRefI;
	OpFunc alphaFunc;
	OpFuncI alphaOpI;
	BlendFactor sfactor;
	BlendFactor dfactor;
	TextureEnvMode texEnvMode[TMU_COUNT];
	c4 texEnvColor[TMU_COUNT];
	CullFaceMode cullFace;
	FrontFaceMode frontFace;

	size_t pixelPackAlignment;
	size_t pixelUnpackAlignment;

	__m128 ndc2sc1;
	__m128 ndc2sc2;
	__m128 ndc2sc3;
	__m128 ndc2sc4;

	State() {
		reset();
	}

	void reset() {
		vpX = vpY = vpWidth = vpHeight = 0;
		mProjection = m4::iden();
		mModelView = m4::iden();
		mPVM = m4::iden();
		bUpdateTotal = false;
		modeMatrix = MM_PROJECTION;
		modeShape = SHAPE_NONE;
		vertices.reserve(2048);
		normals.reserve(2048);
		texcoords.reserve(2048);
		colors.reserve(2048);
		curColor = c4(1.f, 1.f, 1.f, 1.f); // default
		curTexCoord[0] = v2(0.f, 0.f); // default: (0,0,0,1)
		curTexCoord[1] = v2(0.f, 0.f); // default: (0,0,0,1)
		curNormal = v4(1.f, 1.f, 0.f, 0.f); // default: (0,0,1)
		tex2d[0] = tex2d[1] = 0;
		tex[0] = tex[1] = nullptr;
		texMinFilter[0] = texMinFilter[1] = TEX_FILTER_NEAREST_MIPMAP_LINEAR; // initial
		texMagFilter[0] = texMagFilter[1] = TEX_FILTER_LINEAR; // initial
		texWrapS[0] = texWrapS[1] = TEX_WRAP_REPEAT; // initial:REPEAT
		texWrapT[0] = texWrapT[1] = TEX_WRAP_REPEAT;
		texActive = clientTexActive = 0;
		bAlphaTest = false;
		bBlend = false;
		bCullFace = false;
		bStencilTest = false;
		bTexture = 0;
		bVertexArray = false;
		bColorArray = false;
		bNormalArray = false;
		bTextureCoordArray = false;
		drawBuffer = GL_BACK;
		texEnvMode[0] = texEnvMode[1] = TEX_ENV_MODULATE;
		texEnvColor[0] = texEnvColor[1] = IRGBA(0, 0, 0, 0);
		cullFace = CULL_FACE_BACK;
		frontFace = FRONT_FACE_CCW;

		alphaRefI = 0; // default
		alphaFunc = (OpFunc)GL_ALWAYS;
		alphaOpI = OpFuncAlwaysI;
		sfactor = BLEND_ONE; // default
		dfactor = BLEND_ZERO; // default

		bDepthTest = false;
		depthNear = 0.0f; // default
		depthFar = 1.0f; // default
		depthFunc = OP_LESS;
#		ifdef __WBUF__
		depthOpW = OpFuncGreater;
#		else
		depthOpZ = OpFuncLess;
#		endif
		bDepthMask = true;

		pixelPackAlignment = 4; // default: 4
		pixelUnpackAlignment = 4; // default: 4

		ndc2sc1 = _mm_set_ps(1.f,-1.f,1.f,1.f);
		ndc2sc2 = _mm_set_ps(1.f, 1.f,1.f,0.f);
		ndc2sc4 = _mm_set_ps(0.f, 0.f, depthNear, 0.f);

		vertexArraySize = colorArraySize = texcoordArraySize = 0;
		vertexArrayType = colorArrayType = texcoordArrayType = normalArrayType = 0;
		vertexArrayStride = colorArrayStride = texcoordArrayStride = normalArrayStride = 0;
		vertexArrayPointer = colorArrayPointer = texcoordArrayPointer = normalArrayPointer = 0;

		frame = 0;
	}
};

struct BlendTools {
	static c8 __vectorcall ima(cref<c8> c, cref<c8> t, cref<c8> e) { return c.mul<3>(t); } // (Rf*1,Gf*1,Bf*1,AfAt)
	static c8 __vectorcall imi(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (RfIt,GfIt,BfIt,AfIt)
	static c8 __vectorcall iml(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (RfLt,GfLt,BfLt,Af*1)
	static c8 __vectorcall im2(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (RfLt,GfLt,BfLt,AfAt)
	static c8 __vectorcall im3(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (RfRt,GfGt,BfBt,Af*1)
	static c8 __vectorcall im4(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (RfRt,GfGt,BfBt,AfAt)
	static c8 __vectorcall ira(cref<c8> c, cref<c8> t, cref<c8> e) { return c.mov<3>(t); } // (Rf,Gf,Bf,At)
	static c8 __vectorcall iri(cref<c8> c, cref<c8> t, cref<c8> e) { return t; }           // (It,It,It,It)
	static c8 __vectorcall irl(cref<c8> c, cref<c8> t, cref<c8> e) { return t.mov<3>(c); } // (Lt,Lt,Lt,Af)
	static c8 __vectorcall ir2(cref<c8> c, cref<c8> t, cref<c8> e) { return t; }           // (Lt,Lt,Lt,At)
	static c8 __vectorcall ir3(cref<c8> c, cref<c8> t, cref<c8> e) { return t.mov<3>(c); } // (Rt,Gt,Bt,Af)
	static c8 __vectorcall ir4(cref<c8> c, cref<c8> t, cref<c8> e) { return t; }           // (Rt,Gt,Bt,At)
	static c8 __vectorcall iba(cref<c8> c, cref<c8> t, cref<c8> e) { return c * t; }       // (Rf,Gf,Bf,AfAt)
	static c8 __vectorcall ibi(cref<c8> c, cref<c8> t, cref<c8> e) { return (c * (c8::one - t) + e * t); } // (Rf(1-It)+RcIt,Gf(1-It)+GcIt,Bf(1-It)+BcIt,Af(1-It)+AcIt)
	static c8 __vectorcall ibl(cref<c8> c, cref<c8> t, cref<c8> e) { return (c * (c8::one - t) + e * t).mov<3>(c); } // (Rf(1-Lt)+RcLt,Gf(1-Lt)+GcLt,Bf(1-Lt)+BcLt,Af)
	static c8 __vectorcall ib2(cref<c8> c, cref<c8> t, cref<c8> e) { return (c * (c8::one - t) + e * t).mov<3>(c * t); } // (Rf(1-Lt)+RcLt,Gf(1-Lt)+GcLt,Bf(1-Lt)+BcLt,AfAt)
	static c8 __vectorcall ib3(cref<c8> c, cref<c8> t, cref<c8> e) { return (c * (c8::one - t) + e * t).mov<3>(c); } // (Rf(1-Rt)+RcRt,Gf(1-Gt)+GcGt,Bf(1-Bt)+BcBt,Af)
	static c8 __vectorcall ib4(cref<c8> c, cref<c8> t, cref<c8> e) { return (c * (c8::one - t) + e * t).mov<3>(c * t); } // (Rf(1-Rt)+RcRt,Gf(1-Gt)+GcGt,Bf(1-Bt)+BcBt,AfAt)
	static c8 __vectorcall id3(cref<c8> c, cref<c8> t, cref<c8> e) { return t.mov<3>(c); } // (Rt,Gt,Bt,Af)
	static c8 __vectorcall id4(cref<c8> c, cref<c8> t, cref<c8> e) { return c.mad(c8::one - t.shuf<3>(), t * t.shuf<3>()).mov<3>(c); } // (Rf(1-At)+RtAt,Gf(1-At)+GtAt,Bf(1-At)+BtAt,Af)

	typedef c8(__vectorcall * EnvFmtProc8) (cref<c8> c, cref<c8> t, cref<c8> e);
	static EnvFmtProc8 envFmtTable8[4][6];

	static c8 __vectorcall blendZero8us(cref<c8> s, cref<c8> d) { return _mm_setzero_si128(); } // (0, 0, 0, 0)
	static c8 __vectorcall blendOne8us (cref<c8> s, cref<c8> d) { return c8::one; } //(1, 1, 1, 1)
	static c8 __vectorcall blendSrcColor8us(cref<c8> s, cref<c8> d) { return s; } // (Rs, Gs, Bs, As)
	static c8 __vectorcall blendDstColor8us(cref<c8> s, cref<c8> d) { return d; } // (Rd, Gd, Bd, Ad)
	static c8 __vectorcall blendSrcAlpha8us(cref<c8> s, cref<c8> d) { return s.shuf<3>(); } // (As, As, As, As), _mm_set1_epi16(s.a0)
	static c8 __vectorcall blendDstAlpha8us(cref<c8> s, cref<c8> d) { return d.shuf<3>(); } // (Ad, Ad, Ad, Ad), _mm_set1_epi16(d.a0)
	static c8 __vectorcall blendOneMinusSrcColor8us(cref<c8> s, cref<c8> d) { return _mm_sub_epi16(c8::one, s); } // (1, 1, 1, 1) - (Rs, Gs, Bs, As)
	static c8 __vectorcall blendOneMinusDstColor8us(cref<c8> s, cref<c8> d) { return _mm_sub_epi16(c8::one, d); } // (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
	static c8 __vectorcall blendOneMinusSrcAlpha8us(cref<c8> s, cref<c8> d) { return _mm_sub_epi16(c8::one, s.shuf<3>()); } // (1, 1, 1, 1) - (As, As, As, As)
	static c8 __vectorcall blendOneMinusDstAlpha8us(cref<c8> s, cref<c8> d) { return _mm_sub_epi16(c8::one, d.shuf<3>()); } // (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
	static c8 __vectorcall blendSrcAlphaSaturate8us(cref<c8> s, cref<c8> d) { return s.shuf<3>().min(c8::one - d.shuf<3>()).and(c8::cmask) + c8::amask; } // (f, f, f, 1); f = min(As, 1 - Ad)

	typedef c8(__vectorcall* BlendProc8)(cref<c8> s, cref<c8> d);
	static BlendProc8 blendProcTable8[11];
};

BlendTools::EnvFmtProc8 BlendTools::envFmtTable8[4][6] = {
	{ ima, imi, iml, im2, im3, im4 },
	{ ira, iri, irl, ir2, ir3, ir4 },
	{ iba, ibi, ibl, ib2, ib3, ib4 },
	{   0,   0,   0,   0, id3, id4 } };

BlendTools::BlendProc8 BlendTools::blendProcTable8[11] = {
	blendZero8us,
	blendOne8us,
	blendSrcColor8us,
	blendDstColor8us,
	blendSrcAlpha8us,
	blendDstAlpha8us,
	blendOneMinusSrcColor8us,
	blendOneMinusDstColor8us,
	blendOneMinusSrcAlpha8us,
	blendOneMinusDstAlpha8us,
	blendSrcAlphaSaturate8us
};

const f32 f0_49 = .4999999f;

struct BaryEdge {
	BaryEdge() {}
	BaryEdge(cref<Vertex> _s, cref<Vertex> _e, f32 _y) : s(_s), e(_e) {
		f32 const ddy = (e.v.y > s.v.y) ? (1.f / (e.v.y - s.v.y)) : 0.f;
		x = (dxdy = (e.v.x - s.v.x) * ddy) * (_y - s.v.y) + s.v.x;
	}

	void operator ++ (int) { x += dxdy; }

	Vertex s, e;
	f32 x, dxdy;
};

class gx_align BaryCtx {
public:
	BaryCtx() {}
	~BaryCtx() {}

	v4 IV, JV, FV;
	t4 IT, JT, FT;
	c4 IC, JC, FC;
	t4 wh0wh1;
	c4 texEnvColor[2];
	BaryEdge l, r;
	f32 sy, ey;

	i32* cbuf = nullptr;
#	ifdef __WBUF__
	f32* wbuf = nullptr;
#	else
	f32* zbuf = nullptr;
#	endif
	std::shared_ptr<Texture> tex0, tex1;
	size_t width, height;

	//size_t id, count; job must know // min_y=id*size_y, max_y=min_y+size_y-1, size_y=height/job->count

	TextureEnvMode texEnvMode[2];
	BlendFactor sfactor;
	BlendFactor dfactor;
#	ifdef __WBUF__
	OpFuncF depthOpW;
#	else
	OpFuncF depthOpZ;
#	endif
	OpFuncI alphaOpI;
	i16 alphaRefI;
	bool bDepthTest;
	bool bDepthMask;
	bool bAlphaTest;
	bool bBlend;
};

typedef void (*Proc)(size_t line, size_t lineCount, BaryCtx& ctx);

void DbgString(char* fmt, ...) {
#	ifdef _DEBUG
	char buf[1024];
	va_list lst;
	va_start(lst, fmt);
	vsprintf(buf, fmt, lst);
	va_end(lst);
	OutputDebugStringA(buf);
#	endif
}

struct Job {
	Job()
		: avail_(CreateEvent(0, FALSE, FALSE, 0))
		, done_(CreateEvent(0, TRUE, TRUE, 0))
	 {
		static int i = 0;
		id_ = i++;
	}

	~Job() {
		CloseHandle(avail_);
		CloseHandle(done_);
	}

	void exec() {
		for (; ring_.size(); proc_(id_, count_, ring_.pop()));
	}

	static unsigned int __stdcall proc(void* ptr) {
		for (Job* job = static_cast<Job*>(ptr); true; ) {
			WaitForSingleObject(job->avail_, INFINITE);
			if (job->quit_) { break; }
			job->exec();
			SetEvent(job->done_);
		}
		_endthreadex(0);
		return 0;
	}

	void add(cref<BaryCtx> ctx) {
		int dh = ctx.height / count_;
		int miny = id_ * dh, maxy = miny + dh - 1;
		i32 isy = i32(ctx.sy), iey = i32(ctx.ey + f0_49);
		//if (isy >= iey)
		//	return;
		if (iey < miny)
			return;
		if (isy > maxy)
			return;

		if (ring_.remain()) {
			ring_.push(ctx); // todo: move(uptr)
			ResetEvent(done_);
			SetEvent(avail_);
		}
	}

public:
	size_t id_ = 0, count_ = 0;
	HANDLE handle_ = INVALID_HANDLE_VALUE;
	HANDLE avail_;
	HANDLE done_;
	std::atomic<bool> quit_ = false;
	Proc proc_;
	RingQueue<BaryCtx> ring_;
};

struct Jobs {
	enum : size_t { MaxCount = 32 };

	Jobs() {
		count_ = static_cast<int>(std::thread::hardware_concurrency());
		if (count_ > MaxCount) { count_ = MaxCount; }
		if (!count_) { count_ = 1; }
	}

	~Jobs() {
	}

	void start(Proc f) {
		for (size_t i = 0; i < count_; i++) {
			jobs_[i].handle_ = (HANDLE)_beginthreadex(0, 0, Job::proc, jobs_ + i, CREATE_SUSPENDED, 0);
			if (!jobs_[i].handle_) return;
			//SetThreadPriority(jobs_[i].handle_, THREAD_PRIORITY_HIGHEST);
			SetThreadAffinityMask(jobs_[i].handle_, 1 << (i % count_));
			jobs_[i].count_ = count_;
			jobs_[i].proc_ = f;
		}
		for (size_t i = 0; i < count_; ResumeThread(jobs_[i++].handle_));
	}

	void stop()	{
		for (size_t i = 0; i < count_; i++) { jobs_[i].quit_ = true; SetEvent(jobs_[i].avail_); }
		std::array<HANDLE, MaxCount> handles;
		for (size_t i = 0; i < count_; i++) handles[i] = jobs_[i].handle_;
		WaitForMultipleObjects(count_, handles.data(), TRUE, INFINITE);
		for (size_t i = 0; i < count_; CloseHandle(handles[i++]));
	}

	void wait() {
		std::array<HANDLE, MaxCount> handles;
		for (size_t i = 0; i < count_; i++) handles[i] = jobs_[i].done_;
		WaitForMultipleObjects(count_, handles.data(), TRUE, INFINITE);
	}

	size_t count() const { return count_; }
	Job& operator [](size_t const i) { return jobs_[i]; }

public:
	size_t count_ = 1;
	Job jobs_[MaxCount];
};

void spans(size_t id, size_t count, BaryCtx& ctx) {
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
					v4 const xvx = xv + ctx.IV, xvy = xv + ctx.JV;
					t4 const xtwx = (xt + ctx.IT) / xvx.w, xtwy = (xt + ctx.JT) / xvy.w;
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
};

Jobs jobs;

void start()
{
	jobs.start(spans);
}

void stop()
{
	jobs.stop();
}

struct Buffer {
	u8* cbuf_; // todo: make u32, alloc wh_
	size_t x_, y_, width_, height_, wh_;
	size_t vx_, vy_, vw_, vh_;
	size_t stride_;
	size_t size_;
	size_t bpp_;
	size_t BPP_;
	int minx_, miny_, maxx_, maxy_;
#	ifdef __WBUF__
	f32* wbuf_;
#	else
	f32* zbuf_;
#endif

	__int64 cntZpass = 0ll, cntZcheck = 0ll, cntColor = 0ll;

	void triangle_fabct2_bary_mt(State* state, Vertex* p1, Vertex* p2, Vertex* p3, std::array<std::shared_ptr<Texture>, 2>& texs) {
		BaryCtx ctx;

		ctx.tex0 = texs[0];
		ctx.tex1 = texs[1];
		ctx.cbuf = (i32*)cbuf_;
#		ifdef __WBUF__
		ctx.wbuf = wbuf_;
#		else
		ctx.zbuf = zbuf_;
#		endif
		ctx.width = width_;
		ctx.height = height_;

		ctx.texEnvMode[0] = state->texEnvMode[0];
		ctx.texEnvMode[1] = state->texEnvMode[1];
		ctx.texEnvColor[0] = state->texEnvColor[0];
		ctx.texEnvColor[1] = state->texEnvColor[1];
		ctx.sfactor = state->sfactor;
		ctx.dfactor = state->dfactor;
#		ifdef __WBUF__
		ctx.depthOpW = state->depthOpW;
#		else
		ctx.depthOpZ = state->depthOpZ;
#		endif
		ctx.alphaOpI = state->alphaOpI;
		ctx.alphaRefI = state->alphaRefI;
		ctx.bDepthTest = state->bDepthTest;
		ctx.bDepthMask = state->bDepthMask;
		ctx.bAlphaTest = state->bAlphaTest;
		ctx.bBlend = state->bBlend;

		if (ctx.tex1)
			ctx.wh0wh1 = _mm_set_ps(ctx.tex0->width(), ctx.tex0->height(), ctx.tex1->width(), ctx.tex1->height());
		else if (ctx.tex0)
			ctx.wh0wh1 = _mm_set_ps(ctx.tex0->width(), ctx.tex0->height(), 0.f, 0.f);

		v4 const v21 = p2->v - p1->v, v31 = p3->v - p1->v;
		t4 const t21 = p2->t - p1->t, t31 = p3->t - p1->t;
		c4 const c21 = p2->c - p1->c, c31 = p3->c - p1->c;
		//__m128 S = _mm_set1_ps(1.f / (v31.x*p2->v.y - v31.y*p2->v.x + p1->v.x*p3->v.y - p1->v.y*p3->v.x)); // (v31^p2->v).zzzz() + (p1->v ^ p3->v).zzzz()).rcp()
		//f32 F2 = p1->v.x*p3->v.y - p1->v.y*p3->v.x; // (p1->v ^ p3->v).zzzz()
		//f32 F3 = p2->v.x*p1->v.y - p2->v.y*p1->v.x; // (p2->v ^ p1->v).zzzz()
		__m128 const S = (((v31 ^ p2->v) + (p1->v ^ p3->v)).zzzz()).rcp();
		__m128 const F2 = (p1->v ^ p3->v).zzzz(), F3 = (p2->v ^ p1->v).zzzz();
		ctx.IV = (v31 * v21.y - v21 * v31.y) * S, ctx.JV = (v21 * v31.x - v31 * v21.x) * S, ctx.FV = v21.mad(F2, v31 * F3).mad(S, p1->v);
		ctx.IT = (t31 * v21.y - t21 * v31.y) * S, ctx.JT = (t21 * v31.x - t31 * v21.x) * S, ctx.FT = t21.mad(F2, t31 * F3).mad(S, p1->t);
		ctx.IC = (c31 * v21.y - c21 * v31.y) * S, ctx.JC = (c21 * v31.x - c31 * v21.x) * S, ctx.FC = c21.mad(F2, c31 * F3).mad(S, p1->c);

		if (p1->v.y > p2->v.y) std::swap(p1, p2);
		if (p2->v.y > p3->v.y) std::swap(p2, p3);
		if (p1->v.y > p2->v.y) std::swap(p1, p2);

		f32 y2 = f32(i32(p2->v.y + f0_49)) + .5f;
		BaryEdge e23(*p2, *p3, y2), e43(*p1, *p3, y2);
		if (p2->v.y == p1->v.y) {
			if (p2->v.x > p1->v.x) { //i32 isy = i32(ctx.sy), iey = i32(ctx.ey + f0_49);
				ctx.sy = y2; ctx.ey = p3->v.y;
				ctx.l = e43; ctx.r = e23;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));
			} else {
				ctx.sy = y2; ctx.ey = p3->v.y;
				ctx.l = e23; ctx.r = e43;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));
			}
		} else {
			f32 y1 = f32(i32(p1->v.y + f0_49)) + .5f;
			BaryEdge e12(*p1, *p2, y1), e13(*p1, *p3, y1);
			if (e12.dxdy > e13.dxdy) {
				ctx.sy = y2; ctx.ey = p3->v.y;
				ctx.l = e43; ctx.r = e23;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));

				ctx.sy = y1; ctx.ey = p2->v.y;
				ctx.l = e13; ctx.r = e12;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));
			} else {
				ctx.sy = y2; ctx.ey = p3->v.y;
				ctx.l = e23; ctx.r = e43;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));

				ctx.sy = y1; ctx.ey = p2->v.y;
				ctx.l = e12; ctx.r = e13;
				for (size_t i = 0; i < jobs.count(); jobs[i++].add(ctx));
			}
		}
	}

	Buffer()
		: cbuf_(nullptr)
#		ifdef __WBUF__
		, wbuf_(nullptr)
#		else
		, zbuf_(nullptr)
#		endif
	{
	}

	~Buffer() {
		if (cbuf_) {
			_aligned_free(cbuf_);
			cbuf_ = nullptr;
		}
#		ifdef __WBUF__
		if (wbuf_) {
			_aligned_free(wbuf_);
			wbuf_ = nullptr;
		}
#		else
		if (zbuf_) {
			_aligned_free(zbuf_);
			zbuf_ = nullptr;
		}
#		endif
	}

	__forceinline void __fastcall plot(int x, int y, u32 c) {
		if ((c & 0xff000000) != 0xff000000)
			return;

		size_t pos = stride_*y + x*BPP_;
		//assert(pos>=0 && pos+4<=size);
		if (pos >= 0 && pos + 4 <= size_) {
			0[(i32*)(cbuf_ + stride_ * y) + x] = c;
		}
	}

	void line(i32 x0, i32 y0, const i32 x1, const i32 y1, const i32 c) {
		if (0<=x0 && x0<(i32)width_ && 0<=x1 && x1<(i32)width_
			&& 0<=y0 && y0<(i32)height_ && 0<=y1 && y1<(i32)height_)
		{
			i32 dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
			i32 dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
			i32 err = dx+dy; // error value e_xy

			plot(x0, y0, c);
			for (i32 e2=err<<1; ; e2=err<<1, plot(x0, y0, c)) {
				if (x0==x1 && y0==y1) break;
				if (e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
				if (e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
			}
		}
	}

	void process_tri(State* state, int i0, int i1, int i2) {
		// SSE3 due to hsub
		//if (state->tex2d[0] != 1) return;
		if (!vw_ || !vh_) return;
		//vertices contains pvm*v = clip space
		v4 &p0 = state->vertices[i0], &p1 = state->vertices[i1], &p2 = state->vertices[i2];
		if (p0 == p1 || p1 == p2 || p2 == p0) return;
		if (p0.w > state->projFar && p1.w > state->projFar && p2.w > state->projFar) return;

		//eye->PVM->clip->/W=eyeZ->ndc->->screen
		// cut w to znear
		int vi, vo, co = 0;
		if (p0.w < state->projNear) { vo = 0; co++; } else { vi = 0; }
		if (p1.w < state->projNear) { vo = 1; co++; } else { vi = 1; }
		if (p2.w < state->projNear) { vo = 2; co++; } else { vi = 2; }

		if (co == 3)
			return;

		Vertex vtxs[6];
		vtxs[0].v = p0;
		vtxs[0].t = state->texcoords[i0];
		vtxs[0].c = state->colors[i0];
		vtxs[1].v = p1;
		vtxs[1].t = state->texcoords[i1];
		vtxs[1].c = state->colors[i1];
		vtxs[2].v = p2;
		vtxs[2].t = state->texcoords[i2];
		vtxs[2].c = state->colors[i2];

		static const int mod3[] = { 0, 1, 2, 0, 1, 2 };
		std::vector<Vertex, _mm_allocator<Vertex>> res;

		if (co == 0) {
			for (size_t i = 3; i--; ) {
				//assert(_nequal(vtxs[i].v.w, 0.f));
				__m128 const w4 = vtxs[i].v.wwww().rcp();
				vtxs[i].v = w4 * vtxs[i].v.mov<0>(1.f);
				vtxs[i].t = w4 * vtxs[i].t;
			}
			clip_tri_ndc(vtxs, res);
		} else if (co == 1) {
			// 2 tri
			Vertex v3;
			Vertex& v0 = vtxs[vo]; // v
			cref<Vertex> v1 = vtxs[mod3[(vo + 1)]]; // u
			cref<Vertex> v2 = vtxs[mod3[(vo + 2)]]; // u
			__m128 const f4 = _mm_set1_ps((state->projNear - v0.v.w) / (v1.v.w - v0.v.w));
			__m128 const g4 = _mm_set1_ps((state->projNear - v0.v.w) / (v2.v.w - v0.v.w));
			v3.v = (v2.v - v0.v).mad(g4, v0.v);
			v3.t = (v2.t - v0.t).mad(g4, v0.t);
			v3.c = (v2.c - v0.c).mad(g4, v0.c);
			v0.v = (v1.v - v0.v).mad(f4, v0.v);
			v0.t = (v1.t - v0.t).mad(f4, v0.t);
			v0.c = (v1.c - v0.c).mad(f4, v0.c);
			//assert(_equal(v3.v.w, state->projNear, 1.e-4f));
			//assert(_equal(v0.v.w, state->projNear, 1.e-4f));
			vtxs[3] = v3;
			for (size_t i = 4; i--; ) {
				__m128 const w4 = vtxs[i].v.wwww().rcp();
				vtxs[i].v = w4 * vtxs[i].v.mov<0>(1.f);
				vtxs[i].t = w4 * vtxs[i].t;
			}
			vtxs[4] = v0;
			vtxs[5] = v2;
			clip_tri_ndc(vtxs, res);
			clip_tri_ndc(vtxs+3, res);
		} else if (co == 2) {
			// 1 tri
			cref<Vertex> v0 = vtxs[vi]; // u
			Vertex& v1 = vtxs[mod3[(vi + 1)]]; // v
			Vertex& v2 = vtxs[mod3[(vi + 2)]]; // v
			__m128 const f4 = _mm_set1_ps((state->projNear - v1.v.w) / (v0.v.w - v1.v.w));
			__m128 const g4 = _mm_set1_ps((state->projNear - v2.v.w) / (v0.v.w - v2.v.w));
			v1.v = (v0.v - v1.v).mad(f4, v1.v);
			v1.t = (v0.t - v1.t).mad(f4, v1.t);
			v1.c = (v0.c - v1.c).mad(f4, v1.c);
			v2.v = (v0.v - v2.v).mad(g4, v2.v);
			v2.t = (v0.t - v2.t).mad(g4, v2.t);
			v2.c = (v0.c - v2.c).mad(g4, v2.c);
			//assert(_equal(v1.v.w, state->projNear, 1.e-4f));
			//assert(_equal(v2.v.w, state->projNear, 1.e-4f));
			for (size_t i = 3; i--; ) {
				__m128 const w4 = vtxs[i].v.wwww().rcp();
				vtxs[i].v = w4 * vtxs[i].v.mov<0>(1.f);
				vtxs[i].t = w4 * vtxs[i].t;
			}
			clip_tri_ndc(vtxs, res);
		}

		//assert(res.size() % 3 == 0);

		for (auto& p : res) {
			p.v = _mm_madd_ps(_mm_madd_ps(p.v, state->ndc2sc1, state->ndc2sc2), state->ndc2sc3, state->ndc2sc4);
			//assert(state->depthNear < state->depthFar
			//	? _gequal(p.v.z, state->depthNear) && _lequal(p.v.z, state->depthFar)
			//	: _gequal(p.v.z, state->depthFar) && _lequal(p.v.z, state->depthNear));
			//p.v = (p.v * state->ndc2sc1 + state->ndc2sc2) * state->ndc2sc3 + state->ndc2sc4;
			//p.v.x = ( p.v.x + 1.f) * .5f * (vw_-1);//
			//p.v.y = (-p.v.y + 1.f) * .5f * (vh_-1);//
			//p.v.z = ( p.v.z + 1.f) * .5f * (depthFar - depthNear) + depthNear;
			//p.v.w = ( p.v.w + 0.f) * 1.f * 1.f;
		}

		for (size_t i = 0, j = 0; i < res.size(); i += 3, j++) {
			if (state->bCullFace) {
				if (state->cullFace == CULL_FACE_FRONT_AND_BACK)
					continue;
				v4 u = _mm_sub_ps(res[i + 1].v, res[i].v);
				v4 v = _mm_sub_ps(res[i + 2].v, res[i].v);
				//v4 r = _mm_mul_ps(v, _mm_shuffle_ps(u, u, _MM_SHUFFLE(2,3,1,0)));
				//float z_ = _mm_hsub_ps(r, r).m128_f32[1];
				float z = u.x * v.y - u.y * v.x;
				if (state->frontFace == FRONT_FACE_CCW) {
					if (state->cullFace == CULL_FACE_BACK) {
						if (z > 0.f)
							continue;
					} else {
						if (z < 0.f)
							continue;
					}
				} else {
					if (state->cullFace == CULL_FACE_BACK) {
						if (z < 0.f)
							continue;
					} else {
						if (z > 0.f)
							continue;
					}
				}
			}
			std::array<std::shared_ptr<Texture>, 2> texs {
				((state->bTexture & (1 << 0)) != 0) ? state->textures[state->tex2d[0]] : std::shared_ptr<Texture>(),
				((state->bTexture & (1 << 1)) != 0) ? state->textures[state->tex2d[1]] : std::shared_ptr<Texture>()
			};

			triangle_fabct2_bary_mt(state, &res[i], &res[i + 1], &res[i + 2], texs);
		}
	}

	static bool __fastcall lt(f32 a, f32 b) { return _less(a, b, 1e-4f); }
	static bool __fastcall gt(f32 a, f32 b) { return _greater(a, b, 1e-4f); }

	void clip_tri_ndc(Vertex* vtxs, std::vector<Vertex, _mm_allocator<Vertex>>& res, int iplane = 0)
	{
		// clip all in clip space (not ndc)?
		// then: v.t = v.t / v.v.w where v.v.w = 1/z

		//if (vtxs[0].v == vtxs[1].v || vtxs[1].v == vtxs[2].v || vtxs[2].v == vtxs[0].v)
		//	return;

		if (iplane == 5) {
			res.emplace_back(vtxs[0]);
			res.emplace_back(vtxs[1]);
			res.emplace_back(vtxs[2]);
#			ifndef NDEBUG
			for (int i = 3; i--; ) {
				//assert(_gequal(vtxs[i].v.x, -1.f, 1.e-4f) && _lequal(vtxs[i].v.x, 1.f, 1.e-4f));
				//assert(_gequal(vtxs[i].v.y, -1.f, 1.e-4f) && _lequal(vtxs[i].v.y, 1.f, 1.e-4f));
				//assert(_gequal(vtxs[i].v.z, -1.f, 1.e-4f) && _lequal(vtxs[i].v.z, 1.f, 1.e-4f));
			}
#			endif
			return;
		}

		typedef bool (__fastcall*C)(f32, f32);
		struct P { i32 i; f32 w; C c; };
		static P const ps[5] = {{3,-1.f,lt},{3,1.f,gt},{2,-1.f,lt},{2,1.f,gt},{1,1.f,gt}}; //LRBTF
		cref<P> p = ps[iplane];

		i32 vi, vo, co = 0;
		if (p.c(vtxs[0].v[p.i], p.w)) { vo = 0; co++; } else { vi = 0; }
		if (p.c(vtxs[1].v[p.i], p.w)) { vo = 1; co++; } else { vi = 1; }
		if (p.c(vtxs[2].v[p.i], p.w)) { vo = 2; co++; } else { vi = 2; }

		//f32 f = (-1.f - ndc.v.x) / (ndc.u.x - ndc.v.x);
		//ndc.v += f * (ndc.u - ndc.v);
		//t' = t / v.w

		static const i32 mod3[6] = { 0,1,2,0,1,2 };
		switch (co) {
		case 0:
			clip_tri_ndc(vtxs, res, iplane + 1);
			break;
		case 1: { // 2 tri
			Vertex v3;
			Vertex& v0 = vtxs[vo]; // v
			cref<Vertex> v1 = vtxs[mod3[(vo + 1)]]; // u
			cref<Vertex> v2 = vtxs[mod3[(vo + 2)]]; // u
	        __m128 const f4 = _mm_set1_ps((p.w - v0.v[p.i]) / (v1.v[p.i] - v0.v[p.i]));
	        __m128 const g4 = _mm_set1_ps((p.w - v0.v[p.i]) / (v2.v[p.i] - v0.v[p.i]));
			v3.v = (v2.v - v0.v).mad(g4, v0.v); //v3.v = v0.v + g * (v2.v - v0.v);
			v3.t = (v2.t - v0.t).mad(g4, v0.t);
			v3.c = (v2.c - v0.c).mad(g4, v0.c);
			v0.v = (v1.v - v0.v).mad(f4, v0.v); //v0.v += f * (v1.v - v0.v);
			v0.t = (v1.t - v0.t).mad(f4, v0.t);
			v0.c = (v1.c - v0.c).mad(f4, v0.c);
			//assert(_equal(v3.v[p.i], p.w, 1.e-4f));
			//assert(_equal(v0.v[p.i], p.w, 1.e-4f));
	        Vertex vtxs2[3] = { v0, v2, v3 };
			clip_tri_ndc(vtxs, res, iplane + 1);
			clip_tri_ndc(vtxs2, res, iplane + 1);
			} break;
		case 2: { // 1 tri
			cref<Vertex> v0 = vtxs[vi]; // u
			Vertex& v1 = vtxs[mod3[(vi + 1)]]; // v
			Vertex& v2 = vtxs[mod3[(vi + 2)]]; // v
			__m128 const f4 = _mm_set1_ps((p.w - v1.v[p.i]) / (v0.v[p.i] - v1.v[p.i]));
			__m128 const g4 = _mm_set1_ps((p.w - v2.v[p.i]) / (v0.v[p.i] - v2.v[p.i]));
			v1.v = (v0.v - v1.v).mad(f4, v1.v); //v1.v += f * (v0.v - v1.v);
			v1.t = (v0.t - v1.t).mad(f4, v1.t);
			v1.c = (v0.c - v1.c).mad(f4, v1.c);
			v2.v = (v0.v - v2.v).mad(g4, v2.v); //v2.v += g * (v0.v - v2.v);
			v2.t = (v0.t - v2.t).mad(g4, v2.t);
			v2.c = (v0.c - v2.c).mad(g4, v2.c);
			//assert(_equal(v1.v[p.i], p.w, 1.e-4f));
			//assert(_equal(v2.v[p.i], p.w, 1.e-4f));
			clip_tri_ndc(vtxs, res, iplane + 1);
			} break;
		}
	}
};

struct Context {
	BITMAPINFO bmi;
	Buffer buf;
	State* state = nullptr;
	Context() : state(new State) {}
	~Context() { delete state; }
};

const size_t HRC_BASE = 0x10000;
const size_t MAX_CONTEXT_COUNT = 32;
std::map<HGLRC, std::unique_ptr<Context>> contexts;

HDC hdc = 0;
HGLRC hrc = 0;
Context* ctx = nullptr;

void HandleError() {
	i32 const error = GetLastError();
	TCHAR buffer[1024] = {_T('\0')};
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer)/sizeof(TCHAR)-1, 0);
	MessageBox(0, buffer, TEXT("Error"), MB_OK);
	exit(error);
}

#define MAP_LIST_ELEM(E) (E, #E)

static const std::map<int, const char*> g_EnableList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(GL_ALPHA_TEST)
	MAP_LIST_ELEM(GL_AUTO_NORMAL)
	MAP_LIST_ELEM(GL_BLEND)
	MAP_LIST_ELEM(GL_CLIP_PLANE0)
	MAP_LIST_ELEM(GL_CLIP_PLANE1)
	MAP_LIST_ELEM(GL_CLIP_PLANE2)
	MAP_LIST_ELEM(GL_CLIP_PLANE3)
	MAP_LIST_ELEM(GL_CLIP_PLANE4)
	MAP_LIST_ELEM(GL_CLIP_PLANE5)
	MAP_LIST_ELEM(GL_COLOR_LOGIC_OP)
	MAP_LIST_ELEM(GL_COLOR_MATERIAL)
	MAP_LIST_ELEM(GL_CULL_FACE)
	MAP_LIST_ELEM(GL_DEPTH_TEST)
	MAP_LIST_ELEM(GL_DITHER)
	MAP_LIST_ELEM(GL_FOG)
	MAP_LIST_ELEM(GL_INDEX_LOGIC_OP)
	MAP_LIST_ELEM(GL_LIGHT0)
	MAP_LIST_ELEM(GL_LIGHT1)
	MAP_LIST_ELEM(GL_LIGHT2)
	MAP_LIST_ELEM(GL_LIGHT3)
	MAP_LIST_ELEM(GL_LIGHT4)
	MAP_LIST_ELEM(GL_LIGHT5)
	MAP_LIST_ELEM(GL_LIGHT6)
	MAP_LIST_ELEM(GL_LIGHT7)
	MAP_LIST_ELEM(GL_LIGHTING)
	MAP_LIST_ELEM(GL_LINE_SMOOTH)
	MAP_LIST_ELEM(GL_LINE_STIPPLE)
	MAP_LIST_ELEM(GL_LOGIC_OP)
	MAP_LIST_ELEM(GL_MAP1_COLOR_4)
	MAP_LIST_ELEM(GL_MAP1_INDEX)
	MAP_LIST_ELEM(GL_MAP1_NORMAL)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_1)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_2)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_3)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_4)
	MAP_LIST_ELEM(GL_MAP1_VERTEX_3)
	MAP_LIST_ELEM(GL_MAP1_VERTEX_4)
	MAP_LIST_ELEM(GL_MAP2_COLOR_4)
	MAP_LIST_ELEM(GL_MAP2_INDEX)
	MAP_LIST_ELEM(GL_MAP2_NORMAL)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_1)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_2)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_3)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_4)
	MAP_LIST_ELEM(GL_MAP2_VERTEX_3)
	MAP_LIST_ELEM(GL_MAP2_VERTEX_4)
	MAP_LIST_ELEM(GL_NORMALIZE)
	MAP_LIST_ELEM(GL_POINT_SMOOTH)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_FILL)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_LINE)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_POINT)
	MAP_LIST_ELEM(GL_POLYGON_SMOOTH)
	MAP_LIST_ELEM(GL_POLYGON_STIPPLE)
	MAP_LIST_ELEM(GL_SCISSOR_TEST)
	MAP_LIST_ELEM(GL_STENCIL_TEST)
	MAP_LIST_ELEM(GL_TEXTURE_1D)
	MAP_LIST_ELEM(GL_TEXTURE_2D)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_Q)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_R)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_S)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_T);

static const std::map<int, const char*> g_EnableClientList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(GL_COLOR_ARRAY)
	MAP_LIST_ELEM(GL_EDGE_FLAG_ARRAY)
	MAP_LIST_ELEM(GL_INDEX_ARRAY)
	MAP_LIST_ELEM(GL_NORMAL_ARRAY)
	MAP_LIST_ELEM(GL_TEXTURE_COORD_ARRAY)
	MAP_LIST_ELEM(GL_VERTEX_ARRAY);

static const std::map<int, const char*> g_StencilOpList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(GL_KEEP)
	MAP_LIST_ELEM(GL_ZERO)
	MAP_LIST_ELEM(GL_REPLACE)
	MAP_LIST_ELEM(GL_INCR)
	MAP_LIST_ELEM(GL_DECR)
	MAP_LIST_ELEM(GL_INVERT);

static const std::map<int, const char*> g_TexInternalFormatList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(1)
	MAP_LIST_ELEM(2)
	MAP_LIST_ELEM(3)
	MAP_LIST_ELEM(4)
	MAP_LIST_ELEM(GL_ALPHA)
	MAP_LIST_ELEM(GL_ALPHA4)
	MAP_LIST_ELEM(GL_ALPHA8)
	MAP_LIST_ELEM(GL_ALPHA12)
	MAP_LIST_ELEM(GL_ALPHA16)
	MAP_LIST_ELEM(GL_LUMINANCE)
	MAP_LIST_ELEM(GL_LUMINANCE4)
	MAP_LIST_ELEM(GL_LUMINANCE8)
	MAP_LIST_ELEM(GL_LUMINANCE12)
	MAP_LIST_ELEM(GL_LUMINANCE16)
	MAP_LIST_ELEM(GL_LUMINANCE_ALPHA)
	MAP_LIST_ELEM(GL_LUMINANCE4_ALPHA4)
	MAP_LIST_ELEM(GL_LUMINANCE6_ALPHA2)
	MAP_LIST_ELEM(GL_LUMINANCE8_ALPHA8)
	MAP_LIST_ELEM(GL_LUMINANCE12_ALPHA4)
	MAP_LIST_ELEM(GL_LUMINANCE12_ALPHA12)
	MAP_LIST_ELEM(GL_LUMINANCE16_ALPHA16)
	MAP_LIST_ELEM(GL_INTENSITY)
	MAP_LIST_ELEM(GL_INTENSITY4)
	MAP_LIST_ELEM(GL_INTENSITY8)
	MAP_LIST_ELEM(GL_INTENSITY12)
	MAP_LIST_ELEM(GL_INTENSITY16)
	MAP_LIST_ELEM(GL_R3_G3_B2)
	MAP_LIST_ELEM(GL_RGB)
	MAP_LIST_ELEM(GL_RGB4)
	MAP_LIST_ELEM(GL_RGB5)
	MAP_LIST_ELEM(GL_RGB8)
	MAP_LIST_ELEM(GL_RGB10)
	MAP_LIST_ELEM(GL_RGB12)
	MAP_LIST_ELEM(GL_RGB16)
	MAP_LIST_ELEM(GL_RGBA)
	MAP_LIST_ELEM(GL_RGBA2)
	MAP_LIST_ELEM(GL_RGBA4)
	MAP_LIST_ELEM(GL_RGB5_A1)
	MAP_LIST_ELEM(GL_RGBA8)
	MAP_LIST_ELEM(GL_RGB10_A2)
	MAP_LIST_ELEM(GL_RGBA12)
	MAP_LIST_ELEM(GL_RGBA16);

const char* getTexFormatString(GLenum format) {
	static const char* values[] = {
		"GL_ALPHA4", "GL_ALPHA8", "GL_ALPHA12", "GL_ALPHA16",
		"GL_LUMINANCE4", "GL_LUMINANCE8", "GL_LUMINANCE12", "GL_LUMINANCE16",
		"GL_LUMINANCE4_ALPHA4", "GL_LUMINANCE6_ALPHA2", "GL_LUMINANCE8_ALPHA8", "GL_LUMINANCE12_ALPHA4", "GL_LUMINANCE12_ALPHA12", "GL_LUMINANCE16_ALPHA16",
		"GL_INTENSITY", "GL_INTENSITY4", "GL_INTENSITY8", "GL_INTENSITY12", "GL_INTENSITY16",
		"GL_UNKNOWN",
		"GL_RGB4", "GL_RGB5", "GL_RGB8", "GL_RGB10", "GL_RGB12", "GL_RGB16", "GL_RGBA2", "GL_RGBA4", "GL_RGB5_A1", "GL_RGBA8", "GL_RGB10_A2", "GL_RGBA12", "GL_RGBA16",
		"GL_TEXTURE_RED_SIZE", "GL_TEXTURE_GREEN_SIZE", "GL_TEXTURE_BLUE_SIZE", "GL_TEXTURE_ALPHA_SIZE", "GL_TEXTURE_LUMINANCE_SIZE", "GL_TEXTURE_INTENSITY_SIZE",
		"GL_PROXY_TEXTURE_1D", "GL_PROXY_TEXTURE_2D"
	};

	switch (format) {
	case 1: return "1";
	case 2: return "2";
	case 3: return "3";
	case 4: return "4";
	}

	if (format>=GL_ALPHA4 && format<=GL_PROXY_TEXTURE_2D) {
		return values[format-GL_ALPHA4];
	}

	if (format == GL_R3_G3_B2)
		return "GL_R3_G3_B2";

	return "GL_UNKNOWN";
}

static const std::map<int, const char*> g_TexFormatList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(GL_COLOR_INDEX)
	MAP_LIST_ELEM(GL_RED)
	MAP_LIST_ELEM(GL_GREEN)
	MAP_LIST_ELEM(GL_BLUE)
	MAP_LIST_ELEM(GL_ALPHA)
	MAP_LIST_ELEM(GL_RGB)
	MAP_LIST_ELEM(GL_RGBA)
	MAP_LIST_ELEM(GL_BGR_EXT)
	MAP_LIST_ELEM(GL_BGRA_EXT)
	MAP_LIST_ELEM(GL_LUMINANCE)
	MAP_LIST_ELEM(GL_LUMINANCE_ALPHA);

static const std::map<int, const char*> g_HintList = helper::map_list_of<int, const char*>
	MAP_LIST_ELEM(GL_ACCUM_ALPHA_BITS)
	MAP_LIST_ELEM(GL_ACCUM_BLUE_BITS)
	MAP_LIST_ELEM(GL_ACCUM_CLEAR_VALUE)
	MAP_LIST_ELEM(GL_ACCUM_GREEN_BITS)
	MAP_LIST_ELEM(GL_ACCUM_RED_BITS)
	MAP_LIST_ELEM(GL_ALPHA_BIAS)
	MAP_LIST_ELEM(GL_ALPHA_BITS)
	MAP_LIST_ELEM(GL_ALPHA_SCALE)
	MAP_LIST_ELEM(GL_ALPHA_TEST)
	MAP_LIST_ELEM(GL_ALPHA_TEST_FUNC)
	MAP_LIST_ELEM(GL_ALPHA_TEST_REF)
	MAP_LIST_ELEM(GL_ATTRIB_STACK_DEPTH)
	MAP_LIST_ELEM(GL_AUTO_NORMAL)
	MAP_LIST_ELEM(GL_AUX_BUFFERS)
	MAP_LIST_ELEM(GL_BLEND)
	MAP_LIST_ELEM(GL_BLEND_DST)
	MAP_LIST_ELEM(GL_BLEND_SRC)
	MAP_LIST_ELEM(GL_BLUE_BIAS)
	MAP_LIST_ELEM(GL_BLUE_BITS)
	MAP_LIST_ELEM(GL_BLUE_SCALE)
	MAP_LIST_ELEM(GL_CLIENT_ATTRIB_STACK_DEPTH)
	MAP_LIST_ELEM(GL_CLIP_PLANE0)
	MAP_LIST_ELEM(GL_CLIP_PLANE1)
	MAP_LIST_ELEM(GL_CLIP_PLANE2)
	MAP_LIST_ELEM(GL_CLIP_PLANE3)
	MAP_LIST_ELEM(GL_CLIP_PLANE4)
	MAP_LIST_ELEM(GL_CLIP_PLANE5)
	MAP_LIST_ELEM(GL_COLOR_ARRAY)
	MAP_LIST_ELEM(GL_COLOR_ARRAY_SIZE)
	MAP_LIST_ELEM(GL_COLOR_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_COLOR_ARRAY_TYPE)
	MAP_LIST_ELEM(GL_COLOR_CLEAR_VALUE)
	MAP_LIST_ELEM(GL_COLOR_LOGIC_OP)
	MAP_LIST_ELEM(GL_COLOR_MATERIAL)
	MAP_LIST_ELEM(GL_COLOR_MATERIAL_FACE)
	MAP_LIST_ELEM(GL_COLOR_MATERIAL_PARAMETER)
	MAP_LIST_ELEM(GL_COLOR_WRITEMASK)
	MAP_LIST_ELEM(GL_CULL_FACE)
	MAP_LIST_ELEM(GL_CULL_FACE_MODE)
	MAP_LIST_ELEM(GL_CURRENT_COLOR)
	MAP_LIST_ELEM(GL_CURRENT_INDEX)
	MAP_LIST_ELEM(GL_CURRENT_NORMAL)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_COLOR)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_DISTANCE)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_INDEX)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_POSITION)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_POSITION_VALID)
	MAP_LIST_ELEM(GL_CURRENT_RASTER_TEXTURE_COORDS)
	MAP_LIST_ELEM(GL_CURRENT_TEXTURE_COORDS)
	MAP_LIST_ELEM(GL_DEPTH_BIAS)
	MAP_LIST_ELEM(GL_DEPTH_BITS)
	MAP_LIST_ELEM(GL_DEPTH_CLEAR_VALUE)
	MAP_LIST_ELEM(GL_DEPTH_FUNC)
	MAP_LIST_ELEM(GL_DEPTH_RANGE)
	MAP_LIST_ELEM(GL_DEPTH_SCALE)
	MAP_LIST_ELEM(GL_DEPTH_TEST)
	MAP_LIST_ELEM(GL_DEPTH_WRITEMASK)
	MAP_LIST_ELEM(GL_DITHER)
	MAP_LIST_ELEM(GL_DOUBLEBUFFER)
	MAP_LIST_ELEM(GL_DRAW_BUFFER)
	MAP_LIST_ELEM(GL_EDGE_FLAG)
	MAP_LIST_ELEM(GL_EDGE_FLAG_ARRAY)
	MAP_LIST_ELEM(GL_EDGE_FLAG_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_FOG)
	MAP_LIST_ELEM(GL_FOG_COLOR)
	MAP_LIST_ELEM(GL_FOG_DENSITY)
	MAP_LIST_ELEM(GL_FOG_END)
	MAP_LIST_ELEM(GL_FOG_HINT)
	MAP_LIST_ELEM(GL_FOG_INDEX)
	MAP_LIST_ELEM(GL_FOG_MODE)
	MAP_LIST_ELEM(GL_FOG_START)
	MAP_LIST_ELEM(GL_FRONT_FACE)
	MAP_LIST_ELEM(GL_GREEN_BIAS)
	MAP_LIST_ELEM(GL_GREEN_BITS)
	MAP_LIST_ELEM(GL_GREEN_SCALE)
	MAP_LIST_ELEM(GL_INDEX_ARRAY)
	MAP_LIST_ELEM(GL_INDEX_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_INDEX_ARRAY_TYPE)
	MAP_LIST_ELEM(GL_INDEX_BITS)
	MAP_LIST_ELEM(GL_INDEX_CLEAR_VALUE)
	MAP_LIST_ELEM(GL_INDEX_LOGIC_OP)
	MAP_LIST_ELEM(GL_INDEX_MODE)
	MAP_LIST_ELEM(GL_INDEX_OFFSET)
	MAP_LIST_ELEM(GL_INDEX_SHIFT)
	MAP_LIST_ELEM(GL_INDEX_WRITEMASK)
	MAP_LIST_ELEM(GL_LIGHT0)
	MAP_LIST_ELEM(GL_LIGHT1)
	MAP_LIST_ELEM(GL_LIGHT2)
	MAP_LIST_ELEM(GL_LIGHT3)
	MAP_LIST_ELEM(GL_LIGHT4)
	MAP_LIST_ELEM(GL_LIGHT5)
	MAP_LIST_ELEM(GL_LIGHT6)
	MAP_LIST_ELEM(GL_LIGHT7)
	MAP_LIST_ELEM(GL_LIGHTING)
	MAP_LIST_ELEM(GL_LIGHT_MODEL_AMBIENT)
	MAP_LIST_ELEM(GL_LIGHT_MODEL_LOCAL_VIEWER)
	MAP_LIST_ELEM(GL_LIGHT_MODEL_TWO_SIDE)
	MAP_LIST_ELEM(GL_LINE_SMOOTH)
	MAP_LIST_ELEM(GL_LINE_SMOOTH_HINT)
	MAP_LIST_ELEM(GL_LINE_STIPPLE)
	MAP_LIST_ELEM(GL_LINE_STIPPLE_PATTERN)
	MAP_LIST_ELEM(GL_LINE_STIPPLE_REPEAT)
	MAP_LIST_ELEM(GL_LINE_WIDTH)
	MAP_LIST_ELEM(GL_LINE_WIDTH_GRANULARITY)
	MAP_LIST_ELEM(GL_LINE_WIDTH_RANGE)
	MAP_LIST_ELEM(GL_LIST_BASE)
	MAP_LIST_ELEM(GL_LIST_INDEX)
	MAP_LIST_ELEM(GL_LIST_MODE)
	MAP_LIST_ELEM(GL_LOGIC_OP)
	MAP_LIST_ELEM(GL_LOGIC_OP_MODE)
	MAP_LIST_ELEM(GL_MAP1_COLOR_4)
	MAP_LIST_ELEM(GL_MAP1_GRID_DOMAIN)
	MAP_LIST_ELEM(GL_MAP1_GRID_SEGMENTS)
	MAP_LIST_ELEM(GL_MAP1_INDEX)
	MAP_LIST_ELEM(GL_MAP1_NORMAL)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_1)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_2)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_3)
	MAP_LIST_ELEM(GL_MAP1_TEXTURE_COORD_4)
	MAP_LIST_ELEM(GL_MAP1_VERTEX_3)
	MAP_LIST_ELEM(GL_MAP1_VERTEX_4)
	MAP_LIST_ELEM(GL_MAP2_COLOR_4)
	MAP_LIST_ELEM(GL_MAP2_GRID_DOMAIN)
	MAP_LIST_ELEM(GL_MAP2_GRID_SEGMENTS)
	MAP_LIST_ELEM(GL_MAP2_INDEX)
	MAP_LIST_ELEM(GL_MAP2_NORMAL)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_1)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_2)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_3)
	MAP_LIST_ELEM(GL_MAP2_TEXTURE_COORD_4)
	MAP_LIST_ELEM(GL_MAP2_VERTEX_3)
	MAP_LIST_ELEM(GL_MAP2_VERTEX_4)
	MAP_LIST_ELEM(GL_MAP_COLOR)
	MAP_LIST_ELEM(GL_MAP_STENCIL)
	MAP_LIST_ELEM(GL_MATRIX_MODE)
	MAP_LIST_ELEM(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_ATTRIB_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_CLIP_PLANES)
	MAP_LIST_ELEM(GL_MAX_EVAL_ORDER)
	MAP_LIST_ELEM(GL_MAX_LIGHTS)
	MAP_LIST_ELEM(GL_MAX_LIST_NESTING)
	MAP_LIST_ELEM(GL_MAX_MODELVIEW_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_NAME_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_PIXEL_MAP_TABLE)
	MAP_LIST_ELEM(GL_MAX_PROJECTION_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_TEXTURE_SIZE)
	MAP_LIST_ELEM(GL_MAX_TEXTURE_STACK_DEPTH)
	MAP_LIST_ELEM(GL_MAX_VIEWPORT_DIMS)
	MAP_LIST_ELEM(GL_MODELVIEW_MATRIX)
	MAP_LIST_ELEM(GL_MODELVIEW_STACK_DEPTH)
	MAP_LIST_ELEM(GL_NAME_STACK_DEPTH)
	MAP_LIST_ELEM(GL_NORMAL_ARRAY)
	MAP_LIST_ELEM(GL_NORMAL_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_NORMAL_ARRAY_TYPE)
	MAP_LIST_ELEM(GL_NORMALIZE)
	MAP_LIST_ELEM(GL_PACK_ALIGNMENT)
	MAP_LIST_ELEM(GL_PACK_LSB_FIRST)
	MAP_LIST_ELEM(GL_PACK_ROW_LENGTH)
	MAP_LIST_ELEM(GL_PACK_SKIP_PIXELS)
	MAP_LIST_ELEM(GL_PACK_SKIP_ROWS)
	MAP_LIST_ELEM(GL_PACK_SWAP_BYTES)
	MAP_LIST_ELEM(GL_PERSPECTIVE_CORRECTION_HINT)
	MAP_LIST_ELEM(GL_PIXEL_MAP_A_TO_A_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_B_TO_B_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_G_TO_G_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_I_TO_A_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_I_TO_B_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_I_TO_G_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_I_TO_I_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_I_TO_R_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_R_TO_R_SIZE)
	MAP_LIST_ELEM(GL_PIXEL_MAP_S_TO_S_SIZE)
	MAP_LIST_ELEM(GL_POINT_SIZE)
	MAP_LIST_ELEM(GL_POINT_SIZE_GRANULARITY)
	MAP_LIST_ELEM(GL_POINT_SIZE_RANGE)
	MAP_LIST_ELEM(GL_POINT_SMOOTH)
	MAP_LIST_ELEM(GL_POINT_SMOOTH_HINT)
	MAP_LIST_ELEM(GL_POLYGON_MODE)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_FACTOR)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_UNITS)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_FILL)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_LINE)
	MAP_LIST_ELEM(GL_POLYGON_OFFSET_POINT)
	MAP_LIST_ELEM(GL_POLYGON_SMOOTH)
	MAP_LIST_ELEM(GL_POLYGON_SMOOTH_HINT)
	MAP_LIST_ELEM(GL_POLYGON_STIPPLE)
	MAP_LIST_ELEM(GL_PROJECTION_MATRIX)
	MAP_LIST_ELEM(GL_PROJECTION_STACK_DEPTH)
	MAP_LIST_ELEM(GL_READ_BUFFER)
	MAP_LIST_ELEM(GL_RED_BIAS)
	MAP_LIST_ELEM(GL_RED_BITS)
	MAP_LIST_ELEM(GL_RED_SCALE)
	MAP_LIST_ELEM(GL_RENDER_MODE)
	MAP_LIST_ELEM(GL_RGBA_MODE)
	MAP_LIST_ELEM(GL_SCISSOR_BOX)
	MAP_LIST_ELEM(GL_SCISSOR_TEST)
	MAP_LIST_ELEM(GL_SHADE_MODEL)
	MAP_LIST_ELEM(GL_STENCIL_BITS)
	MAP_LIST_ELEM(GL_STENCIL_CLEAR_VALUE)
	MAP_LIST_ELEM(GL_STENCIL_FAIL)
	MAP_LIST_ELEM(GL_STENCIL_FUNC)
	MAP_LIST_ELEM(GL_STENCIL_PASS_DEPTH_FAIL)
	MAP_LIST_ELEM(GL_STENCIL_PASS_DEPTH_PASS)
	MAP_LIST_ELEM(GL_STENCIL_REF)
	MAP_LIST_ELEM(GL_STENCIL_TEST)
	MAP_LIST_ELEM(GL_STENCIL_VALUE_MASK)
	MAP_LIST_ELEM(GL_STENCIL_WRITEMASK)
	MAP_LIST_ELEM(GL_STEREO)
	MAP_LIST_ELEM(GL_SUBPIXEL_BITS)
	MAP_LIST_ELEM(GL_TEXTURE_1D)
	MAP_LIST_ELEM(GL_TEXTURE_2D)
	MAP_LIST_ELEM(GL_TEXTURE_COORD_ARRAY)
	MAP_LIST_ELEM(GL_TEXTURE_COORD_ARRAY_SIZE)
	MAP_LIST_ELEM(GL_TEXTURE_COORD_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_TEXTURE_COORD_ARRAY_TYPE)
	MAP_LIST_ELEM(GL_TEXTURE_ENV_COLOR)
	MAP_LIST_ELEM(GL_TEXTURE_ENV_MODE)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_Q)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_R)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_S)
	MAP_LIST_ELEM(GL_TEXTURE_GEN_T)
	MAP_LIST_ELEM(GL_TEXTURE_MATRIX)
	MAP_LIST_ELEM(GL_TEXTURE_STACK_DEPTH)
	MAP_LIST_ELEM(GL_UNPACK_ALIGNMENT)
	MAP_LIST_ELEM(GL_UNPACK_LSB_FIRST)
	MAP_LIST_ELEM(GL_UNPACK_ROW_LENGTH)
	MAP_LIST_ELEM(GL_UNPACK_SKIP_PIXELS)
	MAP_LIST_ELEM(GL_UNPACK_SKIP_ROWS)
	MAP_LIST_ELEM(GL_UNPACK_SWAP_BYTES)
	MAP_LIST_ELEM(GL_VERTEX_ARRAY)
	MAP_LIST_ELEM(GL_VERTEX_ARRAY_SIZE)
	MAP_LIST_ELEM(GL_VERTEX_ARRAY_STRIDE)
	MAP_LIST_ELEM(GL_VERTEX_ARRAY_TYPE)
	MAP_LIST_ELEM(GL_VIEWPORT)
	MAP_LIST_ELEM(GL_ZOOM_X)
	MAP_LIST_ELEM(GL_ZOOM_Y);

static const char* g_FuncList[] = {
	"GL_NEVER", "GL_LESS", "GL_EQUAL", "GL_LEQUAL", "GL_GREATER", "GL_NOTEQUAL", "GL_GEQUAL", "GL_ALWAYS"
};

static const char* g_DrawBufferModeList[] = {
	"GL_FRONT_LEFT", "GL_FRONT_RIGHT", "GL_BACK_LEFT", "GL_BACK_RIGHT", "GL_FRONT", "GL_BACK", "GL_LEFT",
	"GL_RIGHT", "GL_FRONT_AND_BACK", "GL_AUX0", "GL_AUX1", "GL_AUX2", "GL_AUX3"
};

static const char* g_PolygonModeList[] = { "GL_POINT", "GL_LINE", "GL_FILL" };

static const char* g_DataTypeList[] = { 
	"GL_BYTE", "GL_UNSIGNED_BYTE", "GL_SHORT", "GL_UNSIGNED_SHORT", "GL_INT", "GL_UNSIGNED_INT",
	"GL_FLOAT", "GL_2_BYTES", "GL_3_BYTES", "GL_4_BYTES", "GL_DOUBLE"
};

static const char* g_BeginModeList[] = {
	"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP",
	"GL_TRIANGLE_FAN", "GL_QUADS", "GL_QUAD_STRIP", "GL_POLYGON"
};

GLsizei getTypeSize(GLenum type) {
	//BYTE, UBYTE, SHORT, USHORT, INT, UINT, FLOAT, 2_BYTES, 3_BYTES, 4_BYTES, DOUBLE
	size_t const arr[] = {1,1,2,2,4,4,4,2,3,4,8};
	int offset = type - GL_BYTE;
	return (offset>=0 && offset<sizeof(arr)/sizeof(arr[0])) ? arr[offset] : 0;
}

#pragma region wgl code

DLL_EXPORT BOOL  WINAPI wglCopyContext(HGLRC hRC1, HGLRC hRC2, UINT p) {
	log("%s(0x%X, 0x%X, %u);\n", __FUNCTION__, (UINT)hRC1, (UINT)hRC2, p);
	return FALSE;
}

DLL_EXPORT HGLRC WINAPI wglCreateLayerContext(HDC hDC, int p) {
	log("%s(0x%X, %i);\n", __FUNCTION__, (UINT)hDC, p);
	return 0;
}

DLL_EXPORT HGLRC WINAPI wglGetCurrentContext(VOID) {
	log("%s();\n", __FUNCTION__);
	return hrc;
}

void APIENTRY glMTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t);
void APIENTRY glSelectTextureSGIS(GLenum target);

DLL_EXPORT PROC  WINAPI wglGetProcAddress(LPCSTR name) {
	log("%s('%s');\n", __FUNCTION__, name);

	if (!strcmp(name, "glMTexCoord2fSGIS"))
		return (PROC)glMTexCoord2fSGIS;
	else if (!strcmp(name, "glSelectTextureSGIS"))
		return (PROC)glSelectTextureSGIS;
	else if (!strcmp(name, "glArrayElementEXT"))
		return (PROC)glArrayElement;
	else if (!strcmp(name, "glColorPointerEXT"))
		return (PROC)glColorPointer;
	else if (!strcmp(name, "glTexCoordPointerEXT"))
		return (PROC)glTexCoordPointer;
	else if (!strcmp(name, "glVertexPointerEXT"))
		return (PROC)glVertexPointer;

	return GetProcAddress(g_hDLL, name);
}

DLL_EXPORT BOOL  WINAPI wglShareLists(HGLRC hRC1, HGLRC hRC2) {
	log("%s(0x%X, 0x%X);\n", __FUNCTION__, (UINT)hRC1, (UINT)hRC2);
	return FALSE;
}

DLL_EXPORT BOOL  WINAPI wglUseFontBitmapsA(HDC hDC, DWORD p1, DWORD p2, DWORD p3) {
	log("%s(0x%X, %u, %u, %u);\n", __FUNCTION__, (UINT)hDC, p1, p2, p3);
	return FALSE;
}

DLL_EXPORT BOOL  WINAPI wglUseFontBitmapsW(HDC hDC, DWORD p1, DWORD p2, DWORD p3) {
	log("%s(0x%X, %u, %u, %u);\n", __FUNCTION__, (UINT)hDC, p1, p2, p3);
	return FALSE;
}

DLL_EXPORT int APIENTRY wglChoosePixelFormat(HDC hDC, CONST LPPIXELFORMATDESCRIPTOR pfd) {
	log("%s(0x%X);\n", __FUNCTION__, (UINT)hDC);
	pfd->cColorBits;
	return 1;
}

DLL_EXPORT int APIENTRY wglSetPixelFormat(HDC hDC, int iPixelFormat, CONST LPPIXELFORMATDESCRIPTOR ppfd) {
	log("%s(0x%X, %i);\n", __FUNCTION__, (UINT)hDC, iPixelFormat);

	if (iPixelFormat != 1) {
		return 0;
	}
	
	return 1;
}

DLL_EXPORT int APIENTRY wglDescribePixelFormat(HDC hDC, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd) {
	log("%s(0x%X, %i, %u);\n", __FUNCTION__, (UINT)hDC, iPixelFormat, nBytes);

	PIXELFORMATDESCRIPTOR& pfd = *ppfd;
	ZeroMemory(&pfd, nBytes);
	
	if (iPixelFormat == 1) {
		pfd.nVersion = 1;
		pfd.nSize = sizeof(pfd);
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.cAlphaBits = 8;
		pfd.cRedShift = 24;
		pfd.cRedBits = 8;
		pfd.cRedShift = 16;
		pfd.cGreenBits = 8;
		pfd.cGreenShift = 8;
		pfd.cBlueBits = 8;
		pfd.cBlueShift = 0;
		pfd.iLayerType = PFD_MAIN_PLANE;
	}

	return 1;
}

DLL_EXPORT HGLRC APIENTRY wglCreateContext(HDC hDC) {
	log("%s(0x%X);\n", __FUNCTION__, (UINT)hDC);

	for (size_t i=0; i<MAX_CONTEXT_COUNT; ++i) {
		HGLRC rc = reinterpret_cast<HGLRC>(i + HRC_BASE);
		if (auto ictx = contexts.find(rc); ictx == contexts.end()) {
			auto[it, ok] = contexts.insert(std::pair{rc, std::make_unique<Context>()});
			Context* const ctx = it->second.get();

			size_t width = GetDeviceCaps(hDC, HORZRES), height = GetDeviceCaps(hDC, VERTRES);
			if (HWND hWnd = WindowFromDC(hDC); hWnd) {
				RECT r;
				GetClientRect(hWnd, &r);
				width = r.right;
				height = r.bottom;
			}

			ctx->buf.x_ = 0;
			ctx->buf.y_ = 0;
			ctx->buf.width_ = width;
			ctx->buf.height_ = height;
			ctx->buf.bpp_ = 32;
			ctx->buf.BPP_ = 4;
			ctx->buf.stride_ = width * ctx->buf.BPP_;
			ctx->buf.size_ = ctx->buf.stride_ * height;
			ctx->buf.wh_ = ctx->buf.width_ * ctx->buf.height_;
			ctx->buf.minx_ = ctx->buf.miny_ = 0;
			ctx->buf.maxx_ = width - 1;
			ctx->buf.maxy_ = height - 1;
			ctx->buf.cbuf_ = reinterpret_cast<u8*>(_aligned_malloc(ctx->buf.size_, 32));
#			ifdef __WBUF__
			ctx->buf.wbuf_ = reinterpret_cast<f32*>(_aligned_malloc(ctx->buf.wh_ * sizeof(f32), 32));
#			else
			ctx->buf.zbuf_ = reinterpret_cast<f32*>(_aligned_malloc(ctx->buf.wh_ * sizeof(f32), 32));
#			endif

			ZeroMemory(ctx->buf.cbuf_, ctx->buf.size_);
			ZeroMemory(&ctx->bmi, sizeof(ctx->bmi));

#			ifdef __WBUF__
			for (size_t i = 0; i < ctx->buf.wh_ >> 2; ((__m128*)ctx->buf.wbuf_)[i++] = _mm_set1_ps(1.f / ctx->state->depthFar));
#			else
			for (size_t i = 0; i < ctx->buf.wh_ >> 2; ((__m128*)ctx->buf.zbuf_)[i++] = _mm_set1_ps(ctx->state->depthFar));
#			endif

			ctx->bmi.bmiHeader.biBitCount = GetDeviceCaps(hDC, BITSPIXEL);
			ctx->bmi.bmiHeader.biWidth = ctx->buf.width_;
			ctx->bmi.bmiHeader.biHeight = -(i32)ctx->buf.height_;
			ctx->bmi.bmiHeader.biPlanes = 1;
			ctx->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			ctx->bmi.bmiHeader.biCompression = BI_RGB;
			ctx->bmi.bmiHeader.biSizeImage = ctx->buf.size_;

			ctx->state->ndc2sc3 = _mm_set_ps(f32(ctx->buf.vw_) * .5f, f32(ctx->buf.vh_) * .5f, (ctx->state->depthFar - ctx->state->depthNear) * .5f, 1.f);

			return it->first;
		}
	}

	return 0;
}

DLL_EXPORT BOOL APIENTRY wglDeleteContext(HGLRC hRC) {
	log("%s(0x%X);\n", __FUNCTION__, (UINT)hRC);

	if (auto it = contexts.find(hRC); it != contexts.end()) {
		contexts.erase(it);
		return TRUE;
	}
	
	return FALSE;
}

DLL_EXPORT HDC APIENTRY wglGetCurrentDC(VOID) {
	log("%s(.);\n", __FUNCTION__);
	return hdc;
}

DLL_EXPORT BOOL APIENTRY wglMakeCurrent(HDC hDC, HGLRC hRC) {
	log("%s(0x%X, 0x%X);\n", __FUNCTION__, (UINT)hDC, (UINT)hRC);

	if (!hDC && !hRC) {
		hdc = nullptr;
		ctx = nullptr;
		return TRUE;
	} else if (auto it = contexts.find(hRC); hDC && hRC && it != contexts.end()) {
		hdc = hDC;
		ctx = it->second.get();
		return TRUE;
	}

	hdc = nullptr;
	ctx = nullptr;
	return FALSE;
}

DLL_EXPORT BOOL APIENTRY wglSwapBuffers(HDC hDC) {
	log("%s(0x%X);\n", __FUNCTION__, (UINT)hDC);

	static size_t frame = 0;
	/*
	DWORD dwAttrib = GetFileAttributesA("swap.sem");

	if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		png_t png;
		if (!png_init(0, 0) && !png_open_file_write(&png, "scr.png")) {
			png_set_data(&png, ctx->buf.width_, ctx->buf.height_, 8, PNG_TRUECOLOR_ALPHA, (unsigned char*)ctx->buf.cbuf_);
			png_close_file(&png);
		}
	}
	//*/
	// sync here
	// lock-free
	// 8 threads, per line? thread_id == line % thread_num = [0..7]?
	//WaitForMultipleObjects(threads);
	/* for screens
	for (int i = 0; i < ctx->buf.vh_; i++) {
		for (int j = 0; j < ctx->buf.vw_; j++) {
			ctx->buf.cbuf_[i*ctx->buf.stride_ + j * 4 + 3] = 255;
		}
	}//*/

	if (!ctx->buf.vw_ || !ctx->buf.vh_) {
		return FALSE;
	}
	
	jobs.wait();
	
	if (!SetDIBitsToDevice(hdc, ctx->buf.vx_, ctx->buf.vx_, ctx->buf.vw_, ctx->buf.vh_, 0, 0, 0, ctx->buf.vh_, ctx->buf.cbuf_, &ctx->bmi, DIB_RGB_COLORS)) {
		HandleError();
	}

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//DEBUG, REMOVE
	++frame;

	double ovdColor = ctx->buf.cntColor / (double)ctx->buf.wh_ - 1.;
	double ovdZcheck = ctx->buf.cntZcheck / (double)ctx->buf.wh_ - 1.;
	double ovdZpass = ctx->buf.cntZpass / (double)ctx->buf.wh_ - 1.;

	log("OVD:zCheck=%f zPass=%f, color=%f\n", ovdZcheck, ovdZpass, ovdColor);

	ctx->buf.cntColor = 0ll;
	ctx->buf.cntZpass = 0ll;
	ctx->buf.cntZcheck = 0ll;

	static ts ts_;
	double t = ts_.current_sec();
	static const double dt = 2.;
	static double start = t, end = start + dt;
	static i64 startFrame = frame;

	if (t > end) {
		double fps = (frame - startFrame) / (end - start);
		start = t;
		end = t + dt;
		startFrame = frame;
		char buf[32];
		sprintf_s(buf, "Wnd: fps=%.0f\n", fps);
		OutputDebugStringA(buf);
	}
	/*
	dwAttrib = GetFileAttributesA("fps.sem");

	if ((dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
		//SetTextColor(hdc, RGB(0, 255, 0));
		//SetBkMode(hdc, TRANSPARENT);
		//BeginPath(hdc);
		//TextOut(hdc, 0, 0, buf, strlen(buf));//DRAW FPS HERE
		//RECT rect{ ctx->buf.vx_, ctx->buf.vx_, ctx->buf.vw_, ctx->buf.vh_ };
		//DrawText(hdc, buf, strlen(buf), &rect, DT_RIGHT|DT_TOP);
		//EndPath(hdc);
	}
	//*/
	return TRUE;
}

#pragma endregion

#pragma region gl code

void glVertex(const v4& v, const c4& c, const t4& t, const v4& n/*, Texture* tex[2]*/) {
	ctx->state->vertices.push_back(v);
	ctx->state->colors.push_back(c);
	ctx->state->texcoords.push_back(t);
	ctx->state->normals.push_back(n);
}

DLL_EXPORT void APIENTRY glAccum (GLenum op, GLfloat value) {
	log("%s(0x%X, %.3f);\n", __FUNCTION__, op, value);
}

DLL_EXPORT void APIENTRY glAlphaFunc (GLenum func, GLclampf ref) {
	log("%s(%s, %.3f);\n", __FUNCTION__, (g_FuncList[func-GL_NEVER]), ref);
	ctx->state->alphaFunc = (OpFunc)func;
	ctx->state->alphaRefI = __min(int(ref * 255), 255);

	static const OpFuncI opTableI[] = {
		OpFuncNeverI,
		OpFuncLessI,
		OpFuncEqualI,
		OpFuncLEqualI,
		OpFuncGreaterI,
		OpFuncNotEqualI,
		OpFuncGEqualI,
		OpFuncAlwaysI
	};

	ctx->state->alphaOpI = opTableI[func - GL_NEVER];
}

DLL_EXPORT GLboolean APIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences) {
	log("%s(.);\n", __FUNCTION__);
	return false;
}

DLL_EXPORT void APIENTRY glMultiTexCoord1f(GLenum target, GLfloat s);
DLL_EXPORT void APIENTRY glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t);

DLL_EXPORT void APIENTRY glArrayElement (GLint i) {
	log("%s(%i);\n", __FUNCTION__, i);

	for (int i = 0; i < 2; i++) {
		if (FLAG_BIT(ctx->state->bTextureCoordArray, 0)) {
			//size=1,2,3,4; type=SHORT, INT, FLOAT, DOUBLE; stride=bytes
			// glTexCoordPointer(st, GL_FLOAT, str, pointer);

			GLubyte const* ptr = ctx->state->texcoordArrayPointer + ctx->state->texcoordArrayStride;
			switch (ctx->state->texcoordArrayType) {
			case GL_SHORT: { break; }
			case GL_INT: { break; }
			case GL_FLOAT: {
				GLfloat const* p = (GLfloat const*)ptr;
				switch (ctx->state->texcoordArraySize) {
				case 1: glMultiTexCoord1f(GL_TEXTURE0 + i, p[0]); break;
				case 2: glMultiTexCoord2f(GL_TEXTURE0 + i, p[0], p[1]); break;
				case 3: glMultiTexCoord2f(GL_TEXTURE0 + i, p[0], p[1]); break;
				case 4: glMultiTexCoord2f(GL_TEXTURE0 + i, p[0], p[1]); break;
				}
				break; }
			case GL_DOUBLE: { break; }
			default: ;//GL_INVALID_ENUM
			}
		}
	}

	if (ctx->state->bColorArray) {
		//size=3,4; type=BYTE, UBYTE, SHORT, USHORT, INT, UINT, FLOAT, DOUBLE; stride=bytes
		// glColor4ub glColorPointer(sc, tc, str, pointer+pc);

		GLubyte const* ptr = ctx->state->colorArrayPointer + ctx->state->colorArrayStride;
		switch (ctx->state->colorArrayType) {
		case GL_BYTE: {
			break; }
		case GL_UNSIGNED_BYTE: {
			switch (ctx->state->colorArraySize) {
			case 3: glColor3ub(ptr[0], ptr[1], ptr[2]); break;
			case 4: glColor4ub(ptr[0], ptr[1], ptr[2], ptr[3]); break;
			}
			break; }
		case GL_SHORT: {
			break; }
		case GL_UNSIGNED_SHORT: {
			break; }
		case GL_INT: {
			break; }
		case GL_UNSIGNED_INT: {
			break; }
		case GL_FLOAT: {
			GLfloat const* p = (GLfloat const*)ptr;
			switch (ctx->state->colorArraySize) {
			case 3: glColor3f(p[0], p[1], p[2]); break;
			case 4: glColor4f(p[0], p[1], p[2], p[3]); break;
			}
			break; }
		case GL_DOUBLE: {
			break; }
		default:
			;//GL_INVALID_ENUM
		}
	}

	if (ctx->state->bNormalArray) {
		// type=BYTE, SHORT, INT, FLOAT, DOUBLE; stride=bytes
		// glNormalPointer(GL_FLOAT, stride, pointer+pn);

		GLubyte const* ptr = ctx->state->normalArrayPointer + ctx->state->normalArrayStride;
		switch (ctx->state->normalArrayType) {
		case GL_BYTE: { break; }
		case GL_SHORT: { break; }
		case GL_INT: { break; }
		case GL_FLOAT: {
			GLfloat const* p = (GLfloat const*)ptr;
			glNormal3f(p[0], p[1], p[2]);
			break; }
		case GL_DOUBLE: { break; }
		default: ;//GL_INVALID_ENUM
		}
	}

	if (ctx->state->bVertexArray) {
		// size=2,3,4; type=SHORT, INT, FLOAT, DOUBLE; stride=bytes
		// glVertexPointer(sv, tc, str, pointer+pc);
		GLubyte const* ptr = ctx->state->vertexArrayPointer + ctx->state->vertexArrayStride;
		switch (ctx->state->vertexArrayType) {
		case GL_SHORT: {
			break; }
		case GL_INT: {
			break; }
		case GL_FLOAT: {
			GLfloat const* p = (GLfloat const*)ptr;
			switch (ctx->state->vertexArraySize) {
			case 2:	glVertex2f(p[0], p[1]); break;
			case 3:	glVertex3f(p[0], p[1], p[2]); break;
			case 4:	glVertex4f(p[0], p[1], p[2], p[3]); break;
			}
			break; }
		case GL_DOUBLE: {
			break; }
		default:
			;//GL_INVALID_ENUM
		}
	}
}

DLL_EXPORT void APIENTRY glArrayElementEXT (GLint i) {
	log("%s(%i);\n", __FUNCTION__, i);
	return glArrayElement(i);
}

DLL_EXPORT void APIENTRY glBegin(GLenum mode) {
	log("%s(%s);\n", __FUNCTION__, g_BeginModeList[mode]);

	static const ShapeMode table[] = {
		SHAPE_POINTS,SHAPE_LINES,SHAPE_LINE_LOOP,SHAPE_LINE_STRIP,
		SHAPE_TRIANGLES,SHAPE_TRIANGLE_STRIP,SHAPE_TRIANGLE_FAN,
		SHAPE_QUADS,SHAPE_QUAD_STRIP,SHAPE_POLYGON };

	ctx->state->modeShape = table[mode];
}

DLL_EXPORT void APIENTRY glBindTexture (GLenum target, GLuint texture) {
	log("%s(%s, %u);\n", __FUNCTION__, (target==GL_TEXTURE_2D?"GL_TEXTURE_2D":target==GL_TEXTURE_1D?"GL_TEXTURE_1D":"UNKNOWN"), texture);
	ctx->state->tex2d[ctx->state->texActive] = texture;
}

DLL_EXPORT void APIENTRY glBindTextureEXT (GLenum target, GLuint texture) {
	glBindTexture(target, texture);
}

DLL_EXPORT void APIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
	log("%s(%u, %u, %.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, width, height, xorig, yorig, xmove, ymove);
}

DLL_EXPORT void APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor) {
	//sfactor: GL_ZERO, GL_ONE, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, and GL_SRC_ALPHA_SATURATE
	//dfactor: GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, and GL_ONE_MINUS_DST_ALPHA
	static const char* arr1[] = { "GL_ZERO", "GL_ONE" };
	static const char* arr2[] = {
		"GL_SRC_COLOR", "GL_ONE_MINUS_SRC_COLOR", "GL_SRC_ALPHA", "GL_ONE_MINUS_SRC_ALPHA",
		"GL_DST_ALPHA", "GL_ONE_MINUS_DST_ALPHA", "GL_DST_COLOR", "GL_ONE_MINUS_DST_COLOR",
		"GL_SRC_ALPHA_SATURATE"
	};

	log("%s(%s, %s);\n", __FUNCTION__,
		((sfactor<GL_SRC_COLOR) ? arr1[sfactor] : arr2[sfactor-GL_SRC_COLOR]),
		((dfactor<GL_SRC_COLOR) ? arr1[dfactor] : arr2[dfactor-GL_SRC_COLOR]));

	switch (sfactor) {
	case GL_ZERO: ctx->state->sfactor = BLEND_ZERO; break;
	case GL_ONE: ctx->state->sfactor = BLEND_ONE; break;
	case GL_SRC_ALPHA: ctx->state->sfactor = BLEND_SRC_ALPHA; break;
	case GL_ONE_MINUS_SRC_ALPHA: ctx->state->sfactor = BLEND_ONE_MINUS_SRC_ALPHA; break;
	case GL_DST_ALPHA: ctx->state->sfactor = BLEND_DST_ALPHA; break;
	case GL_ONE_MINUS_DST_ALPHA: ctx->state->sfactor = BLEND_ONE_MINUS_DST_ALPHA; break;
	case GL_DST_COLOR: ctx->state->sfactor = BLEND_DST_COLOR; break;
	case GL_ONE_MINUS_DST_COLOR: ctx->state->sfactor = BLEND_ONE_MINUS_DST_COLOR; break;
	case GL_SRC_ALPHA_SATURATE: ctx->state->sfactor = BLEND_SRC_ALPHA_SATURATE; break;
	default: assert(0);
	}

	switch (dfactor) {
	case GL_ZERO: ctx->state->dfactor = BLEND_ZERO; break;
	case GL_ONE: ctx->state->dfactor = BLEND_ONE; break;
	case GL_SRC_COLOR: ctx->state->dfactor = BLEND_SRC_COLOR; break;
	case GL_ONE_MINUS_SRC_COLOR: ctx->state->dfactor = BLEND_ONE_MINUS_SRC_COLOR; break;
	case GL_SRC_ALPHA: ctx->state->dfactor = BLEND_SRC_ALPHA; break;
	case GL_ONE_MINUS_SRC_ALPHA: ctx->state->dfactor = BLEND_ONE_MINUS_SRC_ALPHA; break;
	case GL_DST_ALPHA: ctx->state->dfactor = BLEND_DST_ALPHA; break;
	case GL_ONE_MINUS_DST_ALPHA: ctx->state->dfactor = BLEND_ONE_MINUS_DST_ALPHA; break;
	}
}

DLL_EXPORT void APIENTRY glCallList (GLuint list) {
	log("%s(%u);\n", __FUNCTION__, list);
}

DLL_EXPORT void APIENTRY glCallLists (GLsizei n, GLenum type, const GLvoid *lists) {
	log("%s(%u, %i);\n", __FUNCTION__, n, type);
}

DLL_EXPORT void APIENTRY glClear(GLbitfield mask) {
#	if LOG
	std::string params;
	if (mask & GL_COLOR_BUFFER_BIT)
		params += "GL_COLOR_BUFFER_BIT";
	if (mask & GL_DEPTH_BUFFER_BIT) {
		if (params.size()) params += " | ";
		params += "GL_DEPTH_BUFFER_BIT";
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
		if (params.size()) params += " | ";
		params += "GL_STENCIL_BUFFER_BIT";
	}
	log("%s(%s);\n", __FUNCTION__, params.c_str());
#	endif

	if (mask & GL_COLOR_BUFFER_BIT) { // do it in mt?
		const __m128i c4 = _mm_set1_epi32(ctx->state->clearColor);
		for (size_t i = 0; i < ctx->buf.wh_ / 4; i++)
			((__m128i*)ctx->buf.cbuf_)[i] = c4;
	}

	if (mask & GL_DEPTH_BUFFER_BIT) { // do it in mt?
#		ifdef __WBUF__
		for (size_t i=0; i < ctx->buf.wh_ / 4; i++) {
			((__m128*)ctx->buf.wbuf_)[i] = _mm_set1_ps(1.f / ctx->state->depthFar);
		}
#		else
		for (size_t i = 0; i < ctx->buf.wh_ / 4; i++) {
			((__m128*)ctx->buf.zbuf_)[i] = _mm_set1_ps(ctx->state->depthFar);
		}
#		endif
	}
}

DLL_EXPORT void APIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	log("%s(%.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, red, green, blue, alpha);
}

DLL_EXPORT void APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	log("%s(%.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, red, green, blue, alpha);
	ctx->state->clearColor = c4(alpha, red, green, blue);
}

DLL_EXPORT void APIENTRY glClearDepth (GLclampd depth) {
	log("%s(%.3f);\n", __FUNCTION__, depth);
}

DLL_EXPORT void APIENTRY glClearIndex (GLfloat c) {
	log("%s(%.3f);\n", __FUNCTION__, c);
}

DLL_EXPORT void APIENTRY glClearStencil (GLint s) {
	log("%s(%i);\n", __FUNCTION__, s);
}

DLL_EXPORT void APIENTRY glClipPlane (GLenum plane, const GLdouble *equation) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3bv (const GLbyte *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	log("\t%s(%.3f,%.3f,%.3f);\n", __FUNCTION__, red, green, blue);
	ctx->state->curColor = c4::clamp(c4(1.f, red, green, blue));
}

DLL_EXPORT void APIENTRY glColor3fv(const GLfloat *v) {
	log("\t%s(%.3f,%.3f,%.3f);\n", __FUNCTION__, v[0], v[1], v[2]);
	ctx->state->curColor = c4::clamp(c4(1.f, v[0], v[1], v[2]));
}

DLL_EXPORT void APIENTRY glColor3i (GLint red, GLint green, GLint blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3s (GLshort red, GLshort green, GLshort blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curColor = c4(255, red, green, blue);
}

DLL_EXPORT void APIENTRY glColor3ubv (const GLubyte *v) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curColor = c4(255, v[0], v[1], v[2]);
}

DLL_EXPORT void APIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3uiv (const GLuint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3us (GLushort red, GLushort green, GLushort blue) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor3usv (const GLushort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4bv (const GLbyte *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	log("\t%s(%.3f,%.3f,%.3f,%.3f);\n", __FUNCTION__, red, green, blue, alpha);
	ctx->state->curColor = c4::clamp(c4(alpha, red, green, blue));
}

DLL_EXPORT void APIENTRY glColor4fv(const GLfloat *v) {
	log("\t%s(%.3f,%.3f,%.3f,%.3f);\n", __FUNCTION__, v[0], v[1], v[2], v[3]);
	ctx->state->curColor = c4::clamp(c4(v[3], v[0], v[1], v[2]));
}

DLL_EXPORT void APIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curColor = c4(alpha, red, green, blue);
}

DLL_EXPORT void APIENTRY glColor4ubv (const GLubyte *v) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curColor = c4(v[3], v[0], v[1], v[2]);
}

DLL_EXPORT void APIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4uiv (const GLuint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColor4usv (const GLushort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColorMaterial (GLenum face, GLenum mode) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	//size=3,4
	//type=GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT, or GL_DOUBLE
	//stride=bytes
	log("%s(%i, %s, %u);\n", __FUNCTION__, size, g_DataTypeList[type-GL_BYTE], stride);
	ctx->state->colorArraySize = size;
	ctx->state->colorArrayType = type;
	ctx->state->colorArrayStride = stride;
	ctx->state->colorArrayPointer = (GLubyte const*)pointer;
}

DLL_EXPORT void APIENTRY glColorPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	return glColorPointer(size, type, stride, pointer);
}

DLL_EXPORT void APIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glCullFace (GLenum mode) {
	// GL_FRONT, <GL_BACK>, GL_FRONT_AND_BACK
	ctx->state->cullFace = (CullFaceMode)mode;
	log("%s(%s);\n", __FUNCTION__, g_DrawBufferModeList[mode-GL_FRONT_LEFT]);
}

DLL_EXPORT void APIENTRY glDeleteLists (GLuint list, GLsizei range) {
	log("%s(%u, %u);\n", __FUNCTION__, list, range);
}

DLL_EXPORT void APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures) {
#	if LOG
	std::ostringstream oss;
	for (GLsizei i=0; i<n; ++i) oss << " " << textures[i];
	log("%s(%u,%s);\n", __FUNCTION__, n, oss.str().c_str());
#	endif

	for (GLsizei i=0; i<n; ++i) {
		ctx->state->gen_textures_ids.erase(i);
	}
}

DLL_EXPORT void APIENTRY glDepthFunc (GLenum func) {
	log("%s(%s);\n", __FUNCTION__, g_FuncList[func-GL_NEVER]);
	ctx->state->depthFunc = (OpFunc)func;

	static const OpFuncF opTableW[] = {
		OpFuncNever,
		OpFuncGreater,
		OpFuncEqual,
		OpFuncGEqual,
		OpFuncLess,
		OpFuncNotEqual,
		OpFuncLEqual,
		OpFuncAlways
	};

	static const OpFuncF opTableZ[] = {
		OpFuncNever,
		OpFuncLess,
		OpFuncEqual,
		OpFuncLEqual,
		OpFuncGreater,
		OpFuncNotEqual,
		OpFuncGEqual,
		OpFuncAlways
	};

#	ifdef __WBUF__
	ctx->state->depthOpW = opTableW[func - GL_NEVER];
#	else
	ctx->state->depthOpZ = opTableZ[func - GL_NEVER];
#	endif
}

DLL_EXPORT void APIENTRY glDepthMask (GLboolean flag) {
	log("%s(%s);\n", __FUNCTION__, (flag?"TRUE":"FALSE"));
	ctx->state->bDepthMask = flag != 0;
}

DLL_EXPORT void APIENTRY glDepthRange (GLclampd zNear, GLclampd zFar) {
	log("%s(%.3f, %.3f);\n", __FUNCTION__, zNear, zFar);
	ctx->state->depthNear = zNear;
	ctx->state->depthFar = zFar;
	ctx->state->ndc2sc3 = _mm_set_ps(f32(ctx->buf.vw_) * .5f, f32(ctx->buf.vh_) * .5f, (zFar - zNear) * .5f, 1.f);
	ctx->state->ndc2sc4 = _mm_set_ps(0.f, 0.f, zNear, 0.f);
}

DLL_EXPORT void APIENTRY glDisable (GLenum cap) {
#	if LOG
	auto it = g_EnableList.find(cap);
	log("%s(%s);\n", __FUNCTION__, (it!=g_EnableList.end()?it->second:"UNKNOWN"));
#	endif

	switch (cap) {
	case GL_ALPHA_TEST: ctx->state->bAlphaTest = false; break;
	case GL_BLEND: ctx->state->bBlend = false; break;
	case GL_CULL_FACE: ctx->state->bCullFace = false; break;
	case GL_DEPTH_TEST: ctx->state->bDepthTest = false; break;
	case GL_STENCIL_TEST: ctx->state->bStencilTest = false; break;
	case GL_TEXTURE_2D: FLAG_OFF(ctx->state->bTexture, ctx->state->texActive); break;
	}
}

DLL_EXPORT void APIENTRY glDisableClientState (GLenum array) {
#	if LOG
	auto it = g_EnableClientList.find(array);
	log("%s(%s);\n", __FUNCTION__, (it!=g_EnableClientList.end()?it->second:"UNKNOWN"));
#	endif

	switch (array) {
	case GL_VERTEX_ARRAY: ctx->state->bVertexArray = false; break;
	case GL_COLOR_ARRAY: ctx->state->bColorArray = false; break;
	case GL_TEXTURE_COORD_ARRAY: FLAG_OFF(ctx->state->bTextureCoordArray, ctx->state->clientTexActive); break;
	case GL_NORMAL_ARRAY: ctx->state->bNormalArray = false; break;
	}
}

DLL_EXPORT void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count) {
	log("%s(%s, %i, %u);\n", __FUNCTION__, g_BeginModeList[mode], first, count);
	//mode=GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_QUAD_STRIP, GL_QUADS, GL_POLYGON

	//if (mode or count is invalid) generate appropriate error
	glBegin(mode);
	for (int i=first; i<first+count; glArrayElement(i++));
	glEnd();
}

DLL_EXPORT void APIENTRY glDrawBuffer (GLenum mode) {
	// Specifies up to four color buffers to be drawn into.
	// Symbolic constants GL_NONE, GL_FRONT_LEFT, GL_FRONT_RIGHT,
	// GL_BACK_LEFT, GL_BACK_RIGHT, GL_FRONT, GL_BACK, GL_LEFT,
	// GL_RIGHT, and GL_FRONT_AND_BACK are accepted. The initial
	// value is GL_FRONT for single-buffered contexts, and GL_BACK
	// for double-buffered contexts.
	ctx->state->drawBuffer = mode;
	static const char* arr[] = { "GL_FRONT_LEFT", "GL_FRONT_RIGHT", "GL_BACK_LEFT", "GL_BACK_RIGHT", "GL_FRONT", "GL_BACK", "GL_LEFT", "GL_RIGHT", "GL_FRONT_AND_BACK" };

	if (mode == GL_NONE) {
		log("%s(GL_NONE);\n", __FUNCTION__);
	} else if (mode>=GL_FRONT_LEFT && mode<=GL_FRONT_AND_BACK) {
		log("%s(%s);\n", __FUNCTION__, arr[mode-GL_FRONT_LEFT]);
	}
}

DLL_EXPORT void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
	log("%s(%s, %u, %s);\n", __FUNCTION__, g_BeginModeList[mode], count, g_DataTypeList[type-GL_BYTE]);
	//mode=GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_QUAD_STRIP, GL_QUADS, GL_POLYGON
	//type=GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT

	//if (mode, count or type is invalid) generate appropriate error
	glBegin(mode);
	switch (type) {
	case GL_UNSIGNED_BYTE:
		for (int i = 0; i < count; glArrayElement(((GLubyte const*)indices)[i++]));
		break;
	case GL_UNSIGNED_SHORT:
		for (int i = 0; i < count; glArrayElement(((GLushort const*)indices)[i++]));
		break;
	case GL_UNSIGNED_INT:
		for (int i = 0; i < count; glArrayElement(((GLuint const*)indices)[i++]));
		break;
	}
	glEnd();
}

DLL_EXPORT void APIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
#	if LOG
	auto fmt = g_TexFormatList.find(format);
	log("%s(%u, %u, %s, %s);\n", __FUNCTION__,
		width, height, (fmt!=g_TexFormatList.end()?fmt->second:"UNKNOWN"),
		g_DataTypeList[type]);
#	endif
}

DLL_EXPORT void APIENTRY glEdgeFlag (GLboolean flag) {
	log("%s(%s);\n", __FUNCTION__, (flag?"TRUE":"FALSE"));
}

DLL_EXPORT void APIENTRY glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer) {
	log("%s(%u);\n", __FUNCTION__, stride);
}

DLL_EXPORT void APIENTRY glEdgeFlagv (const GLboolean *flag) {
	log("%s(%s);\n", __FUNCTION__, (*flag?"TRUE":"FALSE"));
}

DLL_EXPORT void APIENTRY glEnable (GLenum cap) {
	check_enum(cap
		, GL_ALPHA_TEST
		, GL_BLEND
		, GL_CULL_FACE
		, GL_DEPTH_TEST
		, GL_STENCIL_TEST
		, GL_TEXTURE_2D);

#	if LOG
	auto it = g_EnableList.find(cap);
	log("%s(%s);\n", __FUNCTION__, (it!=g_EnableList.end()?it->second:"UNKNOWN"));
#	endif

	switch (cap) {
	case GL_ALPHA_TEST: ctx->state->bAlphaTest = true; break;
	case GL_BLEND: ctx->state->bBlend = true; break;
	case GL_CULL_FACE: ctx->state->bCullFace = true; break;
	case GL_DEPTH_TEST: ctx->state->bDepthTest = true; break;
	case GL_STENCIL_TEST: ctx->state->bStencilTest = true; break;
	case GL_TEXTURE_2D: FLAG_ON(ctx->state->bTexture, ctx->state->texActive); break;
	}
}

DLL_EXPORT void APIENTRY glEnableClientState (GLenum array) {
#	if LOG
	auto it = g_EnableClientList.find(array);
	log("%s(%s);\n", __FUNCTION__, (it!=g_EnableClientList.end()?it->second:"UNKNOWN"));
#	endif

	switch (array) {
	case GL_VERTEX_ARRAY: ctx->state->bVertexArray = true; break;
	case GL_COLOR_ARRAY: ctx->state->bColorArray = true; break;
	case GL_TEXTURE_COORD_ARRAY: FLAG_ON(ctx->state->bTextureCoordArray, ctx->state->clientTexActive); break;
	case GL_NORMAL_ARRAY: ctx->state->bNormalArray = true; break;
	}
}

void swapRgbaBgra(u8* src, u8* dst, int size) {
	u8 *sptr = src, *dptr = dst;
	for (int i = 0; i<size; ++i, dptr += 4, sptr += 4) {
		dptr[0] = sptr[2];
		dptr[1] = sptr[1];
		dptr[2] = sptr[0];
		dptr[3] = sptr[3];
	}
}

#define __process_tri process_tri

DLL_EXPORT void APIENTRY glEnd() {
	log("%s([v%u,c%u,t%u,n%u]);\n", __FUNCTION__, ctx->state->vertices.size(), ctx->state->colors.size(), ctx->state->texcoords.size(), ctx->state->normals.size());
	if (ctx->state->modeShape == SHAPE_NONE)
		return;

	if (ctx->state->drawBuffer == GL_FRONT) {
		ctx->state->vertices.clear();
		ctx->state->colors.clear();
		ctx->state->texcoords.clear();
		ctx->state->normals.clear();
		return;
	}

	if (ctx->state->bUpdateTotal) {
		ctx->state->bUpdateTotal = false;
		ctx->state->mPVM = ctx->state->mProjection * ctx->state->mModelView;
	}

	for (size_t i = 0; i < ctx->state->vertices.size(); i++) {
		v4& v = ctx->state->vertices[i];
		v = ctx->state->mPVM * v;
	}

	for (size_t i = 0; i < TMU_COUNT; i++) {
		if (FLAG_BIT(ctx->state->bTexture, i)) {
			auto it = ctx->state->textures.find(ctx->state->tex2d[i]);
			ctx->state->tex[i] = it!=ctx->state->textures.end() ? it->second : std::shared_ptr<Texture>();
		} else {
			ctx->state->tex[i] = std::shared_ptr<Texture>();
		}
	}

	switch (ctx->state->modeShape) {
	case SHAPE_POINTS:
		/*
		for (size_t i=0; i<ctx->state->vVertices.size(); ++i) {
			Vertex* v = &ctx->state->vVertices[i];
			ctx->buf.plot(v->v.x, v->v.y, v->c);
		}//*/
		break;
	case SHAPE_LINES: {
		/*
		for (size_t i=0; i<ctx->state->vVertices.size(); i+=2) {
			Vertex* v0 = &ctx->state->vVertices[i];
			Vertex* v1 = &ctx->state->vVertices[i+1];
			ctx->buf.line(v0->v.x, v0->v.y, v1->v.x, v1->v.y, v0->c);
		}//*/
		break;
	}
	case SHAPE_LINE_STRIP: {
		if (ctx->state->vertices.size() < 2) {
			break;
		}
		/*
		Vertex* v0 = &ctx->state->vVertices[0];
		for (size_t i=1; i<ctx->state->vVertices.size(); ++i) {
			Vertex* v1 = &ctx->state->vVertices[i];
			ctx->buf.line(v0->v.x, v0->v.y, v1->v.x, v1->v.y, v0->c);
			v0 = v1;
		}//*/
		break;
	}
	case SHAPE_LINE_LOOP: {
		if (ctx->state->vertices.size() < 2) {
			break;
		}
		/*
		Vertex* v0 = &ctx->state->vVertices[ctx->state->vVertices.size()-1];
		for (size_t i=0; i<ctx->state->vVertices.size(); ++i) {
			Vertex* v1 = &ctx->state->vVertices[i];
			ctx->buf.line(v0->v.x, v0->v.y, v1->v.x, v1->v.y, v0->c);
			v0 = v1;
		}//*/
		break;
	}
	case SHAPE_TRIANGLES: {
		for (size_t i=0; i<ctx->state->vertices.size(); i+=3) {
			ctx->buf.__process_tri(ctx->state, i, i + 1, i + 2);
		}
		break;
	}
	case SHAPE_TRIANGLE_STRIP: {
		//0246
		//1357
		if (ctx->state->vertices.size() < 2) {
			break;
		}

		if (ctx->state->vertices.size() == 3) {
			ctx->buf.__process_tri(ctx->state, 0, 1, 2);
			break;
		}

		size_t i0 = 0, i1 = 1;
		for (size_t i=2; i<(ctx->state->vertices.size()&~1); i+=2) {
			size_t i2 = i, i3 = i + 1;
			ctx->buf.__process_tri(ctx->state, i0, i1, i2);
			ctx->buf.__process_tri(ctx->state, i3, i2, i1);
			i0 = i2;
			i1 = i3;
		}

		if (ctx->state->vertices.size() & 1) {
			size_t last = ctx->state->vertices.size()-1;
			ctx->buf.__process_tri(ctx->state, last - 2, last - 1, last);
		}
		break;
	}
	case SHAPE_QUADS: {
		for (size_t i=0; i<ctx->state->vertices.size(); i+=4) {
			ctx->buf.__process_tri(ctx->state, i, i + 1, i + 2);
			ctx->buf.__process_tri(ctx->state, i, i + 2, i + 3);
		}
		break;
	}
	case SHAPE_QUAD_STRIP: {
		//0246
		//1357
		if (ctx->state->vertices.size() % 4) {
			break;
		}
		size_t i0 = 0, i1 = 1;
		for (size_t i = 2; i < ctx->state->vertices.size(); i += 2) {
			size_t i2 = i, i3 = i + 1;
			ctx->buf.__process_tri(ctx->state, i0, i1, i2);
			ctx->buf.__process_tri(ctx->state, i3, i2, i1);
			i0 = i2;
			i1 = i3;
		}
		break;
	}
	case SHAPE_TRIANGLE_FAN:
	case SHAPE_POLYGON: {
		if (ctx->state->vertices.size() < 3) {
			break;
		}
		size_t i0 = 0, i1 = 1;
		for (size_t i0 = 0, i1 = 1, i2 = 2; i2 < ctx->state->vertices.size(); i1 = i2++) {
			ctx->buf.__process_tri(ctx->state, i0, i1, i2);
		}
		break;
	}
	}

	static const char* shapes[] = {
		"NONE",	"POINTS", "LINES", "LINE_STRIP", "LINE_LOOP",
		"TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN", "QUADS", "QUAD_STRIP", "POLYGON"
	};

	const char* shape = shapes[ctx->state->modeShape];
	/* DEBUG
	DWORD dwAttrib = GetFileAttributesA("end.sem");

	if ((dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
		logging = true;
		for (size_t i=0; i<ctx->state->vertices.size(); ++i) {
			const v4& v = ctx->state->vertices[i];
			const t4& t = ctx->state->texcoords[i];
			const c4& c = ctx->state->colors[i];
			const v4& n = ctx->state->normals[i];
			dump("%05i->% 2i: v(%.3f,%.3f,%.3f,%.3f), t(%.3f,%.3f,%.3f,%.3f), c(%.3f,%.3f,%.3f,%.3f)\n"
				, ctx->state->frame, ctx->state->modeShape, v.x, v.y, v.z, v.w, t.u0, t.v0, t.u1, t.v1, c.r, c.g, c.b, c.a);
		}
		//png_t png;
		//char path[64];
		//sprintf(path, "dump/%05i.png", ctx->state->frame);
		//if (!png_init(0, 0) && !png_open_file_write(&png, path)) {
		//	std::unique_ptr<u8> dst(new u8 [ctx->buf.width_*ctx->buf.height_*4]);
		//	swapRgbaBgra((u8*)ctx->buf.cbuf_, dst.get(), ctx->buf.width_*ctx->buf.height_);
		//	png_set_data(&png, ctx->buf.width_, ctx->buf.height_, 8, PNG_TRUECOLOR_ALPHA, dst.get());
		//	png_close_file(&png);
		//}
		++ctx->state->frame;
	}
	//*/

	ctx->state->vertices.clear();
	ctx->state->colors.clear();
	ctx->state->texcoords.clear();
	ctx->state->normals.clear();
	ctx->state->modeShape = SHAPE_NONE;
}

DLL_EXPORT void APIENTRY glEndList (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalCoord1d (GLdouble u) {
	log("%s(%.3f);\n", __FUNCTION__, u);
}

DLL_EXPORT void APIENTRY glEvalCoord1dv (const GLdouble *u) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalCoord1f (GLfloat u) {
	log("%s(%.3f);\n", __FUNCTION__, u);
}

DLL_EXPORT void APIENTRY glEvalCoord1fv (const GLfloat *u) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalCoord2d (GLdouble u, GLdouble v) {
	log("%s(%.3f, %.3f);\n", __FUNCTION__, u, v);
}

DLL_EXPORT void APIENTRY glEvalCoord2dv (const GLdouble *u) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalCoord2f (GLfloat u, GLfloat v) {
	log("%s(%.3f, %.3f);\n", __FUNCTION__, u, v);
}

DLL_EXPORT void APIENTRY glEvalCoord2fv (const GLfloat *u) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glEvalPoint1 (GLint i) {
	log("%s(%i);\n", __FUNCTION__, i);
}

DLL_EXPORT void APIENTRY glEvalPoint2 (GLint i, GLint j) {
	log("%s(%i, %i);\n", __FUNCTION__, i, j);
}

DLL_EXPORT void APIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer) {
	log("%s(%i, %s);\n", __FUNCTION__, size, g_DataTypeList[type-GL_BYTE]);
}

DLL_EXPORT void APIENTRY glFinish (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFlush (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFogf (GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFogfv (GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFogi (GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFogiv (GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glFrontFace (GLenum mode) {
	static const char* arr[] = { "CW", "CCW" };
	ctx->state->frontFace = (FrontFaceMode)mode;
	log("%s(%s);\n", __FUNCTION__, arr[mode-GL_CW]);
}

DLL_EXPORT void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	log("%s(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, left, right, bottom, top, zNear, zFar);

	m4* m = 0;

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		m = &ctx->state->mProjection;
		ctx->state->projNear = zNear;
		ctx->state->projFar = zFar;
		break;
	case MM_MODELVIEW:
		m = &ctx->state->mModelView;
		break;
	case MM_TEXTURE:
		m = &ctx->state->mTexture;
		break;
	}

	*m *= m4::fru(left, right, bottom, top, zNear, zFar);

	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_00, m->_01, m->_02, m->_03);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_10, m->_11, m->_12, m->_13);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_20, m->_21, m->_22, m->_23);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_30, m->_31, m->_32, m->_33);

	ctx->state->bUpdateTotal = true;
}


DLL_EXPORT GLuint APIENTRY glGenLists (GLsizei range) {
	log("%s(%u);\n", __FUNCTION__, range);
	return 0;
}

DLL_EXPORT void APIENTRY glGenTextures (GLsizei n, GLuint *textures) {
	log("%s(%u);\n", __FUNCTION__, n);

	GLsizei i = 0;
	GLuint idx = 1;

	for (auto it=ctx->state->gen_textures_ids.begin(); it!=ctx->state->gen_textures_ids.end() && i<n; ++it, ++idx) {
		for (; idx < *it; ++idx) {
			log("%s(+tex=%i)", __FUNCTION__, idx);
			textures[i++] = idx;
			ctx->state->gen_textures_ids.insert(idx);
		}
	}

	for (; i<n; ++i, ++idx) {
		ctx->state->gen_textures_ids.insert(idx);
		textures[i] = idx;
	}
}

DLL_EXPORT void APIENTRY glGetBooleanv (GLenum pname, GLboolean *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetClipPlane (GLenum plane, GLdouble *equation) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetDoublev (GLenum pname, GLdouble *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT GLenum APIENTRY glGetError (void) {
	log("%s();\n", __FUNCTION__);
	return 0;
}

DLL_EXPORT void APIENTRY glGetFloatv (GLenum pname, GLfloat *params) {
	log("%s(0x%04X);\n", __FUNCTION__, pname);
	//params_00,_10,_20,_30,_01,_11,_21,_31,_02,_12,_22,_32,_03,_13,_23,_33
	//GL_CURRENT_TEXTURE_COORDS: return curTexCoordarams[ctx->state->texActive]; break; // GL_ARB_multitexture
	//GL_COLOR_MATRIX
	//GL_MODELVIEW_MATRIX
	//GL_PROJECTION_MATRIX
	//GL_TEXTURE_MATRIX

	switch (pname) {
	case GL_CURRENT_COLOR:
		params[0] = ctx->state->curColor[1];
		params[1] = ctx->state->curColor[2];
		params[2] = ctx->state->curColor[3];
		params[3] = ctx->state->curColor[0];
		break;
	case GL_CURRENT_NORMAL:
		params[0] = ctx->state->curNormal[3];
		params[1] = ctx->state->curNormal[2];
		params[2] = ctx->state->curNormal[1];
		break;
	case GL_CURRENT_TEXTURE_COORDS:
		params[0] = ctx->state->curTexCoord[ctx->state->texActive][1];
		params[1] = ctx->state->curTexCoord[ctx->state->texActive][0];
		params[2] = 0.f;
		params[3] = 1.f;
		break;
	case GL_MODELVIEW_MATRIX: {
		cref<m4> m = ctx->state->mModelView;
		log("|%.3f, %.3f, %.3f, %.3f|\n", m._00, m._01, m._02, m._03);
		log("|%.3f, %.3f, %.3f, %.3f|\n", m._10, m._11, m._12, m._13);
		log("|%.3f, %.3f, %.3f, %.3f|\n", m._20, m._21, m._22, m._23);
		log("|%.3f, %.3f, %.3f, %.3f|\n", m._30, m._31, m._32, m._33);

		auto* v = reinterpret_cast<v4*>(params);
		v[0] = m.v[0].shuf<0, 1, 2, 3>();
		v[1] = m.v[1].shuf<0, 1, 2, 3>();
		v[2] = m.v[2].shuf<0, 1, 2, 3>();
		v[3] = m.v[3].shuf<0, 1, 2, 3>();
		break; }
	case GL_PROJECTION_MATRIX: {
		cref<m4> m = ctx->state->mProjection;
		auto* v = reinterpret_cast<v4*>(params);
		v[0] = m.v[0].shuf<0, 1, 2, 3>();
		v[1] = m.v[1].shuf<0, 1, 2, 3>();
		v[2] = m.v[2].shuf<0, 1, 2, 3>();
		v[3] = m.v[3].shuf<0, 1, 2, 3>();
		break; }
	case GL_TEXTURE_MATRIX:
		cref<m4> m = ctx->state->mTexture;
		auto* v = reinterpret_cast<v4*>(params);
		v[0] = m.v[0].shuf<0, 1, 2, 3>();
		v[1] = m.v[1].shuf<0, 1, 2, 3>();
		v[2] = m.v[2].shuf<0, 1, 2, 3>();
		v[3] = m.v[3].shuf<0, 1, 2, 3>();
		break;
	}
}

DLL_EXPORT void APIENTRY glGetIntegerv (GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);

	/* GL_EXT_texture_object
	TEXTURE,
	TEXTURE_PRIORITY_EXT
	TEXTURE_RED_SIZE,
	TEXTURE_GREEN_SIZE,
	TEXTURE_BLUE_SIZE,
	TEXTURE_ALPHA_SIZE,
	TEXTURE_LUMINANCE_SIZE,
	TEXTURE_INTENSITY_SIZE,
	TEXTURE_WIDTH,
	TEXTURE_HEIGHT,
	TEXTURE_DEPTH_EXT,
	TEXTURE_BORDER,
	TEXTURE_COMPONENTS,
	TEXTURE_BORDER_COLOR,
	TEXTURE_MIN_FILTER,
	TEXTURE_MAG_FILTER,
	TEXTURE_WRAP_S,
	TEXTURE_WRAP_T,
	TEXTURE_WRAP_R_EXT
	*/

	/*
	GL_MATRIX_MODE
	GL_COLOR_MATRIX_STACK_DEPTH
	GL_MODELVIEW_STACK_DEPTH
	GL_PROJECTION_STACK_DEPTH
	GL_TEXTURE_STACK_DEPTH
	GL_MAX_MODELVIEW_STACK_DEPTH
	GL_MAX_PROJECTION_STACK_DEPTH
	GL_MAX_TEXTURE_STACK_DEPTH
	*/
	switch (pname) {
	case GL_ACTIVE_TEXTURE: params[0] = ctx->state->texActive; break; // GL_ARB_multitexture
	case GL_MAX_TEXTURE_COORDS: params[0] = TMU_COUNT; break; // GL_ARB_multitexture
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: params[0] = TMU_COUNT; break; // GL_ARB_multitexture
	}
}

DLL_EXPORT void APIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetPixelMapfv (GLenum map, GLfloat *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetPixelMapuiv (GLenum map, GLuint *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetPixelMapusv (GLenum map, GLushort *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetPointerv (GLenum pname, GLvoid* *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetPolygonStipple (GLubyte *mask) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT const GLubyte * APIENTRY glGetString (GLenum name) {
	static const char* arr[] = { "VENDOR", "RENDERER", "VERSION", "EXTENSIONS" };

	log("%s(%s);\n", __FUNCTION__, arr[name-GL_VENDOR]);

	switch (name) {
	case GL_VENDOR:
		return (GLubyte *)"SoftGL Q1 Vendor";
	case GL_RENDERER:
		return (GLubyte *)"SoftGL Q1 Renderer";
	case GL_VERSION:
		return (GLubyte *)"SoftGL Q1 Miniport V1.0";
	case GL_EXTENSIONS:
		// GL_SGIS_multitexture glMTexCoord2fSGIS glSelectTextureSGIS
		// GL_ARB_multitexture: glGet(GL_ACTIVE_TEXTURE(0,1), GL_MAX_TEXTURE_COORDS(2), GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS(2))
		return (GLubyte *)"GL_EXT_vertex_array GL_SGIS_multitexture GL_EXT_texture_object"; // "GL_SGIS_multitexture "
		//"GL_EXT_shared_texture_palette" glColorTableEXT GL_SHARED_TEXTURE_PALETTE_EXT
	}
	return 0;
}

DLL_EXPORT void APIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glHint (GLenum target, GLenum mode) {
	static const char* arr[] = { "DONT_CARE", "FASTEST", "NICEST" };

	switch (target) {
	case GL_FOG_HINT:
		log("%s(FOG_HINT: %s);\n", __FUNCTION__, arr[mode-GL_DONT_CARE]);
		break;
	case GL_LINE_SMOOTH_HINT:
		log("%s(LINE_SMOOTH_HINT: %s);\n", __FUNCTION__, arr[mode-GL_DONT_CARE]);
		break;
	case GL_PERSPECTIVE_CORRECTION_HINT:
		log("%s(PERSPECTIVE_CORRECTION_HINT: %s);\n", __FUNCTION__, arr[mode-GL_DONT_CARE]);
		break;
	case GL_POINT_SMOOTH_HINT:
		log("%s(POINT_SMOOTH_HINT: %s);\n", __FUNCTION__, arr[mode-GL_DONT_CARE]);
		break;
	case GL_POLYGON_SMOOTH_HINT:
		log("%s(POLYGON_SMOOTH_HINT: %s);\n", __FUNCTION__, arr[mode-GL_DONT_CARE]);
		break;
	}

	//GL_PERSPECTIVE_CORRECTION_HINT
	//GL_NICEST: Perspective
	//GL_FASTEST: Linear
	//GL_DONT_CARE: Linear
}

DLL_EXPORT void APIENTRY glIndexMask (GLuint mask) {
	log("%s(0x%X);\n", __FUNCTION__, mask);
}

DLL_EXPORT void APIENTRY glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexd (GLdouble c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexdv (const GLdouble *c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexf (GLfloat c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexfv (const GLfloat *c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexi (GLint c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexiv (const GLint *c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexs (GLshort c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexsv (const GLshort *c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexub (GLubyte c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glIndexubv (const GLubyte *c) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glInitNames (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer) {
#	if LOG
	auto fmt = g_TexFormatList.find(format);
	log("%s(%s, %u);\n", __FUNCTION__, (fmt!=g_TexFormatList.end()?fmt->second:"UNKNOWN"), stride);
#	endif

	check_enum(format, GL_V2F, GL_V3F, GL_C4UB_V2F, GL_C4UB_V3F, GL_C3F_V3F, GL_N3F_V3F,
		GL_C4F_N3F_V3F, GL_T2F_V3F, GL_T4F_V4F, GL_T2F_C4UB_V3F, GL_T2F_C3F_V3F, GL_T2F_N3F_V3F,
		GL_T2F_C4F_N3F_V3F, GL_T4F_C4F_N3F_V4F);
	check_value(stride >= 0);

	static const GLenum UB = GL_UNSIGNED_BYTE, FL = GL_FLOAT;
	struct Data { int et, ec, en, st, sc, sv, tc, pc, pn, pv, s; };
	struct Table { int fmt; Data data; };
	static const Table table[] = {
		{GL_V2F,             { 0,0,0, 0, 0, 8, 0, 0, 0, 0, 8 } },
		{GL_V3F,             { 0,0,0, 0, 0,12, 0, 0, 0, 0,12 } },
		{GL_C4UB_V2F,        { 0,1,0, 0, 4, 8,UB, 0, 0, 4,12 } },
		{GL_C4UB_V3F,        { 0,1,0, 0, 4,12,UB, 0, 0, 4,16 } },
		{GL_C3F_V3F,         { 0,1,0, 0,12,12,FL, 0, 0,12,24 } },
		{GL_N3F_V3F,         { 0,0,1, 0, 0,12, 0, 0, 0,12,24 } },
		{GL_C4F_N3F_V3F,     { 0,1,1, 0,16,12,FL, 0,16,28,40 } },
		{GL_T2F_V3F,         { 1,0,0, 8, 0,12, 0, 0, 0, 8,20 } },
		{GL_T4F_V4F,         { 1,0,0,16, 0,16, 0, 0, 0,16,32 } },
		{GL_T2F_C4UB_V3F,    { 1,1,0, 8, 4,12,UB, 8, 0,12,24 } },
		{GL_T2F_C3F_V3F,     { 1,1,0, 8,12,12,FL, 8, 0,20,32 } },
		{GL_T2F_N3F_V3F,     { 1,0,1, 8, 0,12, 0, 0, 8,20,32 } },
		{GL_T2F_C4F_N3F_V3F, { 1,1,1, 8,16,12,FL, 8,24,36,48 } },
		{GL_T4F_C4F_N3F_V4F, { 1,1,1,16,16,16,FL,16,32,44,60 } }};

	cref<Data> data = table[format].data;

	//order:TCNV
	//if (format or stride is invalid) generate appropriate error
	//et,ec,en,st,sc,sv,tc,pc,pn,pv,s from table

	GLsizei str = stride ? stride : data.s;
	glDisableClientState(GL_EDGE_FLAG_ARRAY);
	glDisableClientState(GL_INDEX_ARRAY);
	if (data.et) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(data.st, GL_FLOAT, str, (GLubyte const*)pointer);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (data.ec) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(data.sc, data.tc, str, (GLubyte const*)pointer + data.pc);
	} else {
		glDisableClientState(GL_COLOR_ARRAY);
	}
	if (data.en) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, str, (GLubyte const*)pointer + data.pn);
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(data.sv, GL_FLOAT, str, (GLubyte const*)pointer + data.pv);
}

DLL_EXPORT GLboolean APIENTRY glIsEnabled (GLenum cap) {
#	if LOG
	auto it = g_EnableList.find(cap);
	log("%s(%s);\n", __FUNCTION__, (it!=g_EnableList.end()?it->second:"UNKNOWN"));
#	endif

	switch (cap) {
	case GL_ALPHA_TEST: return ctx->state->bAlphaTest;
	case GL_BLEND: return ctx->state->bBlend;
	case GL_CULL_FACE: return ctx->state->bCullFace;
	case GL_DEPTH_TEST: return ctx->state->bDepthTest;
	case GL_STENCIL_TEST: return ctx->state->bStencilTest;
	case GL_TEXTURE_2D: return FLAG_BIT(ctx->state->bTexture, ctx->state->texActive);
	}

	return false;
}

DLL_EXPORT GLboolean APIENTRY glIsList (GLuint list) {
	log("%s(%u);\n", __FUNCTION__, list);
	return false;
}

DLL_EXPORT GLboolean APIENTRY glIsTexture (GLuint texture) {
	log("%s(%u);\n", __FUNCTION__, texture);
	return ctx->state->textures.find(texture) != ctx->state->textures.end();
}

DLL_EXPORT void APIENTRY glLightModelf (GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightModelfv (GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightModeli (GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightModeliv (GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLighti (GLenum light, GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLineStipple (GLint factor, GLushort pattern) {
	log("%s(%i, 0x%X);\n", __FUNCTION__, factor, pattern);
}

DLL_EXPORT void APIENTRY glLineWidth (GLfloat width) {
	log("%s(%.3f);\n", __FUNCTION__, width);
}

DLL_EXPORT void APIENTRY glListBase (GLuint base) {
	log("%s(%u);\n", __FUNCTION__, base);
}

DLL_EXPORT void APIENTRY glLoadIdentity(void) {
	log("%s();\n", __FUNCTION__);

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		ctx->state->mProjection = m4::iden();
		break;
	case MM_MODELVIEW:
		ctx->state->mModelView = m4::iden();
		break;
	case MM_TEXTURE:
		ctx->state->mTexture = m4::iden();
		break;
	}

	ctx->state->bUpdateTotal = true;
}

DLL_EXPORT void APIENTRY glLoadMatrixd (const GLdouble *m) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLoadMatrixf (const GLfloat *m) {
	log("%s(.);\n", __FUNCTION__);
	m4 n(
		m[ 3], m[ 2], m[ 1], m[ 0],
		m[ 7], m[ 6], m[ 5], m[ 4],
		m[11], m[10], m[ 9], m[ 8],
		m[15], m[14], m[13], m[12]);
	switch (ctx->state->modeMatrix) {
	case MM_MODELVIEW: {
		ctx->state->mModelView = n;
		cref<m4> x = ctx->state->mModelView;
		log("|%.3f, %.3f, %.3f, %.3f|\n", x._00, x._01, x._02, x._03);
		log("|%.3f, %.3f, %.3f, %.3f|\n", x._10, x._11, x._12, x._13);
		log("|%.3f, %.3f, %.3f, %.3f|\n", x._20, x._21, x._22, x._23);
		log("|%.3f, %.3f, %.3f, %.3f|\n", x._30, x._31, x._32, x._33);
		ctx->state->bUpdateTotal = true;
		break; }
	case MM_PROJECTION:
		ctx->state->mProjection = n;
		ctx->state->bUpdateTotal = true;
		break;
	case MM_TEXTURE: ctx->state->mTexture = n; break;
	}
}

DLL_EXPORT void APIENTRY glLoadName (GLuint name) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glLogicOp (GLenum opcode) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMateriali (GLenum face, GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMatrixMode(GLenum mode) {
	switch (mode) {
	case GL_MODELVIEW:
		log("%s(GL_MODELVIEW);\n", __FUNCTION__);
		ctx->state->modeMatrix = MM_MODELVIEW;
		break;
	case GL_PROJECTION:
		log("%s(GL_PROJECTION);\n", __FUNCTION__);
		ctx->state->modeMatrix = MM_PROJECTION;
		break;
	case GL_TEXTURE:
		log("%s(GL_TEXTURE);\n", __FUNCTION__);
		ctx->state->modeMatrix = MM_TEXTURE;
		break;
	}
}

DLL_EXPORT void APIENTRY glMultMatrixd (const GLdouble *m) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glMultMatrixf (const GLfloat *m) {
	log("%s(.);\n", __FUNCTION__);
	m4 n(
		m[3], m[ 2], m[1], m[0],
		m[7], m[ 6], m[5], m[4],
		m[11], m[10], m[9], m[8],
		m[15], m[14], m[13], m[12]);
	switch (ctx->state->modeMatrix) {
	case MM_MODELVIEW:
		ctx->state->mProjection *= n;
		ctx->state->bUpdateTotal = true;
		break;
	case MM_PROJECTION:
		ctx->state->mModelView *= n;
		ctx->state->bUpdateTotal = true;
		break;
	case MM_TEXTURE: ctx->state->mTexture *= n; break;
	}
}

DLL_EXPORT void APIENTRY glNewList (GLuint list, GLenum mode) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3bv (const GLbyte *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curNormal = v4(1.f, nz, ny, nx);
}

DLL_EXPORT void APIENTRY glNormal3fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
	ctx->state->curNormal = v4(1.f, v[2], v[1], v[0]);
}

DLL_EXPORT void APIENTRY glNormal3i (GLint nx, GLint ny, GLint nz) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormal3sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer) {
	//type=GL_BYTE, GL_SHORT, GL_INT, GL_FLOAT, and GL_DOUBLE
	log("%s(%s, %u);\n", __FUNCTION__, g_DataTypeList[type-GL_BYTE], stride);
	ctx->state->normalArrayType = type;
	ctx->state->normalArrayStride = stride;
	ctx->state->normalArrayPointer = (GLubyte const*)pointer;
}

DLL_EXPORT void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	log("%s(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, left, right, bottom, top, zNear, zFar);

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		ctx->state->mProjection *= m4::ort(left, right, bottom, top, zNear, zFar);
		ctx->state->projNear = zNear;
		ctx->state->projFar = zFar;
		break;
	case MM_MODELVIEW:
		ctx->state->mModelView *= m4::ort(left, right, bottom, top, zNear, zFar);
		break;
	case MM_TEXTURE:
		ctx->state->mTexture *= m4::ort(left, right, bottom, top, zNear, zFar);
		break;
	}

	ctx->state->bUpdateTotal = true;
}

DLL_EXPORT void APIENTRY glPassThrough (GLfloat token) {
	log("%s(%.3f);\n", __FUNCTION__, token);
}

DLL_EXPORT void APIENTRY glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelStoref (GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelStorei (GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelTransferf (GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelTransferi (GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPointSize (GLfloat size) {
	log("%s(%.3f);\n", __FUNCTION__, size);
}

DLL_EXPORT void APIENTRY glPolygonMode (GLenum face, GLenum mode) {
	if (face == GL_NONE) {
		log("%s(GL_NONE, %s);\n", __FUNCTION__, g_PolygonModeList[mode-GL_POINT]);
		return;
	}
	log("%s(%s, %s);\n", __FUNCTION__, g_DrawBufferModeList[face-GL_FRONT_LEFT], g_PolygonModeList[mode-GL_POINT]);
}

DLL_EXPORT void APIENTRY glPolygonOffset (GLfloat factor, GLfloat units) {
	log("%s(%.3f, %.3f);\n", __FUNCTION__, factor, units);
}

DLL_EXPORT void APIENTRY glPolygonStipple (const GLubyte *mask) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPopAttrib (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPopClientAttrib (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPopMatrix(void) {
	log("%s();\n", __FUNCTION__);

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		ctx->state->mProjection = ctx->state->stackProjection.top();
		ctx->state->stackProjection.pop();
		if (ctx->state->mProjection._33 == 1.f) { // ortho
			ctx->state->projNear = (1.f + ctx->state->mProjection._23) / ctx->state->mProjection._22;
			ctx->state->projFar = -(1.f - ctx->state->mProjection._23) / ctx->state->mProjection._22;
		} else { // frustum
			ctx->state->projNear = ctx->state->mProjection._23 / (ctx->state->mProjection._22 - 1.f);
			ctx->state->projFar  = ctx->state->mProjection._23 / (ctx->state->mProjection._22 + 1.f);
		}
		ctx->state->bUpdateTotal = true;
		break;
	case MM_MODELVIEW:
		ctx->state->mModelView = ctx->state->stackModelView.top();
		ctx->state->stackModelView.pop();
		ctx->state->bUpdateTotal = true;
		break;
	case MM_TEXTURE:
		ctx->state->mTexture = ctx->state->stackTexture.top();
		ctx->state->stackTexture.pop();
		break;
	}
}

DLL_EXPORT void APIENTRY glPopName (void) {
	log("%s();\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPushAttrib (GLbitfield mask) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPushClientAttrib (GLbitfield mask) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glPushMatrix(void) {
	log("%s();\n", __FUNCTION__);

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION: ctx->state->stackProjection.push(ctx->state->mProjection); break;
	case MM_MODELVIEW:  ctx->state->stackModelView.push(ctx->state->mModelView); break;
	case MM_TEXTURE:    ctx->state->stackTexture.push(ctx->state->mTexture); break;
	}
}

DLL_EXPORT void APIENTRY glPushName (GLuint name) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2d (GLdouble x, GLdouble y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2f (GLfloat x, GLfloat y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2i (GLint x, GLint y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2s (GLshort x, GLshort y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos2sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3i (GLint x, GLint y, GLint z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos3sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRasterPos4sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glReadBuffer (GLenum mode) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectiv (const GLint *v1, const GLint *v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRectsv (const GLshort *v1, const GLshort *v2) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT GLint APIENTRY glRenderMode (GLenum mode) {
	log("%s(.);\n", __FUNCTION__);
	return 0;
}

DLL_EXPORT void APIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	log("%s(%.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, angle, x, y, z);

	m4* m = nullptr;

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		m = &ctx->state->mProjection;
		break;
	case MM_MODELVIEW:
		m = &ctx->state->mModelView;
		break;
	case MM_TEXTURE:
		m = &ctx->state->mTexture;
		break;
	}

	*m *= m4::rot(angle, x, y, z);

	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_00, m->_01, m->_02, m->_03);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_10, m->_11, m->_12, m->_13);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_20, m->_21, m->_22, m->_23);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_30, m->_31, m->_32, m->_33);

	ctx->state->bUpdateTotal = true;
}


DLL_EXPORT void APIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {
	log("%s(%.3f, %.3f, %.3f);\n", __FUNCTION__, x, y, z);

	m4* m = nullptr;

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		m = &ctx->state->mProjection;
		break;
	case MM_MODELVIEW:
		m = &ctx->state->mModelView;
		break;
	case MM_TEXTURE:
		m = &ctx->state->mTexture;
		break;
	}

	*m *= m4::sca(x, y, z);

	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_00, m->_01, m->_02, m->_03);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_10, m->_11, m->_12, m->_13);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_20, m->_21, m->_22, m->_23);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_30, m->_31, m->_32, m->_33);

	ctx->state->bUpdateTotal = true;
}

DLL_EXPORT void APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height) {
	log("%s(x:%i, y:%i, width:%u, height:%u);\n", __FUNCTION__, x, y, width, height);
}

DLL_EXPORT void APIENTRY glSelectBuffer (GLsizei size, GLuint *buffer) {
	log("%s(%u);\n", __FUNCTION__, size);
}

DLL_EXPORT void APIENTRY glShadeModel (GLenum mode) {
	static const char* arr[] = { "FLAT", "SMOOTH" };
	log("%s(%s);\n", __FUNCTION__, arr[mode-GL_FLAT]);
}

DLL_EXPORT void APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask) {
	log("%s(%s, %i, 0x%X);\n", __FUNCTION__, g_FuncList[func-GL_NEVER], ref, mask);
}

DLL_EXPORT void APIENTRY glStencilMask (GLuint mask) {
	log("%s(0x%X);\n", __FUNCTION__, mask);
}

DLL_EXPORT void APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
	auto ifail = g_StencilOpList.find(fail);
	auto izfail = g_StencilOpList.find(zfail);
	auto izpass = g_StencilOpList.find(zpass);
	log("%s(%s, %s, %s);\n", __FUNCTION__, (ifail!=g_StencilOpList.end()?ifail->second:"UNKNOWN"),
		(izfail!=g_StencilOpList.end()?izfail->second:"UNKNOWN"), (izpass!=g_StencilOpList.end()?izpass->second:"UNKNOWN"));
}

DLL_EXPORT void APIENTRY glTexCoord1d (GLdouble s) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1f (GLfloat s) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1i (GLint s) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1s (GLshort s) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord1sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2d (GLdouble s, GLdouble t) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2f (GLfloat s, GLfloat t) {
	log("\t%s(%.6f, %.6f);\n", __FUNCTION__, s, t);
	ctx->state->curTexCoord[0] = v2(t, s);
}

DLL_EXPORT void APIENTRY glTexCoord2fv (const GLfloat *v) {
	log("\t%s(%.6f, %.6f);\n", __FUNCTION__, v[0], v[1]);
	ctx->state->curTexCoord[0] = v2(v[1], v[0]);
}

DLL_EXPORT void APIENTRY glTexCoord2i (GLint s, GLint t) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2s (GLshort s, GLshort t) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord2sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3i (GLint s, GLint t, GLint r) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord3sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4fv (const GLfloat *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoord4sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	//size=1,2,3,4
	//type=GL_SHORT, GL_INT, GL_FLOAT, GL_DOUBLE
	log("%s(%i, %s, %u);\n", __FUNCTION__, size, g_DataTypeList[type-GL_BYTE], stride);
	ctx->state->texcoordArraySize = size;
	ctx->state->texcoordArrayType = type;
	ctx->state->texcoordArrayStride = stride;
	ctx->state->texcoordArrayPointer = (GLubyte const*)pointer;
}

DLL_EXPORT void APIENTRY glTexCoordPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	return glTexCoordPointer(size, type, stride, pointer);
}

DLL_EXPORT void APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param) {
	log("%s(%s, %s, %s);\n", __FUNCTION__,
		(GL_TEXTURE_ENV?"GL_TEXTURE_ENV":"UNKNOWN"),
		(GL_TEXTURE_ENV_MODE?"GL_TEXTURE_ENV_MODE":"UNKNOWN"),
		(param==GL_MODULATE?"GL_MODULATE":param==GL_DECAL?"GL_DECAL":param==GL_BLEND?"GL_BLEND":param==GL_REPLACE?"GL_REPLACE":"UNKNOWN"));

	switch (pname) {
	case GL_TEXTURE_ENV_MODE:
		switch ((int)param) {
		case GL_MODULATE: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_MODULATE; break;
		case GL_REPLACE: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_REPLACE; break;
		case GL_DECAL: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_DECAL; break;
		case GL_BLEND: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_BLEND; break;
		}
		break;
	}
}

DLL_EXPORT void APIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);

	switch (pname) {
	case GL_TEXTURE_ENV_COLOR:
		ctx->state->texEnvColor[ctx->state->texActive] = FRGBA(params[0], params[1], params[2], params[3]);
		break;
	}
}

DLL_EXPORT void APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
	//target = GL_TEXTURE_ENV
	//pname = GL_TEXTURE_ENV_MODE
	//param = GL_MODULATE, GL_REPLACE, GL_DECAL, GL_BLEND

	switch (pname) {
	case GL_TEXTURE_ENV_MODE:
		switch (param) {
		case GL_MODULATE: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_MODULATE; break;
		case GL_REPLACE: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_REPLACE; break;
		case GL_DECAL: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_DECAL; break;
		case GL_BLEND: ctx->state->texEnvMode[ctx->state->texActive] = TEX_ENV_BLEND; break;
		}
		break;
	}
}

DLL_EXPORT void APIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
#	if LOG
	auto ifmt = g_TexInternalFormatList.find(internalformat);
	auto fmt = g_TexInternalFormatList.find(format);

	log("%s(%s, lvl=%i, ifm=%s, w=%u, b=%i, fmt=%s, typ=%s);\n", __FUNCTION__,
		(target==GL_TEXTURE_1D ? "GL_TEXTURE_1D" : "UNKNOWN"),
		level,
		(ifmt!=g_TexInternalFormatList.end()?ifmt->second:"UNKNOWN"),
		width, border,
		(fmt!=g_TexFormatList.end() ? fmt->second : "UNKNOWN"),
		g_DataTypeList[type-GL_BYTE]);
#	endif
}

DLL_EXPORT void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
	//internalformat = 1; 2; 3; 4; A|I|L,4,8,12,16; LA,4/4,6/2,8/8,12/4,12/12,16/16; RGB,4,5,8,10,12,16,3/3/2; RGBA,2,4,5/1,8,10/2,12,16;
	//format = COLOR_INDEX, R, G, B, A, L, LA, RGB, RGBA, BGR_EXT, BGRA_EXT
	//type = UBYTE, BYTE, BITMAP, USHORT, SHORT, UINT, INT, FLOAT

	check_enum(target, GL_TEXTURE_2D);
	check_value(level >= 0);
	check_value(width > 0);
	check_value(height > 0);
	check_value(border == 0);
	check_enum(type, GL_UNSIGNED_BYTE, GL_BYTE);
	check_value(pixels != nullptr);
	check_value(isPowerOfTwo(width));
	check_value(isPowerOfTwo(height));

#	if LOG
	auto ifmt = g_TexInternalFormatList.find(internalformat);
	auto fmt = g_TexFormatList.find(format);
	const char* pif = ifmt!=g_TexInternalFormatList.end() ? ifmt->second : "UNKNOWN";
	const char* pf = fmt!=g_TexFormatList.end() ? fmt->second : "UNKNOWN";

	log("%s(%s, %i, %s, %u, %u, %i, %s, %s);\n", __FUNCTION__,
		(target==GL_TEXTURE_2D ? "GL_TEXTURE_2D" : "UNKNOWN"),
		level,
		pif,
		width, height, border,
		pf,
		g_DataTypeList[type-GL_BYTE]);
#	endif

	/* DEBUG: dump texture
	if (ctx->state->tex2d[ctx->state->texActive] != 27) {
		std::ostringstream oss;
		oss << "img\\";
		if (ctx->state->tex2d[ctx->state->texActive] < 10)
			oss << "00";
		else if (ctx->state->tex2d[ctx->state->texActive] < 100)
			oss << "0";

		oss << ctx->state->tex2d[ctx->state->texActive] << level << ".png";

		png_t png;
		if (!png_init(0, 0) && !png_open_file_write(&png, oss.str().c_str())) {
			switch (format) {
			case 1:	case GL_ALPHA: case GL_LUMINANCE: case GL_INTENSITY:
				png_set_data(&png, width, height, 8, PNG_GREYSCALE, (unsigned char*)pixels); break;
			case 2: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8:
				png_set_data(&png, width, height, 8, PNG_GREYSCALE_ALPHA, (unsigned char*)pixels); break;
			case 3: case GL_RGB: case GL_RGB8:
				png_set_data(&png, width, height, 8, PNG_TRUECOLOR, (unsigned char*)pixels); break;
			case 4: case GL_RGBA: case GL_RGBA8:
				png_set_data(&png, width, height, 8, PNG_TRUECOLOR_ALPHA, (unsigned char*)pixels); break;
			default: assert(0);
			}
			png_close_file(&png);
		}
	}
	//*/

	//if (level != 0) return; //DEBUG

	Texture* t;
	GLuint id = ctx->state->tex2d[ctx->state->texActive];

	if (!width || !height || !pixels) {
		return;
	}

	auto it = ctx->state->textures.find(id);
	
	if (it == ctx->state->textures.end()) {
		auto[i, ok] = ctx->state->textures.insert(std::pair{id, std::make_shared<Texture>(id)});
		if (ok) { it = i; }
	}
	
	if (it == ctx->state->textures.end()) return;
	t = it->second.get();

	Layer* l = t->addLayer(level, width, height, format, internalformat, 4, (GLbyte const*)pixels);
	if (t) {
		t->setMinFilter(ctx->state->texMinFilter[ctx->state->texActive]);
		t->setMagFilter(ctx->state->texMagFilter[ctx->state->texActive]);
	}
	if (l) {
		l->setWrapS(ctx->state->texWrapS[ctx->state->texActive]);
		l->setWrapT(ctx->state->texWrapT[ctx->state->texActive]);
	}
}

DLL_EXPORT void APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param) {
	static const char* arr[] = {
		"GL_TEXTURE_MAG_FILTER", "GL_TEXTURE_MIN_FILTER", "GL_TEXTURE_WRAP_S", "GL_TEXTURE_WRAP_T"
	};

	static const std::map<int, const char*> filters = helper::map_list_of<int, const char*>
		MAP_LIST_ELEM(GL_CLAMP)
		MAP_LIST_ELEM(GL_REPEAT)
		MAP_LIST_ELEM(GL_NEAREST)
		MAP_LIST_ELEM(GL_LINEAR)
		MAP_LIST_ELEM(GL_NEAREST_MIPMAP_NEAREST)
		MAP_LIST_ELEM(GL_LINEAR_MIPMAP_NEAREST)
		MAP_LIST_ELEM(GL_NEAREST_MIPMAP_LINEAR)
		MAP_LIST_ELEM(GL_LINEAR_MIPMAP_LINEAR);

#	if LOG
	auto itf = filters.find(param);
	const char* sparam = itf != filters.end() ? itf->second : "UNKNOWN";

	log("%s(%s, %s, %s);\n", __FUNCTION__,
		(target==GL_TEXTURE_2D?"GL_TEXTURE_2D":target==GL_TEXTURE_1D?"GL_TEXTURE_1D":"UNKNOWN"),
		arr[pname-GL_TEXTURE_MAG_FILTER],
		sparam);
#	endif

	switch (pname) {
	case GL_TEXTURE_MAG_FILTER:
		switch ((GLenum)param) {
		case GL_NEAREST:                ctx->state->texMagFilter[ctx->state->texActive] = TEX_FILTER_NEAREST; break;
		case GL_LINEAR:                 ctx->state->texMagFilter[ctx->state->texActive] = TEX_FILTER_LINEAR; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		switch ((GLenum)param) {
		case GL_NEAREST:                ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST; break;
		case GL_LINEAR:                 ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR; break;
		case GL_NEAREST_MIPMAP_NEAREST: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST_MIPMAP_NEAREST; break;
		case GL_LINEAR_MIPMAP_NEAREST:  ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR_MIPMAP_NEAREST; break;
		case GL_NEAREST_MIPMAP_LINEAR:  ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST_MIPMAP_LINEAR; break;
		case GL_LINEAR_MIPMAP_LINEAR:   ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR_MIPMAP_LINEAR; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_WRAP_S: // GL_CLAMP/GL_REPEAT/GL_CLAMP_TO_EDGE
		switch ((GLenum)param) {
		case GL_CLAMP:                  ctx->state->texWrapS[ctx->state->texActive] = TEX_WRAP_CLAMP; break;
		case GL_REPEAT:                 ctx->state->texWrapS[ctx->state->texActive] = TEX_WRAP_REPEAT; break;
		case GL_CLAMP_TO_EDGE:          ctx->state->texWrapS[ctx->state->texActive] = TEX_WRAP_CLAMP_TO_EDGE; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_WRAP_T: // GL_CLAMP/GL_REPEAT/GL_CLAMP_TO_EDGE
		switch ((GLenum)param) {
		case GL_CLAMP:                  ctx->state->texWrapT[ctx->state->texActive] = TEX_WRAP_CLAMP; break;
		case GL_REPEAT:                 ctx->state->texWrapT[ctx->state->texActive] = TEX_WRAP_REPEAT; break;
		case GL_CLAMP_TO_EDGE:          ctx->state->texWrapT[ctx->state->texActive] = TEX_WRAP_CLAMP_TO_EDGE; break;
		default: assert(0);
		}
		break;
	}
}

DLL_EXPORT void APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param) {
	//log("%s(.);\n", __FUNCTION__);
	//target = GL_TEXTURE_1D, GL_TEXTURE_2D
	//pname = GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
	//GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	//GL_CLAMP, GL_REPEAT, GL_CLAMP_TO_EDGE
	static const char* arr[] = {
		"GL_TEXTURE_MAG_FILTER", "GL_TEXTURE_MIN_FILTER", "GL_TEXTURE_WRAP_S", "GL_TEXTURE_WRAP_T"
	};

	static const std::map<int, const char*> filters = helper::map_list_of<int, const char*>
		MAP_LIST_ELEM(GL_CLAMP)
		MAP_LIST_ELEM(GL_REPEAT)
		MAP_LIST_ELEM(GL_NEAREST)
		MAP_LIST_ELEM(GL_LINEAR)
		MAP_LIST_ELEM(GL_NEAREST_MIPMAP_NEAREST)
		MAP_LIST_ELEM(GL_LINEAR_MIPMAP_NEAREST)
		MAP_LIST_ELEM(GL_NEAREST_MIPMAP_LINEAR)
		MAP_LIST_ELEM(GL_LINEAR_MIPMAP_LINEAR);

#	if LOG
	auto it = filters.find(param);
	const char* sparam = it != filters.end() ? it->second : "UNKNOWN";

	log("%s(%s, %s, %s);\n", __FUNCTION__,
		(target==GL_TEXTURE_2D?"GL_TEXTURE_2D":target==GL_TEXTURE_1D?"GL_TEXTURE_1D":"UNKNOWN"),
		arr[pname-GL_TEXTURE_MAG_FILTER],
		sparam);
#	endif

	switch (pname) {
	case GL_TEXTURE_MAG_FILTER:
		switch (param) {
		case GL_NEAREST: ctx->state->texMagFilter[ctx->state->texActive] = TEX_FILTER_NEAREST; break;
		case GL_LINEAR: ctx->state->texMagFilter[ctx->state->texActive] = TEX_FILTER_LINEAR; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		switch (param) {
		case GL_NEAREST: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST; break;
		case GL_LINEAR: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR; break;
		case GL_NEAREST_MIPMAP_NEAREST: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST_MIPMAP_NEAREST; break;
		case GL_LINEAR_MIPMAP_NEAREST: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR_MIPMAP_NEAREST; break;
		case GL_NEAREST_MIPMAP_LINEAR: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_NEAREST_MIPMAP_LINEAR; break;
		case GL_LINEAR_MIPMAP_LINEAR: ctx->state->texMinFilter[ctx->state->texActive] = TEX_FILTER_LINEAR_MIPMAP_LINEAR; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_WRAP_S: // GL_CLAMP/GL_REPEAT/GL_CLAMP_TO_EDGE
		switch (param) {
		case GL_CLAMP: ctx->state->texWrapS[ctx->state->texActive] = TEX_WRAP_CLAMP; break;
		case GL_REPEAT: ctx->state->texWrapS[ctx->state->texActive] = TEX_WRAP_REPEAT; break;
		default: assert(0);
		}
		break;
	case GL_TEXTURE_WRAP_T: // GL_CLAMP/GL_REPEAT/GL_CLAMP_TO_EDGE
		switch (param) {
		case GL_CLAMP: ctx->state->texWrapT[ctx->state->texActive] = TEX_WRAP_CLAMP; break;
		case GL_REPEAT: ctx->state->texWrapT[ctx->state->texActive] = TEX_WRAP_REPEAT; break;
		default: assert(0);
		}
		break;
	}
}

DLL_EXPORT void APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
#	if LOG
	auto fmt = g_TexFormatList.find(format);
	log("%s(%s, %i, %i, %u, %s, %s);\n", __FUNCTION__,
		(target==GL_TEXTURE_2D?"GL_TEXTURE_2D":"UNKNOWN"),
		level, xoffset, width,
		(fmt!=g_TexFormatList.end()?fmt->second:"UNKNOWN"),
		g_DataTypeList[type-GL_BYTE]);
#	endif
}

DLL_EXPORT void APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
#	if LOG
	auto fmt = g_TexFormatList.find(format);
	log("%s(%s, %i, %i, %i, %u, %u, %s, %s);\n", __FUNCTION__,
		(target==GL_TEXTURE_2D?"GL_TEXTURE_2D":"UNKNOWN"),
		level, xoffset, yoffset, width, height,
		(fmt!=g_TexFormatList.end()?fmt->second:"UNKNOWN"),
		g_DataTypeList[type-GL_BYTE]);
#	endif

	check_enum(target, GL_TEXTURE_2D);
	check_value(level >= 0);
	check_value(xoffset >= 0);
	check_value(yoffset >= 0);
	check_value(width > 0);
	check_value(height > 0);
	check_enum(type, GL_BYTE, GL_UNSIGNED_BYTE);

	if (level != 0)
		return;
	/* DEBUG
	if (ctx->state->tex2d[ctx->state->texActive] != 27) {
		std::ostringstream oss;
		oss << "img\\";
		if (ctx->state->tex2d[ctx->state->texActive] < 10)
			oss << "00";
		else if (ctx->state->tex2d[ctx->state->texActive] < 100)
			oss << "0";

		oss << ctx->state->tex2d[ctx->state->texActive] << level << ".png";

		png_t png;
		if (!png_init(0, 0) && !png_open_file_write(&png, oss.str().c_str())) {
			switch (format) {
			case GL_LUMINANCE: png_set_data(&png, width, height, 8, PNG_GREYSCALE, (unsigned char*)pixels); break;
			case GL_RGB: png_set_data(&png, width, height, 8, PNG_TRUECOLOR, (unsigned char*)pixels); break;
			case GL_RGBA: png_set_data(&png, width, height, 8, PNG_TRUECOLOR_ALPHA, (unsigned char*)pixels); break;
			}
			png_close_file(&png);
		}
	}
	//*/
	GLuint id = ctx->state->tex2d[ctx->state->texActive];

	if (!width || !height || !pixels) {
		return;
	}

	if (!id) {
		return;
	}

	auto it = ctx->state->textures.find(id);
	if (it == ctx->state->textures.end()) {
		return;
	}

	std::shared_ptr<Texture>& t = it->second;
	if (!t) return;
	t = t->shared_copy();
	t->makeLayerCopy(level);
	t->updateLayer(level, xoffset, yoffset, width, height, format, ctx->state->pixelUnpackAlignment, static_cast<GLubyte const*>(pixels), id);
}

DLL_EXPORT void APIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z) {
	log("%s(%.3f, %.3f, %.3f);\n", __FUNCTION__, x, y, z);
}

DLL_EXPORT void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	log("%s(%.3f, %.3f, %.3f);\n", __FUNCTION__, x, y, z);

	m4* m = nullptr;

	switch (ctx->state->modeMatrix) {
	case MM_PROJECTION:
		m = &ctx->state->mProjection;
		break;
	case MM_MODELVIEW:
		m = &ctx->state->mModelView;
		break;
	case MM_TEXTURE:
		m = &ctx->state->mTexture;
		break;
	}

	*m *= m4::tra(x, y, z);

	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_00, m->_01, m->_02, m->_03);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_10, m->_11, m->_12, m->_13);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_20, m->_21, m->_22, m->_23);
	log("|%.3f, %.3f, %.3f, %.3f|\n", m->_30, m->_31, m->_32, m->_33);

	ctx->state->bUpdateTotal = true;
}

DLL_EXPORT void APIENTRY glVertex2d (GLdouble x, GLdouble y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex2dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex2f(const GLfloat x, const GLfloat y) {
	log("\t%s(%.3f, %.3f);\n", __FUNCTION__, x, y);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(1.f, 0.f, y, x), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex2fv(const GLfloat *v) {
	log("\t%s(%.3f, %.3f);\n", __FUNCTION__, v[0], v[1]);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(1.f, 0.f, v[1], v[0]), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex2i (GLint x, GLint y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex2iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex2s (GLshort x, GLshort y) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex2sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3f(const GLfloat x, const GLfloat y, const GLfloat z) {
	log("\t%s(%.3f, %.3f, %.3f);\n", __FUNCTION__, x, y, z);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(1.f, z, y, x), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex3fv(const GLfloat *v) {
	log("\t%s(%.3f, %.3f, %.3f);\n", __FUNCTION__, v[0], v[1], v[2]);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(1.f, v[2], v[1], v[0]), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex3i (GLint x, GLint y, GLint z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3s (GLshort x, GLshort y, GLshort z) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex3sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4dv (const GLdouble *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4f(const GLfloat x, const GLfloat y, const GLfloat z, const GLfloat w) {
	log("\t%s(%.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, x, y, z, w);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(x, y, z, w), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex4fv(const GLfloat *v) {
	log("\t%s(%.3f, %.3f, %.3f, %.3f);\n", __FUNCTION__, v[0], v[1], v[2], v[3]);
	t4 t;
	switch (ctx->state->bTexture) {
	case 3: t = t4(ctx->state->curTexCoord[1], ctx->state->curTexCoord[0]); break;
	case 2: t = t4(ctx->state->curTexCoord[1], v2(0.f, 0.f)); break;
	case 1: t = t4(ctx->state->curTexCoord[0]); break;
	default: t = t4(0.f, 0.f, 0.f, 0.f);
	}
	glVertex(v4(v[0], v[1], v[2], v[3]), ctx->state->curColor, t, ctx->state->curNormal);
}

DLL_EXPORT void APIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4iv (const GLint *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertex4sv (const GLshort *v) {
	log("%s(.);\n", __FUNCTION__);
}

DLL_EXPORT void APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	// size=2,3,4
	// type=GL_SHORT,GL_INT,GL_FLOAT,GL_DOUBLE
	// stride=bytes
	log("%s(%i, %s, %u);\n", __FUNCTION__, size, g_DataTypeList[type-GL_BYTE], stride);
	ctx->state->vertexArraySize = size;
	ctx->state->vertexArrayType = type;
	ctx->state->vertexArrayStride = stride;
	ctx->state->vertexArrayPointer = (GLubyte const*)pointer;
}

DLL_EXPORT void APIENTRY glVertexPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	return glVertexPointer(size, type, stride, pointer);
}

DLL_EXPORT void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	log("%s(%i, %i, %u, %u);\n", __FUNCTION__, x, y, width, height);

	ctx->buf.vx_ = x;
	ctx->buf.vy_ = y;
	ctx->buf.vw_ = width;
	ctx->buf.vh_ = height;
	ctx->state->vpX = x;
	ctx->state->vpY = y;
	ctx->state->vpWidth = width;
	ctx->state->vpHeight = height;
	ctx->state->ndc2sc3 = _mm_set_ps(f32(width) * .5f, f32(height) * .5f, (ctx->state->depthFar - ctx->state->depthNear) * .5f, 1.f);
}

// "GL_SGIS_multitexture"

#define TEXTURE0_SGIS	0x835E
#define TEXTURE1_SGIS	0x835F

static const char* g_TexListSGIS[] = {
	"TEXTURE0_SGIS", "TEXTURE1_SGIS"
};

void APIENTRY glMTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t) { // = glMultiTexCoord2f
	log("\t%s(%s, %.3f, %.3f);\n", __FUNCTION__, g_TexListSGIS[target-TEXTURE0_SGIS], s, t);
	int texActive = target - TEXTURE0_SGIS;
	if (!ctx) return;
	ctx->state->curTexCoord[texActive] = v2(t, s);
}

void APIENTRY glSelectTextureSGIS(GLenum target) { // = glActiveTexture
	//glEnable, glBindTexture, glTexParameter, glTexEnv
	log("%s(%s);\n", __FUNCTION__, g_TexListSGIS[target-TEXTURE0_SGIS]);
	if (!ctx) return;
	ctx->state->texActive = target - TEXTURE0_SGIS;
}

DLL_EXPORT void APIENTRY glActiveTexture(GLenum texture) {
	log("%s(%s);\n", __FUNCTION__, g_TexList[texture - GL_TEXTURE0]);
	if (!ctx) return;
	ctx->state->texActive = texture - GL_TEXTURE0;
}

DLL_EXPORT void APIENTRY glClientActiveTexture(GLenum texture) {
	log("%s(%s);\n", __FUNCTION__, g_TexList[texture - GL_TEXTURE0]);
	//glClientActiveTextureARB -> glTexCoordPointer, glEnableClientState, glDisableClientState
	if (!ctx) return;
	ctx->state->clientTexActive = texture - GL_TEXTURE0;
}

DLL_EXPORT void APIENTRY glMultiTexCoord1f(GLenum target, GLfloat s) {
	log("%s(%s, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0], s);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0] = v2(0.f, s);
}

DLL_EXPORT void APIENTRY glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t) {
	log("%s(%s, %.3f, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0], s, t);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0] = v2(t, s);
}

DLL_EXPORT void APIENTRY glMultiTexCoord1fv(GLenum target, const GLfloat* v) {
	log("%s(%s, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0], v[0]);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0] = v2(0.f, v[0]);
}

DLL_EXPORT void APIENTRY glMultiTexCoord2fv(GLenum target, const GLfloat* v) {
	log("%s(%s, %.3f, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0], v[0], v[1]);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0] = v2(v[1], v[0]);
}

#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1

static const char* g_TexListARB[] = {
	"TEXTURE0_ARB", "TEXTURE1_ARB"
};

DLL_EXPORT void APIENTRY glActiveTextureARB(GLenum texture) {
	log("%s(%s);\n", __FUNCTION__, g_TexList[texture - GL_TEXTURE0_ARB]);
	if (!ctx) return;
	ctx->state->texActive = texture - GL_TEXTURE0_ARB;
}

DLL_EXPORT void APIENTRY glClientActiveTextureARB(GLenum texture) {
	log("%s(%s);\n", __FUNCTION__, g_TexList[texture - GL_TEXTURE0_ARB]);
	//glClientActiveTextureARB -> glTexCoordPointer, glEnableClientState, glDisableClientState
	if (!ctx) return;
	ctx->state->clientTexActive = texture - GL_TEXTURE0_ARB;
}

DLL_EXPORT void APIENTRY glMultiTexCoord1fARB(GLenum target, GLfloat s) {
	log("%s(%s, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0_ARB], s);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0_ARB] = v2(0.f, s);
}

DLL_EXPORT void APIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
	log("%s(%s, %.3f, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0_ARB], s, t);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0_ARB] = v2(t, s);
}

DLL_EXPORT void APIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat* v) {
	log("%s(%s, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0_ARB], v[0]);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0_ARB] = v2(0.f, v[0]);
}

DLL_EXPORT void APIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat* v) {
	log("%s(%s, %.3f, %.3f);\n", __FUNCTION__, g_TexList[target - GL_TEXTURE0_ARB], v[0], v[1]);
	if (!ctx) return;
	ctx->state->curTexCoord[target - GL_TEXTURE0_ARB] = v2(v[1], v[0]);
}

#pragma endregion

#ifdef __cplusplus
}
#endif
