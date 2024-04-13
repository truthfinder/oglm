//#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <gl/gl.h>
#include <gl/glu.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

//#include <png.h>
//#include <jpeglib.h>

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <string_view>

#include <intrin.h>

#include "../utils/types.h"
#include "../utils/mx.h"
#include "../utils/timer.h"
#include "../utils/pnglite.h"

#ifdef UNICODE
#define tstricmp _wcsicmp
#else
#define tstricmp stricmp
#endif

/*
#pragma comment (lib, "jpeg_mt.lib")

#ifdef _DEBUG
#pragma comment (lib, "libpngd.lib")
#pragma comment (lib, "zlibd.lib")
#else
#pragma comment (lib, "libpng.lib")
#pragma comment (lib, "zlib.lib")
#endif
//*/
//#pragma comment (lib, "opengl32")
//#pragma comment (lib, "glu32")

///

class Image {
public:
	Image()
		: data_(nullptr)
		, width_(0)
		, height_(0)
		, stride_(0)
		, size_(0)
		, size_bytes_(0)
		, bpp_(0)
		, BPP_(0)
		, align_(0)
		, components_(0)
	{
	}

	~Image() {
		Close();
	}

	bool Open(const tstr& path, bool revy = true) {
		TCHAR c = path[0];
		if (path.size() > 4) {
			if (!tstricmp(path.substr(path.size()-4).c_str(), _T(".bmp"))) {
				return LoadDib(path.c_str());
			} else if (!tstricmp(path.substr(path.size()-4).c_str(), _T(".png"))) {
				return LoadPng(path.c_str(), revy);
			} else if (!tstricmp(path.substr(path.size()-4).c_str(), _T(".jpg"))) {
				return LoadJpeg(path.c_str());
			}
		} else if (path.size() > 5) {
			if (!tstricmp(path.substr(path.size()-4).c_str(), _T(".jpeg"))) {
				return LoadJpeg(path.c_str());
			}
		}
		return false;
	}

	void Close() {
		if (data_) {
			delete [] data_;
			data_ = nullptr;
		}
		width_ = 0;
		height_ = 0;
		stride_ = 0;
		size_ = 0;
		size_bytes_ = 0;
		bpp_ = 0;
		BPP_ = 0;
		align_ = 0;
		components_ = 0;
	}

	const void* Data() const { return data_; }
	i32 Width() const { return width_; }
	i32 Height() const { return height_; }
	i32 bpp() const { return bpp_; }
	i32 BPP() const { return BPP_; }
	i32 Size() const { return size_; }
	i32 SizeBytes() const { return size_bytes_; }
	i32 Stride() const { return stride_; }
	i32 Align() const { return align_; }
	i32 Components() const { return components_; }
	bool IsOpen() const { return data_ != nullptr; }

private:
	bool LoadDib(LPCTSTR path) {
		HBITMAP hBitmap = (HBITMAP)::LoadImage(0, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if (!hBitmap) {
			//HandleError();
			return false;
		}
		BITMAP bm;
		GetObject(hBitmap, sizeof(bm), &bm);
		data_ = new u8 [bm.bmHeight*bm.bmWidthBytes];
		if (!data_)
			return false;
		HDC hDC = CreateDC(_T("DISPLAY"), 0, 0, 0);
		HDC hCDC = CreateCompatibleDC(hDC);
		SelectObject(hCDC, hBitmap);
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = bm.bmWidth;
		bmi.bmiHeader.biHeight = -bm.bmHeight;
		bmi.bmiHeader.biSizeImage = bm.bmHeight*bm.bmWidthBytes;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = bm.bmBitsPixel;
		bmi.bmiHeader.biCompression = BI_RGB;
		i32 res = GetDIBits(hCDC, hBitmap, 0, bm.bmHeight, data_, &bmi, DIB_RGB_COLORS);
		DeleteDC(hCDC);
		DeleteDC(hDC);
		if (res != bm.bmHeight) {
			Close();
			return false;
		}
		width_ = bm.bmWidth;
		height_ = bm.bmHeight;
		size_ = width_ * height_;
		bpp_ = bm.bmBitsPixel;
		BPP_ = bpp_ >> 3;
		stride_ = width_ * BPP_;
		size_bytes_ = size_ * BPP_;
		align_ = 4;
		return IsOpen();
	}

	bool SaveDib(LPCTSTR path) {
		if (!size_ || !data_)
			return false;

		HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS/*|TRUNCATE_EXISTING*/, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING, 0);

		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		//BITMAPINFO bmi;
		BITMAPFILEHEADER bmfh = {0};
		bmfh.bfType = 0x4D42;
		bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmfh.bfSize = bmfh.bfOffBits + size_;
		BITMAPINFOHEADER bmih = {0};
		bmih.biBitCount = 32;
		bmih.biCompression = BI_RGB;
		bmih.biPlanes = 1;
		bmih.biSize = sizeof(BITMAPINFOHEADER);
		bmih.biWidth = width_;
		bmih.biHeight = height_;
		bmih.biSize = size_bytes_;
		DWORD written;
		WriteFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER), &written, 0);
		WriteFile(hFile, &bmih, sizeof(BITMAPINFOHEADER), &written, 0);
		WriteFile(hFile, data_, size_, &written, 0);

		CloseHandle(hFile);
		return true;
	}

	bool LoadPng(LPCTSTR path, bool revy = true) {
		png_t png;
		if (png_wopen_file_read(&png, path)) // use mmap
			return false;

		size_t datalen = png.width * png.height * png.bpp;
		std::unique_ptr<u8> buf(new u8[datalen]);
		png_get_data(&png, buf.get());
		png_close_file(&png);

		if (!png.width || !png.height || !png.bpp)
			return false;

		assert(png.depth == 8);
		if (png.depth != 8)
			return false;

		//typedef int T[16][16];
		//T& m = **(T**)&buf.get();

		components_ = png.bpp;
		bpp_ = png.bpp * png.depth;
		BPP_ = png.bpp;
		width_ = png.width;
		height_ = png.height;
		size_ = png.width * png.height;
		stride_ = png.width * png.bpp;
		align_ = 1;
		stride_ = (stride_ + (align_ - 1)) & ~(align_ - 1);
		assert((stride_ & 3) == 0);
		size_bytes_ = stride_ * png.height;

		data_ = new u8[size_bytes_];
		u8* ptr = data_;
		assert(png.bpp == 1 || png.bpp == 3 || png.bpp == 4);
		assert(size_bytes_ == datalen); //review, png.png_datalen->aligned to 4
		if (!revy) {
			memcpy(data_, buf.get(), datalen);
		} else {
			for (int i = 0; i < png.height; ++i) {
				memcpy(data_ + stride_ * i, buf.get() + stride_ * (png.height - 1 - i), stride_);
			}
		}

		//png.color_type = PNG_GREYSCALE, PNG_TRUECOLOR, ?PNG_INDEXED, PNG_GREYSCALE_ALPHA, PNG_TRUECOLOR_ALPHA

		switch (png.bpp) {
		case 4: assert(png.color_type == PNG_TRUECOLOR_ALPHA); break;
		case 3: assert(png.color_type == PNG_TRUECOLOR); break;
		case 2: assert(png.color_type == PNG_GREYSCALE_ALPHA); break;
		case 1: assert(png.color_type == PNG_GREYSCALE); break;
		}

		return true;
#		if 0
		png_uint_32 width = 0, height = 0;

		png_structp png_ptr = NULL;
		png_infop info_ptr = NULL;
		FILE *file;

		png_byte pbSig[8];
		int iBitDepth, iColorType;
		double dGamma;
		png_color_16 *pBackground;
		png_byte **ppbRowPointers;

		#ifdef UNICODE
		_wfopen_s(&file, path, L"rb");
		#else
		fopen_s(&file, path, "rb");
		#endif		

		if (!file)
			return false;

		// Identify as PNG file
		fread(pbSig, 1, 8, file);
		if (!png_check_sig(pbSig, 8)){
			fclose(file);
			return false;
		}

		// Create the two png-info structures
		if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, (png_error_ptr) NULL, (png_error_ptr) NULL)) == NULL){
			fclose(file);
			return false;
		}

		if ((info_ptr = png_create_info_struct(png_ptr)) == NULL){
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			fclose(file);
			return false;
		}

		// Initialize the png structure
		png_init_io(png_ptr, file);
		png_set_sig_bytes(png_ptr, 8);

		// Read all PNG info up to image data
		png_read_info(png_ptr, info_ptr);

		// Get width, height, bit-depth and color-type
		png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) &width, (png_uint_32 *) &height, &iBitDepth, &iColorType, NULL, NULL, NULL);

		// Expand all images to 8 bits / channel
		if (iBitDepth == 16) png_set_strip_16(png_ptr);
		if (iColorType == PNG_COLOR_TYPE_PALETTE) png_set_expand(png_ptr);
		if (iBitDepth < 8) png_set_expand(png_ptr);
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_expand(png_ptr);

		// Set the background color to draw transparent and alpha images over.
		if (png_get_bKGD(png_ptr, info_ptr, &pBackground))
			png_set_background(png_ptr, pBackground, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);

		// Set gamma conversion if required
		if (png_get_gAMA(png_ptr, info_ptr, &dGamma))
			png_set_gamma(png_ptr, 2.2, dGamma);

		// After the transformations have been registered update info_ptr data
		png_read_update_info(png_ptr, info_ptr);

		// Get all info again
		png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) &width, (png_uint_32 *) &height, &iBitDepth, &iColorType, NULL, NULL, NULL);

		width_ = width;
		height_ = height;
		bpp_ = iBitDepth;

		i32 nChannels = png_get_channels(png_ptr, info_ptr);

		i32 lw_src = width * nChannels;
		i32 dif_src = lw_src & 0x3;

		if (dif_src)
			dif_src = 4 - dif_src;

		i32 lwidth = lw_src + dif_src;

		switch (nChannels) {
		case 1:
			//format_ = GL_LUMINANCE;
			components_ = 1;
			BPP_ = 1;
			//data_type_ = GL_UNSIGNED_BYTE;
			align_ = 4;

			//gfxColorFormat_ = CF_I8;
			//gfxDataType_ = T_UINT8;
			break;
		case 2:
			//format_ = GL_LUMINANCE_ALPHA;
			components_ = 2;
			BPP_ = 2;
			//data_type_ = GL_UNSIGNED_BYTE;
			align_ = 4;

			//m_gfxColorFormat_ = CF_I8A8;
			//m_gfxDataType_ = T_UINT8;
			break;
		case 3:
			//format_ = GL_RGB;
			components_ = 3;
			BPP_ = 3;
			//data_type_ = GL_UNSIGNED_BYTE;
			align_ = 4;

			//gfxColorFormat_ = CF_RGB8;
			//gfxDataType_ = T_UINT8;
			break;
		case 4:
			//format_ = GL_RGBA;
			components_ = 4;
			BPP_ = 4;
			//data_type_ = GL_UNSIGNED_BYTE;
			align_ = 4;

			//gfxColorFormat_ = CF_RGBA8;
			//gfxDataType_ = T_UINT8;
			break;
		}

		// Allocate memory
		size_bytes_ = lwidth * height;
		data_ = new byte [size_bytes_];
		bpp_ *= nChannels;

		// Pointers for scanlines
		ppbRowPointers = new png_bytep [height];

		for (uint i = 0; i < height; i++)
			ppbRowPointers[i] = data_ + (height-1-i) * lwidth;

		png_read_image(png_ptr, ppbRowPointers);
		png_read_end(png_ptr, NULL);

		delete [] ppbRowPointers;

		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

		fclose(file);
		return true;
