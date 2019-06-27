#pragma once
#ifndef TINY_KTX_TINYKTX_H
#define TINY_KTX_TINYKTX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ktx is based on GL (slightly confusing imho) texture format system
// there is format, internal format, type etc.

// we try and expose a more dx12/vulkan/metal style of format
// but obviously still need to GL data so bare with me.
// a TinyKTX_Format is the equivilent to GL/KTX Format and Type
// the API doesn't expose the actual values (which come from GL itself)
// but provide an API call to crack them back into the actual GL values).

// this may look very similar to Vulkan/Dx12 format (its not but related)
// these are even more closely related to my gfx_imageformat library...
typedef enum TinyKTX_Format {
	R4G4_UNORM_PACK8,
	R4G4B4A4_UNORM_PACK16,
	B4G4R4A4_UNORM_PACK16,
	R5G6B5_UNORM_PACK16,
	B5G6R5_UNORM_PACK16,
	R5G5B5A1_UNORM_PACK16,
	B5G5R5A1_UNORM_PACK16,
	A1R5G5B5_UNORM_PACK16,

	R8_UNORM,
	R8_SNORM,
	R8_USCALED,
	R8_SSCALED,
	R8_UINT,
	R8_SINT,
	R8_SRGB,

	R8G8_UNORM,
	R8G8_SNORM,
	R8G8_USCALED,
	R8G8_SSCALED,
	R8G8_UINT,
	R8G8_SINT,
	R8G8_SRGB,

	R8G8B8_UNORM,
	R8G8B8_SNORM,
	R8G8B8_USCALED,
	R8G8B8_SSCALED,
	R8G8B8_UINT,
	R8G8B8_SINT,
	R8G8B8_SRGB,
	B8G8R8_UNORM,
	B8G8R8_SNORM,
	B8G8R8_USCALED,
	B8G8R8_SSCALED,
	B8G8R8_UINT,
	B8G8R8_SINT,
	B8G8R8_SRGB,

	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R8G8B8A8_USCALED,
	R8G8B8A8_SSCALED,
	R8G8B8A8_UINT,
	R8G8B8A8_SINT,
	R8G8B8A8_SRGB,
	B8G8R8A8_UNORM,
	B8G8R8A8_SNORM,
	B8G8R8A8_USCALED,
	B8G8R8A8_SSCALED,
	B8G8R8A8_UINT,
	B8G8R8A8_SINT,
	B8G8R8A8_SRGB,

	A8B8G8R8_UNORM_PACK32,
	A8B8G8R8_SNORM_PACK32,
	A8B8G8R8_USCALED_PACK32,
	A8B8G8R8_SSCALED_PACK32,
	A8B8G8R8_UINT_PACK32,
	A8B8G8R8_SINT_PACK32,
	A8B8G8R8_SRGB_PACK32,

	A2R10G10B10_UNORM_PACK32,
	A2R10G10B10_USCALED_PACK32,
	A2R10G10B10_UINT_PACK32,
	A2B10G10R10_UNORM_PACK32,
	A2B10G10R10_USCALED_PACK32,
	A2B10G10R10_UINT_PACK32,

	R16_UNORM,
	R16_SNORM,
	R16_USCALED,
	R16_SSCALED,
	R16_UINT,
	R16_SINT,
	R16_SFLOAT,
	R16G16_UNORM,
	R16G16_SNORM,
	R16G16_USCALED,
	R16G16_SSCALED,
	R16G16_UINT,
	R16G16_SINT,
	R16G16_SFLOAT,
	R16G16B16_UNORM,
	R16G16B16_SNORM,
	R16G16B16_USCALED,
	R16G16B16_SSCALED,
	R16G16B16_UINT,
	R16G16B16_SINT,
	R16G16B16_SFLOAT,
	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R16G16B16A16_USCALED,
	R16G16B16A16_SSCALED,
	R16G16B16A16_UINT,
	R16G16B16A16_SINT,
	R16G16B16A16_SFLOAT,
	R32_UINT,
	R32_SINT,
	R32_SFLOAT,
	R32G32_UINT,
	R32G32_SINT,
	R32G32_SFLOAT,
	R32G32B32_UINT,
	R32G32B32_SINT,
	R32G32B32_SFLOAT,
	R32G32B32A32_UINT,
	R32G32B32A32_SINT,
	R32G32B32A32_SFLOAT,
	R64_UINT,
	R64_SINT,
	R64_SFLOAT,
	R64G64_UINT,
	R64G64_SINT,
	R64G64_SFLOAT,
	R64G64B64_UINT,
	R64G64B64_SINT,
	R64G64B64_SFLOAT,
	R64G64B64A64_UINT,
	R64G64B64A64_SINT,
	R64G64B64A64_SFLOAT,
	B10G11R11_UFLOAT_PACK32,
	E5B9G9R9_UFLOAT_PACK32,

	D16_UNORM,
	X8_D24_UNORM_PACK32,
	D32_SFLOAT,
	S8_UINT,
	D16_UNORM_S8_UINT,
	D24_UNORM_S8_UINT,
	D32_SFLOAT_S8_UINT,

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,
	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,
	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,
	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,
	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,
	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,
	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,

	PVR_2BPP_BLOCK,
	PVR_2BPPA_BLOCK,
	PVR_4BPP_BLOCK,
	PVR_4BPPA_BLOCK,
	PVR_2BPP_SRGB_BLOCK,
	PVR_2BPPA_SRGB_BLOCK,
	PVR_4BPP_SRGB_BLOCK,
	PVR_4BPPA_SRGB_BLOCK,

	ASTC_4x4_UNORM_BLOCK,
	ASTC_4x4_SRGB_BLOCK,
	ASTC_5x4_UNORM_BLOCK,
	ASTC_5x4_SRGB_BLOCK,
	ASTC_5x5_UNORM_BLOCK,
	ASTC_5x5_SRGB_BLOCK,
	ASTC_6x5_UNORM_BLOCK,
	ASTC_6x5_SRGB_BLOCK,
	ASTC_6x6_UNORM_BLOCK,
	ASTC_6x6_SRGB_BLOCK,
	ASTC_8x5_UNORM_BLOCK,
	ASTC_8x5_SRGB_BLOCK,
	ASTC_8x6_UNORM_BLOCK,
	ASTC_8x6_SRGB_BLOCK,
	ASTC_8x8_UNORM_BLOCK,
	ASTC_8x8_SRGB_BLOCK,
	ASTC_10x5_UNORM_BLOCK,
	ASTC_10x5_SRGB_BLOCK,
	ASTC_10x6_UNORM_BLOCK,
	ASTC_10x6_SRGB_BLOCK,
	ASTC_10x8_UNORM_BLOCK,
	ASTC_10x8_SRGB_BLOCK,
	ASTC_10x10_UNORM_BLOCK,
	ASTC_10x10_SRGB_BLOCK,
	ASTC_12x10_UNORM_BLOCK,
	ASTC_12x10_SRGB_BLOCK,
	ASTC_12x12_UNORM_BLOCK,
	ASTC_12x12_SRGB_BLOCK,
};