#		endif
	}

	bool LoadJpeg(LPCTSTR path) {
#		if 0
		FILE* file;

		#ifdef UNICODE
		_wfopen_s(&file, path, L"rb");
		#else
		fopen_s(&file, path, "rb");
		#endif		

		if (!file)
			return false;

		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		jpeg_stdio_src(&cinfo, file);
		jpeg_read_header(&cinfo, TRUE);

		width_ = cinfo.image_width;
		height_ = cinfo.image_height;

		switch (cinfo.num_components)
		{
		case 3:
			components_ = 3;
			//format_ = GL_RGB;
			//data_type = GL_UNSIGNED_BYTE;
			align_ = 4;
			bpp_ = 24;
			//m_gfxDataType = T_UINT8;
			//m_gfxColorFormat = CF_RGB8;
			break;
		}

		jpeg_start_decompress(&cinfo);

		data_ = new uint8 [cinfo.image_width * cinfo.image_height * cinfo.num_components];

		// Read all pixels
		uint8* dest = data_ + cinfo.image_width * (cinfo.image_height-1) * cinfo.num_components;

		while (cinfo.output_scanline < cinfo.image_height)
		{
			jpeg_read_scanlines(&cinfo, &dest, 1);
			dest -= cinfo.image_width * cinfo.num_components;
		}

		// Clean up
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(file);
		return true;
#		endif
		return false;
	}

private:
	u8* data_;
	i32 width_;
	i32 height_;
	i32 stride_;
	i32 size_;
	i32 size_bytes_;
	i32 bpp_;
	i32 BPP_;
	i32 align_;
	i32 components_;
};

///

LPCTSTR lpClassName = _T("classOgl");
LPCTSTR lpWindowName = _T("Ogl");

float angle = 0.0f, pitch = 0.0f, yaw = 0.0f;
bool pause = true;
int width = 0, height = 0;
bool bDraw_R = true, bDraw_G = true, bDraw_B = true, bDraw_M = true;
bool bDraw_r = true, bDraw_g = true, bDraw_b = true, bDraw_m = true;
bool bPause = false;

/*
(PFNWGLCREATECONTEXTPROC, wglCreateContext)
(PFNWGLDELETECONTEXTPROC, wglDeleteContext)
(PFNWGLGETCURRENTDCPROC, wglGetCurrentDC)
(PFNWGLMAKECURRENTPROC, wglMakeCurrent)
(PFNGLCLEARPROC, glClear)
(PFNGLCLEARCOLORPROC, glClearColor)
(PFNGLCOLOR3FPROC, glColor3f)
(PFNGLLOADIDENTITYPROC, glLoadIdentity)
(PFNGLMATRIXMODEPROC, glMatrixMode)
(PFNGLORTHOPROC, glOrtho)
(PFNGLPOPMATRIXPROC, glPopMatrix)
(PFNGLPUSHMATRIXPROC, glPushMatrix)
(PFNGLROTATEFPROC, glRotatef)
(PFNGLTRANSLATEFPROC, glTranslatef)
(PFNGLVERTEX2FVPROC, glVertex2fv)
(PFNGLVIEWPORTPROC, glViewport)
*/
#define TEXTURE0_SGIS	0x835E
#define TEXTURE1_SGIS	0x835F

typedef HGLRC (APIENTRY * PFNWGLCREATECONTEXTPROC) (HDC);
typedef BOOL (APIENTRY * PFNWGLDELETECONTEXTPROC) (HGLRC);
typedef HDC (APIENTRY * PFNWGLGETCURRENTDCPROC) (VOID);
typedef HGLRC (APIENTRY * PFNWGLGETCURRENTCONTEXTPROC) (VOID);
typedef BOOL (APIENTRY * PFNWGLMAKECURRENTPROC) (HDC, HGLRC);
typedef PROC (APIENTRY * PFNWGLGETPROCADDRESSPROC) (LPCSTR);
typedef void (APIENTRY * PFNGLBEGINPROC) (GLenum mode);
typedef void (APIENTRY * PFNGLCLEARPROC) (GLbitfield mask);
typedef void (APIENTRY * PFNGLCLEARCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRY * PFNGLCOLOR3UBPROC) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (APIENTRY * PFNGLCOLOR3UBVPROC) (GLubyte *v);
typedef void (APIENTRY * PFNGLCOLOR4UBPROC) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
typedef void (APIENTRY * PFNGLCOLOR4UBVPROC) (GLubyte *v);
typedef void (APIENTRY * PFNGLCOLOR3FPROC) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY * PFNGLCOLOR3FVPROC) (GLfloat *v);
typedef void (APIENTRY * PFNGLCOLOR4FPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY * PFNGLCOLOR4FVPROC) (GLfloat *v);
typedef void (APIENTRY * PFNGLENDPROC) (void);
typedef void (APIENTRY * PFNGLFRUSTUMPROC) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (APIENTRY * PFNGLLOADIDENTITYPROC) (void);
typedef void (APIENTRY * PFNGLMATRIXMODEPROC) (GLenum mode);
typedef void (APIENTRY * PFNGLORTHOPROC) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (APIENTRY * PFNGLPOPMATRIXPROC) (void);
typedef void (APIENTRY * PFNGLPUSHMATRIXPROC) (void);
typedef void (APIENTRY * PFNGLROTATEFPROC) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTRANSLATEFPROC) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLSCALEFPROC) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLVERTEX2FPROC) (const GLfloat x, const GLfloat y);
typedef void (APIENTRY * PFNGLVERTEX2FVPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEX3FPROC) (const GLfloat x, const GLfloat y, const GLfloat z);
typedef void (APIENTRY * PFNGLVERTEX3FVPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEX4FPROC) (const GLfloat x, const GLfloat y, const GLfloat z, const GLfloat w);
typedef void (APIENTRY * PFNGLVERTEX4FVPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef const GLubyte * (APIENTRY * PFNGLGETSTRINGPROC) (GLenum name);
typedef void (APIENTRY * PFNGLTEXCOORD2FPROC) (GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLTEXCOORD2FVPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXENVFPROC) (GLenum target, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLTEXENVFVPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLTEXENVIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLTEXENVIVPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLTEXGENDPROC) (GLenum coord, GLenum pname, GLdouble param);
typedef void (APIENTRY * PFNGLTEXGENDVPROC) (GLenum coord, GLenum pname, const GLdouble *params);
typedef void (APIENTRY * PFNGLTEXGENFPROC) (GLenum coord, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLTEXGENFVPROC) (GLenum coord, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLTEXGENIPROC) (GLenum coord, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLTEXGENIVPROC) (GLenum coord, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLTEXIMAGE1DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLTEXPARAMETERFPROC) (GLenum target, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLTEXPARAMETERFVPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLTEXPARAMETERIVPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE1DPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRY * PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRY * PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRY * PFNGLCULLFACEPROC) (GLenum mode);
typedef void (APIENTRY * PFNGLENABLEPROC) (GLenum cap);
typedef void (APIENTRY * PFNGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRY * PFNGLDEPTHFUNCPROC) (GLenum func);
typedef void (APIENTRY * PFNGLALPHAFUNCPROC) (GLenum func, GLclampf ref);
typedef void (APIENTRY * PFNGLARRAYELEMENTPROC) (GLint i);
typedef void (APIENTRY * PFNGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRY * PFNGLCOLORMASKPROC) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (APIENTRY * PFNGLDEPTHMASKPROC) (GLboolean flag);
typedef void (APIENTRY * PFNGLDEPTHRANGEPROC) (GLclampd zNear, GLclampd zFar);
typedef void (APIENTRY * PFNGLDISABLECLIENTSTATEPROC) (GLenum array);
typedef void (APIENTRY * PFNGLDRAWARRAYSPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLDRAWBUFFERPROC) (GLenum mode);
typedef void (APIENTRY * PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (APIENTRY * PFNGLENABLECLIENTSTATEPROC) (GLenum array);
typedef void (APIENTRY * PFNGLFINISHPROC) (void);
typedef void (APIENTRY * PFNGLFLUSHPROC) (void);
typedef void (APIENTRY * PFNGLCOLORPOINTERPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLNORMALPOINTERPROC) (GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLTEXCOORDPOINTERPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLVERTEXPOINTERPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLPIXELSTOREFPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLPIXELTRANSFERFPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLPIXELTRANSFERIPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLPOINTSIZEPROC) (GLfloat size);
typedef void (APIENTRY * PFNGLPOLYGONMODEPROC) (GLenum face, GLenum mode);
typedef void (APIENTRY * PFNGLSTENCILFUNCPROC) (GLenum func, GLint ref, GLuint mask);
typedef void (APIENTRY * PFNGLSTENCILMASKPROC) (GLuint mask);
typedef void (APIENTRY * PFNGLSTENCILOPPROC) (GLenum fail, GLenum zfail, GLenum zpass);
typedef void (APIENTRY * PFNGLHINTPROC) (GLenum target, GLenum mode);
typedef void (APIENTRY * PFNGLFRONTFACEPROC) (GLenum mode);
typedef GLenum (APIENTRY * PFNGLGETERRORPROC) ();

typedef void (APIENTRY * PFNGLMTEXCOORD2FSGISPROC)(GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLSELECTTEXTURESGISPROC)(GLenum target);

#define VAR(type, var) type q##var = 0
#define VAL(type, var) q##var = (type)GetProcAddress(hmodule, #var)
#define WAR(type, var) type q##var = 0
#define WAL(type, var) q##var = (type)qwglGetProcAddress(#var)

VAR(PFNWGLCREATECONTEXTPROC, wglCreateContext);
VAR(PFNWGLDELETECONTEXTPROC, wglDeleteContext);
VAR(PFNWGLGETCURRENTDCPROC, wglGetCurrentDC);
VAR(PFNWGLGETCURRENTCONTEXTPROC, wglGetCurrentContext);
VAR(PFNWGLMAKECURRENTPROC, wglMakeCurrent);
VAR(PFNWGLGETPROCADDRESSPROC, wglGetProcAddress);
VAR(PFNGLBEGINPROC, glBegin);
VAR(PFNGLCLEARPROC, glClear);
VAR(PFNGLCLEARCOLORPROC, glClearColor);
VAR(PFNGLCOLOR3UBPROC, glColor3ub);
VAR(PFNGLCOLOR3UBVPROC, glColor3ubv);
VAR(PFNGLCOLOR4UBPROC, glColor4ub);
VAR(PFNGLCOLOR4UBVPROC, glColor4ubv);
VAR(PFNGLCOLOR3FPROC, glColor3f);
VAR(PFNGLCOLOR3FVPROC, glColor3fv);
VAR(PFNGLCOLOR4FPROC, glColor4f);
VAR(PFNGLCOLOR4FVPROC, glColor4fv);
VAR(PFNGLENDPROC, glEnd);
VAR(PFNGLFRUSTUMPROC, glFrustum);
VAR(PFNGLLOADIDENTITYPROC, glLoadIdentity);
VAR(PFNGLMATRIXMODEPROC, glMatrixMode);
VAR(PFNGLORTHOPROC, glOrtho);
VAR(PFNGLPOPMATRIXPROC, glPopMatrix);
VAR(PFNGLPUSHMATRIXPROC, glPushMatrix);
VAR(PFNGLROTATEFPROC, glRotatef);
VAR(PFNGLTRANSLATEFPROC, glTranslatef);
VAR(PFNGLSCALEFPROC, glScalef);
VAR(PFNGLVERTEX2FPROC, glVertex2f);
VAR(PFNGLVERTEX2FVPROC, glVertex2fv);
VAR(PFNGLVERTEX3FPROC, glVertex3f);
VAR(PFNGLVERTEX3FVPROC, glVertex3fv);
VAR(PFNGLVERTEX4FPROC, glVertex4f);
VAR(PFNGLVERTEX4FVPROC, glVertex4fv);
VAR(PFNGLVIEWPORTPROC, glViewport);
VAR(PFNGLGETSTRINGPROC, glGetString);
VAR(PFNGLTEXCOORD2FPROC, glTexCoord2f);
VAR(PFNGLTEXCOORD2FVPROC, glTexCoord2fv);
VAR(PFNGLTEXENVFPROC, glTexEnvf);
VAR(PFNGLTEXENVFVPROC, glTexEnvfv);
VAR(PFNGLTEXENVIPROC, glTexEnvi);
VAR(PFNGLTEXENVIVPROC, glTexEnviv);
VAR(PFNGLTEXGENDPROC, glTexGend);
VAR(PFNGLTEXGENDVPROC, glTexGendv);
VAR(PFNGLTEXGENFPROC, glTexGenf);
VAR(PFNGLTEXGENFVPROC, glTexGenfv);
VAR(PFNGLTEXGENIPROC, glTexGeni);
VAR(PFNGLTEXGENIVPROC, glTexGeniv);
VAR(PFNGLTEXIMAGE1DPROC, glTexImage1D);
VAR(PFNGLTEXIMAGE2DPROC, glTexImage2D);
VAR(PFNGLTEXPARAMETERFPROC, glTexParameterf);
VAR(PFNGLTEXPARAMETERFVPROC, glTexParameterfv);
VAR(PFNGLTEXPARAMETERIPROC, glTexParameteri);
VAR(PFNGLTEXPARAMETERIVPROC, glTexParameteriv);
VAR(PFNGLTEXSUBIMAGE1DPROC, glTexSubImage1D);
VAR(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
VAR(PFNGLGENTEXTURESPROC, glGenTextures);
VAR(PFNGLDELETETEXTURESPROC, glDeleteTextures);
VAR(PFNGLBINDTEXTUREPROC, glBindTexture);
VAR(PFNGLCULLFACEPROC, glCullFace);
VAR(PFNGLENABLEPROC, glEnable);
VAR(PFNGLDISABLEPROC, glDisable);
VAR(PFNGLDEPTHFUNCPROC, glDepthFunc);
VAR(PFNGLALPHAFUNCPROC, glAlphaFunc);
VAR(PFNGLARRAYELEMENTPROC, glArrayElement);
VAR(PFNGLBLENDFUNCPROC, glBlendFunc);
VAR(PFNGLCOLORMASKPROC, glColorMask);
VAR(PFNGLDEPTHMASKPROC, glDepthMask);
VAR(PFNGLDEPTHRANGEPROC, glDepthRange);
VAR(PFNGLDISABLECLIENTSTATEPROC, glDisableClientState);
VAR(PFNGLDRAWARRAYSPROC, glDrawArrays);
VAR(PFNGLDRAWBUFFERPROC, glDrawBuffer);
VAR(PFNGLDRAWELEMENTSPROC, glDrawElements);
VAR(PFNGLENABLECLIENTSTATEPROC, glEnableClientState);
VAR(PFNGLFINISHPROC, glFinish);
VAR(PFNGLFLUSHPROC, glFlush);
VAR(PFNGLCOLORPOINTERPROC, glColorPointer);
VAR(PFNGLNORMALPOINTERPROC, glNormalPointer);
VAR(PFNGLTEXCOORDPOINTERPROC, glTexCoordPointer);
VAR(PFNGLVERTEXPOINTERPROC, glVertexPointer);
VAR(PFNGLPIXELSTOREFPROC, glPixelStoref);
VAR(PFNGLPIXELSTOREIPROC, glPixelStorei);
VAR(PFNGLPIXELTRANSFERFPROC, glPixelTransferf);
VAR(PFNGLPIXELTRANSFERIPROC, glPixelTransferi);
VAR(PFNGLPOINTSIZEPROC, glPointSize);
VAR(PFNGLPOLYGONMODEPROC, glPolygonMode);
VAR(PFNGLSTENCILFUNCPROC, glStencilFunc);
VAR(PFNGLSTENCILMASKPROC, glStencilMask);
VAR(PFNGLSTENCILOPPROC, glStencilOp);
VAR(PFNGLHINTPROC, glHint);
VAR(PFNGLFRONTFACEPROC, glFrontFace);
VAR(PFNGLGETERRORPROC, glGetError);
WAR(PFNGLMTEXCOORD2FSGISPROC, glMTexCoord2fSGIS);
WAR(PFNGLSELECTTEXTURESGISPROC, glSelectTextureSGIS);


void initLib() {
	char buf[1024];
	GetCurrentDirectoryA(1024, buf);
	//strcat(buf, "\\..\\_build\\opengl32\\Debug\\opengl32.dll");
	//HMODULE hmodule = LoadLibraryA(buf);
	HMODULE hmodule = LoadLibraryA("opengl32.dll");

	VAL(PFNWGLCREATECONTEXTPROC, wglCreateContext);
	VAL(PFNWGLDELETECONTEXTPROC, wglDeleteContext);
	VAL(PFNWGLGETCURRENTDCPROC, wglGetCurrentDC);
	VAL(PFNWGLGETCURRENTCONTEXTPROC, wglGetCurrentContext);
	VAL(PFNWGLMAKECURRENTPROC, wglMakeCurrent);
	VAL(PFNWGLGETPROCADDRESSPROC, wglGetProcAddress);
	VAL(PFNGLBEGINPROC, glBegin);
	VAL(PFNGLCLEARPROC, glClear);
	VAL(PFNGLCLEARCOLORPROC, glClearColor);
	VAL(PFNGLCOLOR3UBPROC, glColor3ub);
	VAL(PFNGLCOLOR3UBVPROC, glColor3ubv);
	VAL(PFNGLCOLOR4UBPROC, glColor4ub);
	VAL(PFNGLCOLOR4UBVPROC, glColor4ubv);
	VAL(PFNGLCOLOR3FPROC, glColor3f);
	VAL(PFNGLCOLOR3FVPROC, glColor3fv);
	VAL(PFNGLCOLOR4FPROC, glColor4f);
	VAL(PFNGLCOLOR4FVPROC, glColor4fv);
	VAL(PFNGLENDPROC, glEnd);
	VAL(PFNGLFRUSTUMPROC, glFrustum);
	VAL(PFNGLLOADIDENTITYPROC, glLoadIdentity);
	VAL(PFNGLMATRIXMODEPROC, glMatrixMode);
	VAL(PFNGLORTHOPROC, glOrtho);
	VAL(PFNGLPOPMATRIXPROC, glPopMatrix);
	VAL(PFNGLPUSHMATRIXPROC, glPushMatrix);
	VAL(PFNGLROTATEFPROC, glRotatef);
	VAL(PFNGLTRANSLATEFPROC, glTranslatef);
	VAL(PFNGLSCALEFPROC, glScalef);
	VAL(PFNGLVERTEX2FPROC, glVertex2f);
	VAL(PFNGLVERTEX2FVPROC, glVertex2fv);
	VAL(PFNGLVERTEX3FPROC, glVertex3f);
	VAL(PFNGLVERTEX3FVPROC, glVertex3fv);
	VAL(PFNGLVERTEX4FPROC, glVertex4f);
	VAL(PFNGLVERTEX4FVPROC, glVertex4fv);
	VAL(PFNGLVIEWPORTPROC, glViewport);
	VAL(PFNGLGETSTRINGPROC, glGetString);
	VAL(PFNGLTEXCOORD2FPROC, glTexCoord2f);
	VAL(PFNGLTEXCOORD2FVPROC, glTexCoord2fv);
	VAL(PFNGLTEXENVFPROC, glTexEnvf);
	VAL(PFNGLTEXENVFVPROC, glTexEnvfv);
	VAL(PFNGLTEXENVIPROC, glTexEnvi);
	VAL(PFNGLTEXENVIVPROC, glTexEnviv);
	VAL(PFNGLTEXGENDPROC, glTexGend);
	VAL(PFNGLTEXGENDVPROC, glTexGendv);
	VAL(PFNGLTEXGENFPROC, glTexGenf);
	VAL(PFNGLTEXGENFVPROC, glTexGenfv);
	VAL(PFNGLTEXGENIPROC, glTexGeni);
	VAL(PFNGLTEXGENIVPROC, glTexGeniv);
	VAL(PFNGLTEXIMAGE1DPROC, glTexImage1D);
	VAL(PFNGLTEXIMAGE2DPROC, glTexImage2D);
	VAL(PFNGLTEXPARAMETERFPROC, glTexParameterf);
	VAL(PFNGLTEXPARAMETERFVPROC, glTexParameterfv);
	VAL(PFNGLTEXPARAMETERIPROC, glTexParameteri);
	VAL(PFNGLTEXPARAMETERIVPROC, glTexParameteriv);
	VAL(PFNGLTEXSUBIMAGE1DPROC, glTexSubImage1D);
	VAL(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
	VAL(PFNGLGENTEXTURESPROC, glGenTextures);
	VAL(PFNGLDELETETEXTURESPROC, glDeleteTextures);
	VAL(PFNGLBINDTEXTUREPROC, glBindTexture);
	VAL(PFNGLCULLFACEPROC, glCullFace);
	VAL(PFNGLENABLEPROC, glEnable);
	VAL(PFNGLDISABLEPROC, glDisable);
	VAL(PFNGLDEPTHFUNCPROC, glDepthFunc);
	VAL(PFNGLALPHAFUNCPROC, glAlphaFunc);
	VAL(PFNGLARRAYELEMENTPROC, glArrayElement);
	VAL(PFNGLBLENDFUNCPROC, glBlendFunc);
	VAL(PFNGLCOLORMASKPROC, glColorMask);
	VAL(PFNGLDEPTHMASKPROC, glDepthMask);
	VAL(PFNGLDEPTHRANGEPROC, glDepthRange);
	VAL(PFNGLDISABLECLIENTSTATEPROC, glDisableClientState);
	VAL(PFNGLDRAWARRAYSPROC, glDrawArrays);
	VAL(PFNGLDRAWBUFFERPROC, glDrawBuffer);
	VAL(PFNGLDRAWELEMENTSPROC, glDrawElements);
	VAL(PFNGLENABLECLIENTSTATEPROC, glEnableClientState);
	VAL(PFNGLFINISHPROC, glFinish);
	VAL(PFNGLFLUSHPROC, glFlush);
	VAL(PFNGLCOLORPOINTERPROC, glColorPointer);
	VAL(PFNGLNORMALPOINTERPROC, glNormalPointer);
	VAL(PFNGLTEXCOORDPOINTERPROC, glTexCoordPointer);
	VAL(PFNGLVERTEXPOINTERPROC, glVertexPointer);
	VAL(PFNGLPIXELSTOREFPROC, glPixelStoref);
	VAL(PFNGLPIXELSTOREIPROC, glPixelStorei);
	VAL(PFNGLPIXELTRANSFERFPROC, glPixelTransferf);
	VAL(PFNGLPIXELTRANSFERIPROC, glPixelTransferi);
	VAL(PFNGLPOINTSIZEPROC, glPointSize);
	VAL(PFNGLPOLYGONMODEPROC, glPolygonMode);
	VAL(PFNGLSTENCILFUNCPROC, glStencilFunc);
	VAL(PFNGLSTENCILMASKPROC, glStencilMask);
	VAL(PFNGLSTENCILOPPROC, glStencilOp);
	VAL(PFNGLHINTPROC, glHint);
	VAL(PFNGLFRONTFACEPROC, glFrontFace);
	VAL(PFNGLGETERRORPROC, glGetError);
	WAL(PFNGLMTEXCOORD2FSGISPROC, glMTexCoord2fSGIS);
	WAL(PFNGLSELECTTEXTURESGISPROC, glSelectTextureSGIS);

	const char* str = (const char*)qglGetString(GL_VERSION);
	//assert(glMTexCoord2fSGIS != nullptr);
	/*
	const char* t = (const char*)qglGetString(GL_VENDOR);
	if (t) {
		strcat(s, "vendor: ");
		strcat(s, t);
		strcat(s, "\n");
	}
	t = (const char*)qglGetString(GL_RENDERER);
	if (t) {
		strcat(s, "renderer: ");
		strcat(s, t);
		strcat(s, "\n");
	}
	t = (const char*)qglGetString(GL_VERSION);
	if (t) {
		strcat(s, "version: ");
		strcat(s, t);
		strcat(s, "\n");
	}
	t = (const char*)qglGetString(GL_EXTENSIONS);
	if (t) {
		strcat(s, "extensions: ");
		strcat(s, t);
	}
	MessageBoxA(0, s, s, MB_OK);
	//*/
}

void qglPerspective(float fovy, float aspect, float znear, float zfar) {
	const float top = znear*tanf((0.5f*fovy*float(M_PI)/180.f)), right = top*aspect;
	qglFrustum(-right, right, -top, top, znear, zfar);
}

void size(int cx, int cy) {
	if (!qglViewport)
		return;

	width = cx;
	height = cy;
	qglViewport(0, 0, cx, cy);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	//qglOrtho(0, cx, cy, 0, -1, 1);
	qglPerspective(90.f, cx/float(cy), .1f, 100.f);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
}

GLuint tex = 0;
bool active = false;

void init() {
	qglClearColor(0, 0, 1.0f, 1.0f);

	Image img;
	//img.Open(_T("tex/cat.png"));
	//img.Open(_T("tex/tex1.png"));
	img.Open(_T("tex/tex2.png"));

	//qglPixelStorei(GL_UNPACK_ROW_LENGTH, data.Width);
	//qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	qglGenTextures(1, &tex);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//qglEnable(GL_TEXTURE_GEN_S);
	//qglEnable(GL_TEXTURE_GEN_T);
	//qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	//qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	//qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	//qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	//qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	//qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	switch (img.BPP()) {
	case 3: qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.Width(), img.Height(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.Data()); break;
	case 4: qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data()); break;
	}

	img.Close();
	img.Open(_T("tex/0010.png"));
	qglBindTexture(GL_TEXTURE_2D, 10);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/0760.png"));
	qglBindTexture(GL_TEXTURE_2D, 76);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/0770.png"));
	qglBindTexture(GL_TEXTURE_2D, 77);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/0780.png"));
	qglBindTexture(GL_TEXTURE_2D, 78);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/0960.png"));
	qglBindTexture(GL_TEXTURE_2D, 96);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/1620.png"));
	qglBindTexture(GL_TEXTURE_2D, 162);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, 1, img.Width(), img.Height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img.Data());

	img.Close();
	img.Open(_T("tex/1155.png"), false);
	qglBindTexture(GL_TEXTURE_2D, 1155);
	qglTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Data());
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_LINEAR_MIPMAP_NEAREST
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//qglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	qglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void draw() {
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	qglPushMatrix();
	qglTranslatef(0.f, 0.f, -1.f);
	static ts t;

	qglEnable(GL_DEPTH_TEST);

#if 0
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglBegin(GL_TRIANGLES);
		qglColor3f(1.f, 0.f, 0.f); qglTexCoord2f(0.f, 1.f); qglVertex3f(-1.f, +1.f, +.0f);
		qglColor3f(0.f, 1.f, 0.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-1.f, -1.f, -.2f);
		qglColor3f(0.f, 1.f, 1.f); qglTexCoord2f(1.f, 1.f); qglVertex3f(+1.f, +1.f, +.2f);
#		if 1
		qglColor3f(0.f, 1.f, 1.f); qglTexCoord2f(1.f, 1.f); qglVertex3f(+1.f, +1.f, +.2f);
		qglColor3f(0.f, 1.f, 0.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-1.f, -1.f, -.2f);
		qglColor3f(1.f, 1.f, 0.f); qglTexCoord2f(1.f, 0.f); qglVertex3f(+1.f, -1.f, +.0f);
#		endif
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
#endif
#if 0
	static float angle = 0.f;
	if (!bPause)
		angle = float(t.current_sec());
	qglRotatef(angle * 10.f, 0.f, 0.f, 1.f);

	//qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, .5f);
	//qglEnable(GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglSelectTextureSGIS(TEXTURE0_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglSelectTextureSGIS(TEXTURE1_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglBegin(GL_TRIANGLES);
		qglColor4f(1.f, 0.f, 0.f, .9f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.f, 1.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.f, 1.f); qglVertex3f(-1.f, 1.f, 0.f);
		qglColor4f(0.f, 1.f, 0.f, .6f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.f, 0.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.f, 0.f); qglVertex3f(-1.f,-1.f,-1.f);
		qglColor4f(0.f, 1.f, 1.f, .3f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 1.f, 1.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 1.f, 1.f); qglVertex3f( 1.f, 1.f, 1.f);
#		if 1
		qglColor4f(0.f, 1.f, 1.f, .8f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 2.f, 2.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 2.f, 2.f); qglVertex3f( 1.f, 1.f, 1.f);
		qglColor4f(0.f, 1.f, 0.f, .5f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.f, 0.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.f, 0.f); qglVertex3f(-1.f,-1.f,-1.f);
		qglColor4f(1.f, 1.f, 0.f, .2f); qglMTexCoord2fSGIS(TEXTURE0_SGIS, 2.f, 0.f); qglMTexCoord2fSGIS(TEXTURE1_SGIS, 2.f, 0.f); qglVertex3f( 1.f,-1.f, 0.f);
#		endif
	qglEnd();
	qglSelectTextureSGIS(TEXTURE1_SGIS);
	qglDisable(GL_TEXTURE_2D);
	qglSelectTextureSGIS(TEXTURE0_SGIS);
	qglDisable(GL_TEXTURE_2D);

	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_BLEND);