typedef uint32_t TinyKTX_Format;

// GL types
#define TINYKTX_GL_BYTE                           0x1400
#define TINYKTX_GL_UNSIGNED_BYTE                  0x1401
#define TINYKTX_GL_SHORT                          0x1402
#define TINYKTX_GL_UNSIGNED_SHORT                 0x1403
#define TINYKTX_GL_INT                            0x1404
#define TINYKTX_GL_UNSIGNED_INT                   0x1405
#define TINYKTX_GL_FLOAT                          0x1406
#define TINYKTX_GL_DOUBLE 												0x140A
#define TINYKTX_GL_HALF_FLOAT                   	0x140B

//
#define TINYKTX_GL_RED                            0x1903
#define TINYKTX_GL_GREEN                          0x1904
#define TINYKTX_GL_BLUE                           0x1905
#define TINYKTX_GL_ALPHA                          0x1906
#define TINYKTX_GL_RGB                            0x1907
#define TINYKTX_GL_RGBA 													0x1908
#define TINYKTX_GL_LUMINANCE                    	0x1909
#define TINYKTX_GL_LUMINANCE_ALPHA              	0x190A

#define TINYKTX_GL_INTENSITY                    0x8049
#define TINYKTX_GL_BGR                          0x80E0
#define TINYKTX_GL_BGRA                         0x80E1
#define TINYKTX_GL_RED_INTEGER                  0x8D94
#define TINYKTX_GL_RGB_INTEGER                  0x8D98
#define TINYKTX_GL_RGBA_INTEGER                 0x8D99
#define TINYKTX_GL_GREEN_INTEGER                0x8D95
#define TINYKTX_GL_BLUE_INTEGER                 0x8D96
#define TINYKTX_GL_ALPHA_INTEGER                0x8D97
#define TINYKTX_GL_BGR_INTEGER                  0x8D9A
#define TINYKTX_GL_BGRA_INTEGER                 0x8D9B
#define TINYKTX_GL_DEPTH_STENCIL                0x84F9