#endif
	qglPopMatrix();

#	if 0
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
	qglMatrixMode(GL_MODELVIEW);

	qglDisable(GL_DEPTH_TEST);
	qglDepthMask(GL_FALSE);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglEnable(GL_TEXTURE_2D);
	qglBegin(GL_QUADS);
		qglColor4f(1.f, 1.f, 0.f, .9f); qglTexCoord2f(0.f, 1.f); qglVertex2f(-1.f,-.8f);
		qglColor4f(1.f, 1.f, 0.f, .9f); qglTexCoord2f(0.f, 0.f); qglVertex2f(-1.f,-1.f);
		qglColor4f(1.f, 1.f, 0.f, .9f); qglTexCoord2f(1.f, 0.f); qglVertex2f( 1.f,-1.f);
		qglColor4f(1.f, 1.f, 0.f, .9f); qglTexCoord2f(1.f, 1.f); qglVertex2f( 1.f,-.8f);
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
	qglDepthMask(GL_TRUE);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglEnable(GL_DEPTH_TEST);
#	endif

#	if 1
	qglPushMatrix();
	qglTranslatef(0, 0, -25.f);
	//qglRotatef(90.f, 0.f, 0.f, 1.f);
	qglColor4f(1.f, 1.f, 1.f, 1.f);

	//*
#if 0 // BUTTON
	qglEnable(GL_TEXTURE_2D);
	qglSelectTextureSGIS(TEXTURE0_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 76);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglSelectTextureSGIS(TEXTURE1_SGIS);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, 162);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

	qglBegin(GL_POLYGON);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, -25.000f, -3.000f);
		qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.707f, 0.246f);
		qglVertex3f(-24.0f, 24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, -25.000, -2.000f);
		qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.707f, 0.270f);
		qglVertex3f(-24.0f,-24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, -26.000f, -2.000f);
		qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.684f, 0.270f);
		qglVertex3f( 24.0f,-24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, -26.000f, -3.000f);
		qglMTexCoord2fSGIS(TEXTURE1_SGIS, 0.684f, 0.246f);
		qglVertex3f( 24.0f, 24.0f, 0.f);
	qglEnd();
#elif 1 // LIGHTMAP
	qglEnable(GL_TEXTURE_2D);
	qglSelectTextureSGIS(TEXTURE0_SGIS);
	qglBindTexture(GL_TEXTURE_2D, 162);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	qglBegin(GL_POLYGON);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.707f, 0.246f);
		qglVertex3f(24.0f, 24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.707f, 0.270f);
		qglVertex3f(-24.0f, 24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.684f, 0.270f);
		qglVertex3f(-24.0f, -24.0f, 0.f);
		qglMTexCoord2fSGIS(TEXTURE0_SGIS, 0.684f, 0.246f);
		qglVertex3f(24.0f, -24.0f, 0.f);
	qglEnd();
#else // W letter
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, 10);
	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.6f);
	qglBegin(GL_QUADS);
	qglTexCoord2f(0.4375f, 0.3125f); qglVertex2f( 24.f,  24.f);
	qglTexCoord2f(0.5000f, 0.3125f); qglVertex2f(-24.f,  24.f);
	qglTexCoord2f(0.5000f, 0.3750f); qglVertex2f(-24.f, -24.f);
	qglTexCoord2f(0.4375f, 0.3750f); qglVertex2f( 24.f, -24.f);
	qglEnd();
#endif
//*/

#	if 0

	qglFrontFace(GL_CW);
	qglEnable(GL_CULL_FACE);
	qglDepthFunc(GL_LEQUAL);
	qglEnable(GL_TEXTURE_2D);
	qglBindTexture(GL_TEXTURE_2D, tex);
	//qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	qglBegin(GL_TRIANGLE_FAN);
	qglTexCoord2f(0.462662f, 0.553691f); qglColor3f(0.310f, 0.310f, 0.310f); qglVertex3f( 24.f, 24.f, 0.f);
	qglTexCoord2f(0.381494f, 0.553691f); qglColor3f(0.512f, 0.512f, 0.512f); qglVertex3f( 24.f,-24.f, 0.f);
	qglTexCoord2f(0.381494f, 0.406040f); qglColor3f(0.550f, 0.550f, 0.550f); qglVertex3f(-24.f,-24.f, 0.f);
	qglTexCoord2f(0.462662f, 0.406040f); qglColor3f(0.365f, 0.365f, 0.365f); qglVertex3f(-24.f, 24.f, 0.f);
	qglEnd();
	//*
	qglBegin(GL_TRIANGLE_FAN);
	qglTexCoord2f(0.881494f, 0.406040f); qglColor3f(0.550f, 0.550f, 0.550f); qglVertex3f(-24.f,-24.f, 0.f);
	qglTexCoord2f(0.881494f, 0.553691f); qglColor3f(0.512f, 0.512f, 0.512f); qglVertex3f( 24.f,-24.f, 0.f);
	qglTexCoord2f(0.962662f, 0.553691f); qglColor3f(0.310f, 0.310f, 0.310f); qglVertex3f( 24.f, 24.f, 0.f);
	qglEnd();
	//*/
#	endif

	qglPopMatrix();
	qglDisable(GL_TEXTURE_2D);
	qglSelectTextureSGIS(TEXTURE0_SGIS);
	qglDisable(GL_TEXTURE_2D);
#	endif
	
	SwapBuffers(qwglGetCurrentDC());
}

void print() {
	qglClearColor(0.f, 0.f, 0.f, 1.f);
	qglClear(GL_COLOR_BUFFER_BIT);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0.f, 800.f, 600.f, 0.f, -99999.f, 99999.f);
	//qglOrtho(0.f, 800.f, 600.f, 0.f, -1.f, 1.f);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglPushMatrix();

	/* // testing bad console font issue due to viewport-1
	qglBindTexture(GL_TEXTURE_2D, 1155);

	qglColor4f(1.f, 1.f, 1.f, 1.f);
	qglAlphaFunc(GL_GREATER, .666f);
	qglEnable(GL_ALPHA_TEST);
	qglEnable(GL_TEXTURE_2D);

	std::string_view s{ "v3.20" };

	//f32 dx = 8.f, dy = 8.f, dh = 9.f;
	f32 dx = 8.f, dy = dx, dh = dy+1.f;
	auto write = [dx, dy](f32 x, f32 y, std::string_view s) {
		int dofs = 246 - 'v';
		//f32 dx = 8.f, dy = 8.f;
		for (std::string_view::iterator i = s.begin(); i != s.end(); ++i, x += dx) {
			int ofs = *i + dofs;
			int ox = ofs & 15, oy = ofs / 16;

			qglBegin(GL_QUADS);
				qglTexCoord2f( ox        / 16.f,  oy        / 16.f); qglVertex2f(x     , y     );
				qglTexCoord2f((ox + 1.f) / 16.f,  oy        / 16.f); qglVertex2f(x + dx, y     );
				qglTexCoord2f((ox + 1.f) / 16.f, (oy + 1.f) / 16.f); qglVertex2f(x + dx, y + dy);
				qglTexCoord2f( ox        / 16.f, (oy + 1.f) / 16.f); qglVertex2f(x     , y + dy);
			qglEnd();
		}
	};

	//write(796.f - dx * 5, 596.f - dy - dh * 22, "3");
	//write(796.f - dx * 5, 596.f - dy - dh * 20, "3");

	for (f32 y = 596.f - dy - dh*18; y > 0; y -= dh)
		//write(796.f - dx * 5, y, "v3.20");
		write(756.f, y, "v3.20");
		//write(796.f - dx * 5, y, "3");
	/*
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.375000, 0.937500); qglVertex2f(756.000, 288.000);
		qglTexCoord2f(0.437500, 0.937500); qglVertex2f(764.000, 288.000);
		qglTexCoord2f(0.437500, 1.000000); qglVertex2f(764.000, 296.000);
		qglTexCoord2f(0.375000, 1.000000); qglVertex2f(756.000, 296.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.187500, 0.687500); qglVertex2f(764.000, 288.000);
		qglTexCoord2f(0.250000, 0.687500); qglVertex2f(772.000, 288.000);
		qglTexCoord2f(0.250000, 0.750000); qglVertex2f(772.000, 296.000);
		qglTexCoord2f(0.187500, 0.750000); qglVertex2f(764.000, 296.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.875000, 0.625000); qglVertex2f(772.000, 288.000);
		qglTexCoord2f(0.937500, 0.625000); qglVertex2f(780.000, 288.000);
		qglTexCoord2f(0.937500, 0.687500); qglVertex2f(780.000, 296.000);
		qglTexCoord2f(0.875000, 0.687500); qglVertex2f(772.000, 296.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.125000, 0.687500); qglVertex2f(780.000, 288.000);
		qglTexCoord2f(0.187500, 0.687500); qglVertex2f(788.000, 288.000);
		qglTexCoord2f(0.187500, 0.750000); qglVertex2f(788.000, 296.000);
		qglTexCoord2f(0.125000, 0.750000); qglVertex2f(780.000, 296.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.000000, 0.687500); qglVertex2f(788.000, 288.000);
		qglTexCoord2f(0.062500, 0.687500); qglVertex2f(796.000, 288.000);
		qglTexCoord2f(0.062500, 0.750000); qglVertex2f(796.000, 296.000);
		qglTexCoord2f(0.000000, 0.750000); qglVertex2f(788.000, 296.000);
	qglEnd();

	//

	qglBegin(GL_QUADS);
		qglTexCoord2f(0.375000, 0.937500); qglVertex2f(756.000, 588.000);
		qglTexCoord2f(0.437500, 0.937500); qglVertex2f(764.000, 588.000);
		qglTexCoord2f(0.437500, 1.000000); qglVertex2f(764.000, 596.000);
		qglTexCoord2f(0.375000, 1.000000); qglVertex2f(756.000, 596.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.187500, 0.687500); qglVertex2f(764.000, 588.000);
		qglTexCoord2f(0.250000, 0.687500); qglVertex2f(772.000, 588.000);
		qglTexCoord2f(0.250000, 0.750000); qglVertex2f(772.000, 596.000);
		qglTexCoord2f(0.187500, 0.750000); qglVertex2f(764.000, 596.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.875000, 0.625000); qglVertex2f(772.000, 588.000);
		qglTexCoord2f(0.937500, 0.625000); qglVertex2f(780.000, 588.000);
		qglTexCoord2f(0.937500, 0.687500); qglVertex2f(780.000, 596.000);
		qglTexCoord2f(0.875000, 0.687500); qglVertex2f(772.000, 596.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.125000, 0.687500); qglVertex2f(780.000, 588.000);
		qglTexCoord2f(0.187500, 0.687500); qglVertex2f(788.000, 588.000);
		qglTexCoord2f(0.187500, 0.750000); qglVertex2f(788.000, 596.000);
		qglTexCoord2f(0.125000, 0.750000); qglVertex2f(780.000, 596.000);
	qglEnd();
	qglBegin(GL_QUADS);
		qglTexCoord2f(0.000000, 0.687500); qglVertex2f(788.000, 588.000);
		qglTexCoord2f(0.062500, 0.687500); qglVertex2f(796.000, 588.000);
		qglTexCoord2f(0.062500, 0.750000); qglVertex2f(796.000, 596.000);
		qglTexCoord2f(0.000000, 0.750000); qglVertex2f(788.000, 596.000);
	qglEnd();//*/

	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_ALPHA_TEST);
	//*/

	qglPopMatrix();
	SwapBuffers(qwglGetCurrentDC());
}