#define GL_SRGB                           			0x8C40
#define GL_SRGB8                          			0x8C41
#define GL_SRGB_ALPHA                     			0x8C42
#define GL_SRGB8_ALPHA8 												0x8C43

#define TINYKTX_GL_UNSIGNED_BYTE_3_3_2          0x8032
#define TINYKTX_GL_UNSIGNED_INT_8_8_8_8         0x8035
#define TINYKTX_GL_UNSIGNED_INT_10_10_10_2      0x8036
#define TINYKTX_GL_UNSIGNED_BYTE_2_3_3_REV      0x8362
#define TINYKTX_GL_UNSIGNED_SHORT_5_6_5         0x8363
#define TINYKTX_GL_UNSIGNED_SHORT_5_6_5_REV     0x8364
#define TINYKTX_GL_UNSIGNED_SHORT_4_4_4_4_REV   0x8365
#define TINYKTX_GL_UNSIGNED_SHORT_1_5_5_5_REV   0x8366
#define TINYKTX_GL_UNSIGNED_INT_8_8_8_8_REV     0x8367
#define TINYKTX_GL_UNSIGNED_INT_2_10_10_10_REV  0x8368
#define TINYKTX_GL_UNSIGNED_INT_24_8            0x84FA
#define TINYKTX_GL_UNSIGNED_INT_5_9_9_9_REV     0x8C3E
#define TINYKTX_GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define TINYKTX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV   0x8DAD

#define TINYKTX_GL_GREEN                       		0x1904
#define TINYKTX_GL_BLUE                        		0x1905
#define TINYKTX_GL_RG                          		0x8227
#define TINYKTX_GL_RG_INTEGER                  		0x8228

#define TINYKTX_GL_R16_SNORM                   		0x8F98
#define TINYKTX_GL_RG16_SNORM                  		0x8F99
#define TINYKTX_GL_R16                         		0x822A
#define TINYKTX_GL_RG16                        		0x822C
#define TINYKTX_GL_RGB8                        		0x8051
#define TINYKTX_GL_RGBA8                       		0x8058
#define TINYKTX_GL_SRGB8                       		0x8C41
#define TINYKTX_GL_SRGB8_ALPHA8                		0x8C43

#define TINYKTX_GL_ALPHA_SNORM                    0x9010
#define TINYKTX_GL_LUMINANCE_SNORM                0x9011
#define TINYKTX_GL_LUMINANCE_ALPHA_SNORM          0x9012
#define TINYKTX_GL_INTENSITY_SNORM                0x9013
#define TINYKTX_GL_RED_SNORM                      0x8F90
#define TINYKTX_GL_RG_SNORM                       0x8F91
#define TINYKTX_GL_RGB_SNORM                      0x8F92
#define TINYKTX_GL_RGBA_SNORM                     0x8F93

#define TINYKTX_GL_ALPHA8_SNORM                   0x9014
#define TINYKTX_GL_LUMINANCE8_SNORM               0x9015
#define TINYKTX_GL_LUMINANCE8_ALPHA8_SNORM        0x9016
#define TINYKTX_GL_INTENSITY8_SNORM               0x9017
#define TINYKTX_GL_ALPHA16_SNORM                  0x9018
#define TINYKTX_GL_LUMINANCE16_SNORM              0x9019
#define TINYKTX_GL_LUMINANCE16_ALPHA16_SNORM      0x901A
#define TINYKTX_GL_INTENSITY16_SNORM              0x901B

#define TINYKTX_GL_RGB9_E5                    		0x8C3D
#define TINYKTX_GL_UNSIGNED_INT_5_9_9_9_REV		   0x8C3E

// compressed formats
#define TINYKTX_GL_ETC1_RGB8_OES                							0x8D64
#define TINYKTX_GL_COMPRESSED_R11_EAC           							0x9270
#define TINYKTX_GL_COMPRESSED_SIGNED_R11_EAC    							0x9271
#define TINYKTX_GL_COMPRESSED_RG11_EAC          							0x9272
#define TINYKTX_GL_COMPRESSED_SIGNED_RG11_EAC   							0x9273
#define TINYKTX_GL_COMPRESSED_RGB8_ETC2         							0x9274
#define TINYKTX_GL_COMPRESSED_SRGB8_ETC2        							0x9275
#define TINYKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2   0x9276
#define TINYKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2  0x9277
#define TINYKTX_GL_COMPRESSED_RGBA8_ETC2_EAC                  0x9278
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC           0x9279

// legacy formats
#define TINYKTX_GL_ALPHA4                       0x803B
#define TINYKTX_GL_ALPHA8                       0x803C
#define TINYKTX_GL_ALPHA12                      0x803D
#define TINYKTX_GL_ALPHA16                      0x803E
#define TINYKTX_GL_LUMINANCE4                   0x803F
#define TINYKTX_GL_LUMINANCE8                   0x8040
#define TINYKTX_GL_LUMINANCE12                  0x8041
#define TINYKTX_GL_LUMINANCE16                  0x8042
#define TINYKTX_GL_LUMINANCE4_ALPHA4            0x8043
#define TINYKTX_GL_LUMINANCE6_ALPHA2            0x8044
#define TINYKTX_GL_LUMINANCE8_ALPHA8            0x8045
#define TINYKTX_GL_LUMINANCE12_ALPHA4           0x8046
#define TINYKTX_GL_LUMINANCE12_ALPHA12          0x8047
#define TINYKTX_GL_LUMINANCE16_ALPHA16          0x8048
#define TINYKTX_GL_INTENSITY4                   0x804A
#define TINYKTX_GL_INTENSITY8                   0x804B
#define TINYKTX_GL_INTENSITY12                  0x804C
#define TINYKTX_GL_INTENSITY16                  0x804D
#define TINYKTX_GL_SLUMINANCE_ALPHA             0x8C44
#define TINYKTX_GL_SLUMINANCE8_ALPHA8           0x8C45
#define TINYKTX_GL_SLUMINANCE                   0x8C46
#define TINYKTX_GL_SLUMINANCE8                  0x8C47
// end legacy formats

#define TINYKTX_MAX_MIPMAPLEVELS 16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TinyKtx_Context* TinyKtx_ContextHandle;