void idle() {
	//print();
	//draw();
	//return;

	//qglClearColor(.0f, .1f, .1f, 1);
	qglClearColor(1.f, 1.f, 1.f, 1.f);
	//qglClear(GL_COLOR_BUFFER_BIT);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
	qglBegin(GL_TRIANGLES);
		qglColor3f(1, 0, 0);
		qglVertex2f( 0,  0);
		qglVertex2f(10, 50);
		qglVertex2f(50, 10);
	qglEnd();//*/
	/*
	static bool zflag = true;
	if (zflag) {
		qglDepthRange(0.f, .49999f);
		qglDepthFunc(GL_LEQUAL);
	} else {
		qglDepthRange(1.f, 0.5f);
		qglDepthFunc(GL_GEQUAL);
	}
	zflag = !zflag;
	//*/
	qglPushMatrix();

#	if 0 // ortho
	float s = 120, ox = 30, oy = 30;
	qglTranslatef(ox+width/2, oy+height/2, 0);
#	else // frustum
	float s = 1, ox = 30, oy = 30;
	qglTranslatef(0, 0, -2);
	//qglScalef(.5f, .5f, .5f);
#	endif
	
	qglRotatef(pitch, 1, 0, 0);
	qglRotatef(  yaw, 0, 1, 0);

	qglRotatef(angle*2, 0, 0, 1);

	float x1 = s*-1+ox+width/2, x2 = s*0+ox+width/2, x3 = s*1+ox+width/2;
	float y1 = s*-1+oy+height/2, y2 = s*0+oy+height/2, y3 = s*1+oy+height/2;

	typedef float vec2f[2];

	static vec2f v[3][3] = {
		{{-s, -s},{0, -s},{s, -s}},
		{{-s,  0},{0,  0},{s,  0}},
		{{-s,  s},{0,  s},{s,  s}}
	};

	static vec2f t[3][3] = {
		{{0.0f, 0.0f},{0.5f, 0.0f},{1.0f, 0.0f}},
		{{0.0f, 0.5f},{0.5f, 0.5f},{1.0f, 0.5f}},
		{{0.0f, 1.0f},{0.5f, 1.0f},{1.0f, 1.0f}}
	};

	/*
	qglBegin(GL_TRIANGLES);
		qglColor3f(1, 0, 0);
		qglVertex2f( 0, 0);
		qglVertex2f( 0, 1);
		qglVertex2f( 1, 0);
	qglEnd();//*/

	const float x = 2.f;

	/*
	qglEnable(GL_TEXTURE_2D);
	qglColor3f(1.f, 1.f, 1.f);
	qglBegin(GL_QUADS);
	qglEnable(GL_CULL_FACE);
	qglFrontFace(GL_CW);
	//f
	qglTexCoord2f( 0.f, 1.f); qglVertex3f(-x, x,-x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f(-x,-x,-x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f( x,-x,-x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f( x, x,-x);
	//b
	qglTexCoord2f( 0.f, 1.f); qglVertex3f( x, x, x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f( x,-x, x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f(-x,-x, x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f(-x, x, x);
	//l
	qglTexCoord2f( 0.f, 1.f); qglVertex3f(-x, x, x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f(-x,-x, x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f(-x,-x,-x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f(-x, x,-x);
	//r
	qglTexCoord2f( 0.f, 1.f); qglVertex3f( x, x,-x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f( x,-x,-x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f( x,-x, x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f( x, x, x);
	//t
	qglTexCoord2f( 0.f, 1.f); qglVertex3f(-x, x, x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f(-x, x,-x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f( x, x,-x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f( x, x, x);
	//b
	qglTexCoord2f( 0.f, 1.f); qglVertex3f(-x,-x,-x);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f(-x,-x, x);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f( x,-x, x);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f( x,-x,-x);
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_CULL_FACE);
	//*/
	/*
	qglEnable(GL_TEXTURE_2D);
	qglBegin(GL_QUADS);
	qglColor3f(1, 0, 0);
#	if 0
	qglTexCoord2f( 0.f, 1.f); qglVertex3f(-s, s,-2.0f);
	qglTexCoord2f( 0.f, 0.f); qglVertex3f(-s,-s,-1.0f);
	qglTexCoord2f( 1.f, 0.f); qglVertex3f( s,-s,-2.0f);
	qglTexCoord2f( 1.f, 1.f); qglVertex3f( s, s,-3.0f);
#	else
	qglTexCoord2f(-5.f,-1.f); qglVertex3f(-s, s,-2.0f);
	qglTexCoord2f(-5.f,-.125f); qglVertex3f(-s,-s,-1.0f);
	qglTexCoord2f(-6.f,-.125f); qglVertex3f( s,-s,-2.0f);
	qglTexCoord2f(-6.f,-1.f); qglVertex3f( s, s,-3.0f);
	//qglTexCoord2f(-2.5f,-4.5f); qglVertex3f(-s, s,-2.0f);
	//qglTexCoord2f(-2.5f,-2.5f); qglVertex3f(-s,-s,-1.0f);
	//qglTexCoord2f(-4.5f,-2.5f); qglVertex3f( s,-s,-2.0f);
	//qglTexCoord2f(-4.5f,-4.5f); qglVertex3f( s, s,-3.0f);
#	endif
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
	//*/

	//MB
	//RG
	/*
	qglPushMatrix();
	qglBegin(GL_TRIANGLES);
	qglColor3ub(160,   0,   0);	qglVertex2fv(v[0][1]);
	qglColor3ub(  0, 160,   0);	qglVertex2fv(v[0][2]);
	qglColor3ub(  0,   0, 160);	qglVertex2fv(v[1][2]);
	qglEnd();
	qglPopMatrix();
	//*/

	/*
	qglPushMatrix();
	qglRotatef(angle, 0, 0, 1);
	qglRotatef(angle, 0, 1, 0);
	qglRotatef(angle, 1, 0, 0);

	qglEnable(GL_TEXTURE_2D);
	qglBegin(GL_TRIANGLES);
	qglColor3ub(160,   0,   0);	qglTexCoord2fv(t[1][0]); qglVertex2fv(v[1][0]); qglTexCoord2fv(t[0][0]); qglVertex2fv(v[0][0]); qglTexCoord2fv(t[0][1]); qglVertex2fv(v[0][1]);
	qglColor3ub(  0, 160,   0);	qglTexCoord2fv(t[0][1]); qglVertex2fv(v[0][1]); qglTexCoord2fv(t[0][2]); qglVertex2fv(v[0][2]); qglTexCoord2fv(t[1][2]); qglVertex2fv(v[1][2]);
	qglColor3ub(  0,   0, 160);	qglTexCoord2fv(t[1][2]); qglVertex2fv(v[1][2]); qglTexCoord2fv(t[2][2]); qglVertex2fv(v[2][2]); qglTexCoord2fv(t[2][1]); qglVertex2fv(v[2][1]);
	qglColor3ub(160,   0, 160);	qglTexCoord2fv(t[2][1]); qglVertex2fv(v[2][1]); qglTexCoord2fv(t[2][0]); qglVertex2fv(v[2][0]); qglTexCoord2fv(t[1][0]); qglVertex2fv(v[1][0]);

	qglColor3ub(120,   0,   0);	qglTexCoord2fv(t[1][1]); qglVertex2fv(v[1][1]); qglTexCoord2fv(t[1][0]); qglVertex2fv(v[1][0]); qglTexCoord2fv(t[0][1]); qglVertex2fv(v[0][1]);
	qglColor3ub(  0, 120,   0);	qglTexCoord2fv(t[1][1]); qglVertex2fv(v[1][1]); qglTexCoord2fv(t[0][1]); qglVertex2fv(v[0][1]); qglTexCoord2fv(t[1][2]); qglVertex2fv(v[1][2]);
	qglColor3ub(  0,   0, 120);	qglTexCoord2fv(t[1][1]); qglVertex2fv(v[1][1]); qglTexCoord2fv(t[1][2]); qglVertex2fv(v[1][2]); qglTexCoord2fv(t[2][1]); qglVertex2fv(v[2][1]);
	qglColor3ub(120,   0, 120);	qglTexCoord2fv(t[1][1]); qglVertex2fv(v[1][1]); qglTexCoord2fv(t[2][1]); qglVertex2fv(v[2][1]); qglTexCoord2fv(t[1][0]); qglVertex2fv(v[1][0]);
	qglEnd();
	qglDisable(GL_TEXTURE_2D);

	qglPopMatrix();
	//*/

	/*
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	//glNormalPointer(type, stride, pointer);
	glColorPointer(size, type, stride, pointer);
	glTexCoordPointer(size, type, stride, pointer);
	glVertexPointer(size, type, stride, pointer);

	glDrawArrays(GLTRIANGLES, 0, count);
	glDrawElements(GL_TRIANGLES, count, type, indices);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	//*/

	/*
	qglBindTexture(GL_TEXTURE_2D, 1);
	qglEnable(GL_TEXTURE_2D);
	qglBegin(GL_TRIANGLES);
	qglColor3f(1, 0, 0);
	qglTexCoord2f(-0, -1); qglVertex3f(-s, s, 1.0f);
	qglTexCoord2f(-0, -0); qglVertex3f(-s,-s, 0.5f);
	qglTexCoord2f(-1, -0); qglVertex3f( s,-s, 0.5f);
	//qglTexCoord2f(1, 0); qglVertex3f( s,-s, 0.5f);
	//qglTexCoord2f(1, 1); qglVertex3f( s, s,-1.0f);
	//qglTexCoord2f(0, 1); qglVertex3f(-s, s, 1.0f);
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
	//*/

	//* !!! pixel
	qglBegin(GL_TRIANGLES);
	if (bDraw_R) { qglColor3ub(160,   0,   0);	qglVertex2fv(v[1][0]); qglVertex2fv(v[0][0]); qglVertex2fv(v[0][1]); }
	if (bDraw_G) { qglColor3ub(  0, 160,   0);	qglVertex2fv(v[0][1]); qglVertex2fv(v[0][2]); qglVertex2fv(v[1][2]); }
	if (bDraw_B) { qglColor3ub(  0,   0, 160);	qglVertex2fv(v[1][2]); qglVertex2fv(v[2][2]); qglVertex2fv(v[2][1]); }
	if (bDraw_M) { qglColor3ub(160,   0, 160);	qglVertex2fv(v[2][1]); qglVertex2fv(v[2][0]); qglVertex2fv(v[1][0]); }
	if (bDraw_r) { qglColor3ub(120,   0,   0);	qglVertex2fv(v[1][1]); qglVertex2fv(v[1][0]); qglVertex2fv(v[0][1]); }
	if (bDraw_g) { qglColor3ub(  0, 120,   0);	qglVertex2fv(v[1][1]); qglVertex2fv(v[0][1]); qglVertex2fv(v[1][2]); }
	if (bDraw_b) { qglColor3ub(  0,   0, 120);	qglVertex2fv(v[1][1]); qglVertex2fv(v[1][2]); qglVertex2fv(v[2][1]); }
	if (bDraw_m) { qglColor3ub(120,   0, 120);	qglVertex2fv(v[1][1]); qglVertex2fv(v[2][1]); qglVertex2fv(v[1][0]); }
	qglEnd();
	//*/

	/* color interpolation
	qglBegin(GL_TRIANGLES);
		qglColor3ub(0, 0, 255);	qglVertex2fv(v[1][1]);
		qglColor3ub(255, 0, 0); qglVertex2fv(v[1][2]);
		qglColor3ub(0, 255, 0); qglVertex2fv(v[2][1]);
	qglEnd();
	//*/

	/* depth test
	qglEnable(GL_DEPTH_TEST);
	qglBegin(GL_TRIANGLES);
		qglColor3ub(255, 0, 0);
		qglVertex3f(.5f, .0f, -1.f);
		qglVertex3f(-.5f, .5f, -2.f);
		qglVertex3f(-.5f, -.5f, -2.f);

		qglColor3ub(0, 0, 255);
		qglVertex3f(-.5f, .0f,-1.f);
		qglVertex3f( .5f,-.5f,-2.f);
		qglVertex3f( .5f, .5f,-2.f);
	qglEnd();
	qglDisable(GL_DEPTH_TEST);//*/

	/* alpha test
	qglAlphaFunc(GL_GREATER, 0.5f);
	qglEnable(GL_ALPHA_TEST);
	qglBegin(GL_TRIANGLES);
		qglColor4ub(0, 0, 255,   0); qglVertex2fv(v[1][1]);
		qglColor4ub(255, 0, 0, 255); qglVertex2fv(v[1][2]);
		qglColor4ub(0, 255, 0, 255); qglVertex2fv(v[2][1]);
	qglEnd();
	qglDisable(GL_ALPHA_TEST);
	//*/

	/* blend test
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable(GL_BLEND);
	qglBegin(GL_TRIANGLES);
		qglColor4ub(0, 0, 255,   0); qglVertex2fv(v[1][1]);
		qglColor4ub(255, 0, 0, 255); qglVertex2fv(v[1][2]);
		qglColor4ub(0, 255, 0, 255); qglVertex2fv(v[2][1]);
	qglEnd();
	qglDisable(GL_BLEND);
	//*/

	/* tex text

	//(GLenum target,
	//GLint level,
	//GLint xoffset,
	//GLint yoffset,
	//GLsizei width,
	//GLsizei height,
	//GLenum format,
	//GLenum type,
	//const void* data);

	uint32_t tsi_data[] = {
		0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
		0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,	0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
		0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,	0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
		0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,	0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
		0xffff00ff, 0xffff00ff, 0xffff00ff, 0xffff00ff, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
		0xffff00ff, 0xffff00ff, 0xffff00ff, 0xffff00ff,	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
		0xffff00ff, 0xffff00ff, 0xffff00ff, 0xffff00ff,	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
		0xffff00ff, 0xffff00ff, 0xffff00ff, 0xffff00ff,	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
	};
	//                              L oX oY  W  H
	qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, 8, GL_RGBA, GL_UNSIGNED_BYTE, tsi_data);
	qglScalef(2.f, 2.f, 2.f);
	qglBindTexture(GL_TEXTURE_2D, 1);
	qglEnable(GL_TEXTURE_2D);
	qglBegin(GL_TRIANGLES);
		qglTexCoord2f(0.f, 1.f); qglColor4ub(  0, 255,   0, 255); qglVertex2fv(v[2][1]);
		qglTexCoord2f(0.f, 0.f); qglColor4ub(  0,   0, 255,   0); qglVertex2fv(v[1][1]);
		qglTexCoord2f(1.f, 0.f); qglColor4ub(255,   0,   0, 255); qglVertex2fv(v[1][2]);

		qglTexCoord2f(0.f, 1.f); qglColor4ub(  0, 255,   0, 255); qglVertex2fv(v[2][1]);
		qglTexCoord2f(1.f, 0.f); qglColor4ub(255,   0,   0, 255); qglVertex2fv(v[1][2]);
		qglTexCoord2f(1.f, 1.f); qglColor4ub(  0,   0, 255,   0); qglVertex2fv(v[2][2]);
	qglEnd();
	qglDisable(GL_TEXTURE_2D);
	//*/

	qglPopMatrix();

	SwapBuffers(qwglGetCurrentDC());
}