typedef void* (*TinyKtx_AllocFunc)(void* user, size_t size);
typedef void (*TinyKtx_FreeFunc)(void* user, void* memory);
typedef size_t (*TinyKtx_ReadFunc)(void* user, void *buffer, size_t byteCount);
typedef bool (*TinyKtx_SeekFunc)(void* user, int64_t offset);
typedef int64_t (*TinyKtx_TellFunc)(void* user);
typedef void (*TinyKtx_ErrorFunc)(void* user, char const* msg);

typedef struct TinyKtx_Callbacks {
	TinyKtx_ErrorFunc error;
	TinyKtx_AllocFunc alloc;
	TinyKtx_FreeFunc free;
	TinyKtx_ReadFunc read;
	TinyKtx_SeekFunc seek;
	TinyKtx_TellFunc tell;
} TinyKtx_Callbacks;

TinyKtx_ContextHandle TinyKtx_CreateContext(TinyKtx_Callbacks const* callbacks, void* user);
void TinyKtx_DestroyContext(TinyKtx_ContextHandle handle);

// begin should be used if KTX header isn't at offset 0 (pack files etc.)
void TinyKtx_BeginRead(TinyKtx_ContextHandle handle);
// reset like you reuse the context for another file (saves an alloc/free cycle)
void TinyKtx_Reset(TinyKtx_ContextHandle handle);

bool TinyKtx_ReadHeader(TinyKtx_ContextHandle handle);

// this is slow linear search. TODO add iterator style reading of key value pairs
bool TinyKtx_GetValue(TinyKtx_ContextHandle handle, char const* key, void const** value);

bool TinyKtx_Is1D(TinyKtx_ContextHandle handle);
bool TinyKtx_Is2D(TinyKtx_ContextHandle handle);
bool TinyKtx_Is3D(TinyKtx_ContextHandle handle);
bool TinyKtx_IsCubemap(TinyKtx_ContextHandle handle);
bool TinyKtx_IsArray(TinyKtx_ContextHandle handle);

uint32_t TinyKtx_Width(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_Height(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_Depth(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_ArraySlices(TinyKtx_ContextHandle handle);
TinyKTX_Format TinyKtx_Format(TinyKtx_ContextHandle handle);

bool TinyKtx_CrackFormatToGL(TinyKTX_Format format, uint32_t* glformat, uint32_t* gltype);

bool TinyKtx_NeedsGenerationOfMipmaps(TinyKtx_ContextHandle handle);
bool TinyKtx_NeedsEndianCorrecting(TinyKtx_ContextHandle handle);

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_ImageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);

// don't data return by ImageRawData is owned by the context. Don't free it!
void const* TinyKtx_ImageRawData(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);



typedef void (*TinyKtx_WriteFunc)(void* user, void const *buffer, size_t byteCount);

typedef struct TinyKtx_WriteCallbacks {
	TinyKtx_ErrorFunc error;
	TinyKtx_AllocFunc alloc;
	TinyKtx_FreeFunc free;
	TinyKtx_WriteFunc write;
} TinyKtx_WriteCallbacks;

bool TinyKtx_WriteImage(	TinyKtx_WriteCallbacks const* callbacks,
													void* user,
													uint32_t width,
													uint32_t height,
													uint32_t depth,
													uint32_t slices,
													uint32_t mipmaplevels,
													TinyKTX_Format format,
													bool cubemap,
													uint32_t const* mipmapsizes,
													void const** mipmaps);

bool TinyKtx_WriteImageGL(	TinyKtx_WriteCallbacks const* callbacks,
													void* user,
													uint32_t width,
													uint32_t height,
													uint32_t depth,
													uint32_t slices,
													uint32_t mipmaplevels,
													uint32_t format,
													uint32_t internalFormat,
													uint32_t baseFormat,
													uint32_t type,
													uint32_t typeSize,
													bool cubemap,
													uint32_t const* mipmapsizes,
													void const** mipmaps);
#ifdef __cplusplus
};
#endif

#endif // end header