void CALLBACK OnTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	if (!pause) {
		angle += 0.1f;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int pmx = 0, pmy = 0;
	switch (uMsg) {
	case WM_QUIT:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		else if (wParam == VK_SPACE)
			pause = !pause;
		else if (wParam == VK_PRIOR)
			angle -= 0.01f;
		else if (wParam == VK_NEXT)
			angle += 0.01f;

		switch (wParam) {
		case 'R': bDraw_R = !bDraw_R; bDraw_r = !bDraw_r; break;
		case 'G': bDraw_G = !bDraw_G; bDraw_g = !bDraw_g; break;
		case 'B': bDraw_B = !bDraw_B; bDraw_b = !bDraw_b; break;
		case 'M': bDraw_M = !bDraw_M; bDraw_m = !bDraw_m; break;
		case VK_SPACE: bPause = !bPause; break;
		}
		break;
	case WM_SIZE:
		size(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_ACTIVATE:
		active = wParam != WA_INACTIVE;
		break;
	case WM_MOUSEMOVE: {
		if (wParam & MK_LBUTTON) {
			//wParam = MK_(CONTROL|SHIFT) | MK_[L|R|M]BUTTON;
			int mx = GET_X_LPARAM(lParam);
			int my = GET_Y_LPARAM(lParam);
			pitch += (my - pmy) * .2f;
			yaw += (mx - pmx) * .2f;
			if (pitch>80.f) pitch = 80.f;
			if (pitch<-80.f) pitch = -80.f;
			if (yaw>180.f) yaw -= 360.f;
			if (yaw<-180.f) yaw += 360.f;
			//SetCapture/GetCapture
			pmx = mx;
			pmy = my;
		}
		if (wParam & MK_RBUTTON) {
			char buf[1024];
			POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			//ScreenToClient(hWnd, &pt);
			snprintf(buf, 1000, "mouse=(%i, %i)\n", pt.x, pt.y);
			OutputDebugStringA(buf);
		}
		break;
	}
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0L;
}

char s[1024*1024] = {0};

//#pragma comment (linker, "/SUBSYSTEM:WINDOWS")

void test(int cx, int cy) {
	size_t count = 100;
	const float s = 1.f;

	qglClearColor(.0f, .0f, 1.f, .0f);

	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglPerspective(90.f, cx/float(cy), .1f, 100.f);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	qglTranslatef(0.f, 0.f, -1.f);

	ts t0;
	//*
	qglBegin(GL_QUADS);
		qglColor3f(1.f, 0.f, 0.f); qglVertex3f(-s, s,-2.f);
		qglColor3f(0.f, 1.f, 0.f); qglVertex3f(-s,-s,-1.f);
		qglColor3f(0.f, 0.f, 1.f); qglVertex3f( s,-s,-2.f);
		qglColor3f(1.f, 0.f, 1.f); qglVertex3f( s, s,-3.f);
	qglEnd();
	//*/
	t0.stop();

	ts t1;
	//*
	qglEnable(GL_BLEND);
	for (size_t i=0; i<count; ++i) {
		qglBegin(GL_QUADS);
			qglColor3f(1.f, 0.f, 0.f); qglVertex3f(-s, s,-2.f);
			qglColor3f(0.f, 1.f, 0.f); qglVertex3f(-s,-s,-1.f);
			qglColor3f(0.f, 0.f, 1.f); qglVertex3f( s,-s,-2.f);
			qglColor3f(1.f, 0.f, 1.f); qglVertex3f( s, s,-3.f);
		qglEnd();
	}
	qglDisable(GL_BLEND);
	//*/
	t1.stop();

	ts t2;
	//*	
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglEnable(GL_TEXTURE_2D);
	for (size_t i=0; i<count; ++i) {
		qglBegin(GL_QUADS);
			qglColor3f(1.f, 0.f, 0.f); qglTexCoord2f(0.f, 1.f); qglVertex3f(-s, s,-2.f);
			qglColor3f(0.f, 1.f, 0.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-s,-s,-1.f);
			qglColor3f(0.f, 0.f, 1.f); qglTexCoord2f(1.f, 0.f); qglVertex3f( s,-s,-2.f);
			qglColor3f(1.f, 0.f, 1.f); qglTexCoord2f(1.f, 1.f); qglVertex3f( s, s,-3.f);
		qglEnd();
	}
	qglDisable(GL_TEXTURE_2D);
	//*/
	t2.stop();

	ts t3;
	//*	
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglEnable(GL_TEXTURE_2D);
	for (size_t i=0; i<count; ++i) {
		qglBegin(GL_QUADS);
			qglColor3f(1.f, 0.f, 0.f); qglTexCoord2f(0.f, 1.f); qglVertex3f(-s, s,-2.f);
			qglColor3f(0.f, 1.f, 0.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-s,-s,-1.f);
			qglColor3f(0.f, 0.f, 1.f); qglTexCoord2f(1.f, 0.f); qglVertex3f( s,-s,-2.f);
			qglColor3f(1.f, 0.f, 1.f); qglTexCoord2f(1.f, 1.f); qglVertex3f( s, s,-3.f);
		qglEnd();
	}
	qglDisable(GL_TEXTURE_2D);
	//*/
	t3.stop();

	ts t4;
	//*
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglEnable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	for (size_t i=0; i<count; ++i) {
		qglBegin(GL_QUADS);
			qglColor3f(1.f, 0.f, 0.f); qglTexCoord2f(0.f, 1.f); qglVertex3f(-s, s,-2.f);
			qglColor3f(0.f, 1.f, 0.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-s,-s,-1.f);
			qglColor3f(0.f, 0.f, 1.f); qglTexCoord2f(1.f, 0.f); qglVertex3f( s,-s,-2.f);
			qglColor3f(1.f, 0.f, 1.f); qglTexCoord2f(1.f, 1.f); qglVertex3f( s, s,-3.f);
		qglEnd();
	}
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
	//*/
	t4.stop();

	ts t5;
	qglAlphaFunc(GL_LESS, .25f);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglEnable(GL_ALPHA_TEST);
	qglEnable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	for (size_t i=0; i<count; ++i) {
		qglBegin(GL_QUADS);
			qglColor4f(1.f, 0.f, 0.f, .5f); qglTexCoord2f(0.f, 1.f); qglVertex3f(-s, s,-2.f);
			qglColor4f(0.f, 1.f, 0.f, 1.f); qglTexCoord2f(0.f, 0.f); qglVertex3f(-s,-s,-1.f);
			qglColor4f(0.f, 0.f, 1.f, .5f); qglTexCoord2f(1.f, 0.f); qglVertex3f( s,-s,-2.f);
			qglColor4f(1.f, 0.f, 1.f, .0f); qglTexCoord2f(1.f, 1.f); qglVertex3f( s, s,-3.f);
		qglEnd();
	}
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	t5.stop();

#	ifndef _DEBUG
	FILE* f = fopen("test.txt", "a");
	fprintf(f, "C=%lli B=%lli Tr=%lli Tm=%lli BTr=%lli ABTm=%lli B=%.1f Tr=%.1f Tm=%.1f BTr=%.1f ABTm=%.1f\n",
		t0.passed(), t1.passed(), t2.passed(), t3.passed(), t4.passed(), t5.passed(),
		t1.passed()/(double)t0.passed(), t2.passed()/(double)t0.passed(), t3.passed()/(double)t0.passed(),
		t4.passed()/(double)t0.passed(), t5.passed()/(double)t0.passed());
	fclose(f);
#	endif
}

/*
Pixel GetPixel(const Image* img, float x, float y)
{
	int px = (int)x; // floor of x
	int py = (int)y; // floor of y
	const int stride = img->width;
	const Pixel* p0 = img->data + px + py * stride; // pointer to first pixel load the four neighboring pixels
	const Pixel& p1 = p0[0 + 0 * stride];
	const Pixel& p2 = p0[1 + 0 * stride];
	const Pixel& p3 = p0[0 + 1 * stride];
	const Pixel& p4 = p0[1 + 1 * stride];
	// Calculate the weights for each pixel
	float fx = x - px;
	float fy = y - py;
	float fx1 = 1.0f - fx;
	float fy1 = 1.0f - fy;
	int w1 = fx1 * fy1 * 256.0f;
	int w2 = fx * fy1 * 256.0f;
	int w3 = fx1 * fy * 256.0f;
	int w4 = fx * fy * 256.0f;
	// Calculate the weighted sum of pixels (for each color channel)
	int outr = p1.r * w1 + p2.r * w2 + p3.r * w3 + p4.r * w4;
	int outg = p1.g * w1 + p2.g * w2 + p3.g * w3 + p4.g * w4;
	int outb = p1.b * w1 + p2.b * w2 + p3.b * w3 + p4.b * w4;
	int outa = p1.a * w1 + p2.a * w2 + p3.a * w3 + p4.a * w4;
	return Pixel(outr >> 8, outg >> 8, outb >> 8, outa >> 8);
}
*/

struct Pixel {
	u8 a;
	Pixel() = default;
	Pixel(u8 _a) : a(_a) {}
};
struct Pic {
	int width = 2;
	int stride = 2;
	Pixel data[4] { 0x00, 0xff, 0xff, 0x00 };
	Pic() = default;
};

Pixel GetPixel(const Pic* pic, float x, float y) {
	int px = (int)x; // floor of x
	int py = (int)y; // floor of y
	const int stride = pic->width;
	const Pixel* p0 = pic->data + pic->stride * py + px; // pointer to first pixel load the four neighboring pixels
	const Pixel& p1 = p0[stride * 0 + 0];
	const Pixel& p2 = p0[stride * 0 + 1];
	const Pixel& p3 = p0[stride * 1 + 0];
	const Pixel& p4 = p0[stride * 1 + 1];
	// Calculate the weights for each pixel
	float fx1 = x - px;
	float fy1 = y - py;
	float fx0 = 1.f - fx1;
	float fy0 = 1.f - fy1;
	int w1 = int(fy0 * fx0 * 255.f);
	int w2 = int(fy0 * fx1 * 255.f);
	int w3 = int(fy1 * fx0 * 255.f);
	int w4 = int(fy1 * fx1 * 255.f);
	// Calculate the weighted sum of pixels (for each color channel)
	int outa = p1.a * w1 + p2.a * w2 + p3.a * w3 + p4.a * w4;
	return Pixel(outa >> 8);
}
#include "immintrin.h"
int _abs(int x) { return x >= 0 ? x : -x; }
int _lmbd1(int v) { _BitScanReverse(&(DWORD&)v, v); return 31 - v; }
int _subc(int src1, int src2) { return (src1 >= src2) ? (((src1 - src2) << 1) + 1) : (src1 << 1); }
int _extu(int src, int csta) { return (int)(((unsigned&)src << csta) >> csta); }

unsigned int udiv(unsigned int num, unsigned int den)
{
	if (den > num || num == 0) return 0;
	if (den == 0) return -1;
	int shift = _lmbd1(den) - _lmbd1(num);
	den <<= shift; // align denominator
	for (int i = shift + 1; i--; num = _subc(num, den));
	shift = 31 - shift;
	return (num << shift) >> shift; // extract quotient
}

int sdiv(int num, int den)
{
	//int sign = (num >> 31) ^ (den >> 31); /* test the sign of inputs */
	int sign = (num ^ den) >> 31;
	num = _abs(num);
	den = _abs(den);
	if (den > num || num == 0) return 0;
	if (den == 0) return -1; // div by 0
	int shift = _lmbd1(den) - _lmbd1(num);
	den <<= shift; // align denominator
	for (int i = shift + 1; i--; num = _subc(num, den));
	num = _extu(num, 31 - shift); // unsigned extraction
	return sign ? -num : num; // attach sign back to quotient
}

//_MM_FROUND_TO_NEAREST_INT
//_MM_FROUND_TO_NEG_INF
//_MM_FROUND_TO_POS_INF
//_MM_FROUND_TO_ZERO
//_MM_FROUND_CUR_DIRECTION
//round_down = floor = -ceil(-x)
//round_up = ceil = -floor(-x)
//round_towards_zero = truncate(x) = sgn(x)*floor(|x|) = -sgn(x)*ceil(-|x|)
//round_away_from_zero = sgn(x)*ceil(|x|) = -sgn(x)*floor(-|x|)
//round_to_the_nearest_integer
//round_half_down = ceil(x-0.5) = -floor(-x+0.5)
//round_half_up = floor(x+0.5) = -ceil(-x-0.5)
//round_half_towards_zero = sgn(x)*ceil(|x|-0.5) = -sgn*floor(-|x|+0.5)
//round_half_away_from_zero = sgn(x)*floor(|x|+0.5) = -sgn*ceil(-|x|-0.5)
//round_half_to_even = ?
//round_half_to_odd = ?

static __m128 _mx_sgn_ps(__m128 x) {
	return _mm_or_ps(_mm_and_ps(_mm_cmpgt_ps(x, _mm_setzero_ps()), _mm_set1_ps(+1.f)), _mm_and_ps(_mm_cmplt_ps(x, _mm_setzero_ps()), _mm_set1_ps(-1.f)));
}
static __m128 _mx_abs_ps(__m128 x) {
	return _mm_max_ps(x, _mm_sub_ps(_mm_setzero_ps(), x));
}

static f32 _round_down(f32 const a) noexcept { return _mm_cvtss_f32(_mm_floor_ps(_mm_set_ss(a))); } // _MM_FROUND_TO_NEG_INF
static f32 _round_up(f32 const a) noexcept { return _mm_cvtss_f32(_mm_ceil_ps(_mm_set_ss(a))); }    // _MM_FROUND_TO_POS_INF
static f32 _round_towards_zero(f32 const a) noexcept { return  _mm_cvtss_f32(_mm_round_ps(_mm_set_ss(a), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)); }
static f32 _round_away_from_zero(f32 const x) noexcept { return _mm_cvtss_f32(_mm_mul_ps(_mx_sgn_ps(_mm_set_ss(x)), _mm_ceil_ps(_mx_abs_ps(_mm_set_ss(x))))); }

static f32 _round_half_down(f32 const a) noexcept { return _mm_cvtss_f32(_mm_ceil_ps(_mm_sub_ps(_mm_set_ss(a), _mm_set_ss(0.5f)))); }
static f32 _round_half_up(f32 const a) noexcept { return _mm_cvtss_f32(_mm_floor_ps(_mm_add_ps(_mm_set_ss(a), _mm_set_ss(0.5f)))); }
static f32 _round_half_towards_zero(f32 const a) noexcept { return _mm_cvtss_f32(_mm_mul_ps(_mx_sgn_ps(_mm_set_ss(a)), _mm_ceil_ps(_mm_sub_ps(_mx_abs_ps(_mm_set_ss(a)), _mm_set1_ps(0.5f))))); }
static f32 _round_half_away_from_zero(f32 const a) noexcept { return _mm_cvtss_f32(_mm_mul_ps(_mx_sgn_ps(_mm_set_ss(a)), _mm_floor_ps(_mm_add_ps(_mx_abs_ps(_mm_set_ss(a)), _mm_set1_ps(0.5f))))); }
static f32 _round_half_to_even(f32 const a) noexcept { return _mm_cvtss_f32(_mm_round_ps(_mm_set_ss(a), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)); }

//_round_math == _round_half_away_from_zero

static void test_round() {
	//_mm_cvtsi32_si128 _mm_(load|store)u_si(8|16|32)

	assert(_round_down(+1.8f) == +1.0f);
	assert(_round_down(+1.5f) == +1.0f);
	assert(_round_down(+1.2f) == +1.0f);
	assert(_round_down(+0.8f) == +0.0f);
	assert(_round_down(+0.5f) == +0.0f);
	assert(_round_down(+0.2f) == +0.0f);
	assert(_round_down(-0.2f) == -1.0f);
	assert(_round_down(-0.5f) == -1.0f);
	assert(_round_down(-0.8f) == -1.0f);
	assert(_round_down(-1.2f) == -2.0f);
	assert(_round_down(-1.5f) == -2.0f);
	assert(_round_down(-1.8f) == -2.0f);

	assert(_round_up(+1.8f) == +2.0f);
	assert(_round_up(+1.5f) == +2.0f);
	assert(_round_up(+1.2f) == +2.0f);
	assert(_round_up(+0.8f) == +1.0f);
	assert(_round_up(+0.5f) == +1.0f);
	assert(_round_up(+0.2f) == +1.0f);
	assert(_round_up(-0.2f) == -0.0f);
	assert(_round_up(-0.5f) == -0.0f);
	assert(_round_up(-0.8f) == -0.0f);
	assert(_round_up(-1.2f) == -1.0f);
	assert(_round_up(-1.5f) == -1.0f);
	assert(_round_up(-1.8f) == -1.0f);

	assert(_round_towards_zero(+1.8f) == +1.0f);
	assert(_round_towards_zero(+1.5f) == +1.0f);
	assert(_round_towards_zero(+1.2f) == +1.0f);
	assert(_round_towards_zero(+0.8f) == +0.0f);
	assert(_round_towards_zero(+0.5f) == +0.0f);
	assert(_round_towards_zero(+0.2f) == +0.0f);
	assert(_round_towards_zero(-0.2f) == -0.0f);
	assert(_round_towards_zero(-0.5f) == -0.0f);
	assert(_round_towards_zero(-0.8f) == -0.0f);
	assert(_round_towards_zero(-1.2f) == -1.0f);
	assert(_round_towards_zero(-1.5f) == -1.0f);
	assert(_round_towards_zero(-1.8f) == -1.0f);

	assert(_round_away_from_zero(+1.8f) == +2.0f);
	assert(_round_away_from_zero(+1.5f) == +2.0f);
	assert(_round_away_from_zero(+1.2f) == +2.0f);
	assert(_round_away_from_zero(+0.8f) == +1.0f);
	assert(_round_away_from_zero(+0.5f) == +1.0f);
	assert(_round_away_from_zero(+0.2f) == +1.0f);
	assert(_round_away_from_zero(-0.2f) == -1.0f);
	assert(_round_away_from_zero(-0.5f) == -1.0f);
	assert(_round_away_from_zero(-0.8f) == -1.0f);
	assert(_round_away_from_zero(-1.2f) == -2.0f);
	assert(_round_away_from_zero(-1.5f) == -2.0f);
	assert(_round_away_from_zero(-1.8f) == -2.0f);
	
	assert(_round_half_down(+1.8f) == +2.0f);
	assert(_round_half_down(+1.5f) == +1.0f);
	assert(_round_half_down(+1.2f) == +1.0f);
	assert(_round_half_down(+0.8f) == +1.0f);
	assert(_round_half_down(+0.5f) == +0.0f);
	assert(_round_half_down(+0.2f) == +0.0f);
	assert(_round_half_down(-0.2f) == -0.0f);
	assert(_round_half_down(-0.5f) == -1.0f);
	assert(_round_half_down(-0.8f) == -1.0f);
	assert(_round_half_down(-1.2f) == -1.0f);
	assert(_round_half_down(-1.5f) == -2.0f);
	assert(_round_half_down(-1.8f) == -2.0f);

	assert(_round_half_up(+1.8f) == +2.0f);
	assert(_round_half_up(+1.5f) == +2.0f);
	assert(_round_half_up(+1.2f) == +1.0f);
	assert(_round_half_up(+0.8f) == +1.0f);
	assert(_round_half_up(+0.5f) == +1.0f);
	assert(_round_half_up(+0.2f) == +0.0f);
	assert(_round_half_up(-0.2f) == -0.0f);
	assert(_round_half_up(-0.5f) == -0.0f);
	assert(_round_half_up(-0.8f) == -1.0f);
	assert(_round_half_up(-1.2f) == -1.0f);
	assert(_round_half_up(-1.5f) == -1.0f);
	assert(_round_half_up(-1.8f) == -2.0f);

	assert(_round_half_towards_zero(+1.8) == +2.0f);
	assert(_round_half_towards_zero(+1.5) == +1.0f);
	assert(_round_half_towards_zero(+1.2) == +1.0f);
	assert(_round_half_towards_zero(+0.8) == +1.0f);
	assert(_round_half_towards_zero(+0.5) == +0.0f);
	assert(_round_half_towards_zero(+0.2) == +0.0f);
	assert(_round_half_towards_zero(-0.2) == -0.0f);
	assert(_round_half_towards_zero(-0.5) == -0.0f);
	assert(_round_half_towards_zero(-0.8) == -1.0f);
	assert(_round_half_towards_zero(-1.2) == -1.0f);
	assert(_round_half_towards_zero(-1.5) == -1.0f);
	assert(_round_half_towards_zero(-1.8) == -2.0f);

	assert(_round_half_away_from_zero(+1.8f) == +2.0f);
	assert(_round_half_away_from_zero(+1.5f) == +2.0f);
	assert(_round_half_away_from_zero(+1.2f) == +1.0f);
	assert(_round_half_away_from_zero(+0.8f) == +1.0f);
	assert(_round_half_away_from_zero(+0.5f) == +1.0f);
	assert(_round_half_away_from_zero(+0.2f) == +0.0f);
	assert(_round_half_away_from_zero(-0.2f) == -0.0f);
	assert(_round_half_away_from_zero(-0.5f) == -1.0f);
	assert(_round_half_away_from_zero(-0.8f) == -1.0f);
	assert(_round_half_away_from_zero(-1.2f) == -1.0f);
	assert(_round_half_away_from_zero(-1.5f) == -2.0f);
	assert(_round_half_away_from_zero(-1.8f) == -2.0f);

	assert(_round_half_to_even(+1.8f) == +2.0f);
	assert(_round_half_to_even(+1.5f) == +2.0f);
	assert(_round_half_to_even(+1.2f) == +1.0f);
	assert(_round_half_to_even(+0.8f) == +1.0f);
	assert(_round_half_to_even(+0.5f) == +0.0f);
	assert(_round_half_to_even(+0.2f) == +0.0f);
	assert(_round_half_to_even(-0.2f) == -0.0f);
	assert(_round_half_to_even(-0.5f) == -0.0f);
	assert(_round_half_to_even(-0.8f) == -1.0f);
	assert(_round_half_to_even(-1.2f) == -1.0f);
	assert(_round_half_to_even(-1.5f) == -2.0f);
	assert(_round_half_to_even(-1.8f) == -2.0f);

	//assert(_round_half_to_odd(+1.8f) == +2.0f);
	//assert(_round_half_to_odd(+1.5f) == +1.0f);
	//assert(_round_half_to_odd(+1.2f) == +1.0f);
	//assert(_round_half_to_odd(+0.8f) == +1.0f);
	//assert(_round_half_to_odd(+0.5f) == +1.0f);
	//assert(_round_half_to_odd(+0.2f) == +0.0f);
	//assert(_round_half_to_odd(-0.2f) == -0.0f);
	//assert(_round_half_to_odd(-0.5f) == -1.0f);
	//assert(_round_half_to_odd(-0.8f) == -1.0f);
	//assert(_round_half_to_odd(-1.2f) == -1.0f);
	//assert(_round_half_to_odd(-1.5f) == -1.0f);
	//assert(_round_half_to_odd(-1.8f) == -2.0f);

	//__m128 cs;
	//__m128 sn = _mm_sincos_ps(&cs, _mm_set1_ps(0.f));
	//printf("%f %f %f %f", sn.m128_f32[0], sn.m128_f32[1], sn.m128_f32[2], sn.m128_f32[3]);
	//printf("%f %f %f %f", cs.m128_f32[0], cs.m128_f32[1], cs.m128_f32[2], cs.m128_f32[3]);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {
	test_round();

	volatile int ss = 15;
	int rr = sdiv(123461, ss);
	printf("%i", rr);
	Pic pic;
	GetPixel(&pic, 1.9f, 0.9f);

	if (png_init(0, 0))
		return 0;


	Image xx;
	xx.Open(_T("z:/android/mcdu-fms2-256.jpg"));
	
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = lpClassName;
	RegisterClass(&wc);

	/*
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	//RECT cr = {600, 400, 80, 60};
	//RECT cr = {600, 400, 160, 120};
	//RECT cr = {600, 400, 320, 240};
	RECT cr = {600, 400, 640, 480};
	//RECT cr = {600, 400, 1280, 960};
	//AdjustWindowRect(&cr, dwStyle, FALSE);
	cr.left -= GetSystemMetrics(SM_CXFRAME);
	cr.top -= GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME);
	cr.right += GetSystemMetrics(SM_CXFRAME)*2;
	cr.bottom += GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME)*2;
	*/

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	//RECT rec{0,0,800,600};
	RECT rec{ 0,0,1600,1200 };
	AdjustWindowRect(&rec, dwStyle, FALSE);

	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 320, 200, 0, 0, hInstance, 0);
	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 320, 240, 0, 0, hInstance, 0);
	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 640, 480, 0, 0, hInstance, 0);
	HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400+rec.left, 200+rec.top, rec.right-rec.left, rec.bottom-rec.top, 0, 0, hInstance, 0);
	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 1600, 900, 0, 0, hInstance, 0);
	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 1600, 1000, 0, 0, hInstance, 0);
	//HWND hWnd = CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, 400, 200, 1920, 1200, 0, 0, hInstance, 0);

	HDC hDC = GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iPixelFormat = ChoosePixelFormat(hDC, &pfd);
	SetPixelFormat(hDC, iPixelFormat, &pfd);
	DescribePixelFormat(hDC, iPixelFormat, sizeof(pfd), &pfd);

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	initLib();

	HGLRC hRC = qwglCreateContext(hDC);
	qwglMakeCurrent(hDC, hRC);

	RECT r;
	GetClientRect(hWnd, &r);
	size(r.right, r.bottom);
	init();

	{
		#if 0
		test(r.right, r.bottom);
		SwapBuffers(qwglGetCurrentDC());
		return 0;
		#endif
	}

	SetTimer(0, 1, 10, OnTimer);

	bool bQuit = false;
	while (!bQuit) {
		for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE); ) {
			if (msg.message == WM_QUIT)
				bQuit = true;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		#if 1
		idle();
		#endif

		static ts ts_;
		double t = ts_.current_sec();
		static const double dt = 2.;
		static double start = t, end = start + dt;
		static i64 frame = 0;
		static i64 startFrame = frame;
		
		if (t > end) {
			double fps = (frame - startFrame) / (end - start);
			start = t;
			end = t + dt;
			startFrame = frame;
			char buf[32];
			sprintf_s(buf, "Ogl: fps=%.0f\n", fps);
			SetWindowTextA(hWnd, buf);
			OutputDebugStringA(buf);
		}

		frame++;
		//Sleep(1);
	}

	qwglMakeCurrent(0, 0);
	qwglDeleteContext(hRC);

	return 0;
}
