// MIT license see full LICENSE text at end of file
#pragma once
#ifndef TINY_KTX_TINYKTX2_H
#define TINY_KTX_TINYKTX2_H

#ifndef TINYKTX_HAVE_UINTXX_T
#include <stdint.h> 	// for uint32_t and int64_t
#endif
#ifndef TINYKTX_HAVE_BOOL
#include <stdbool.h>	// for bool
#endif
#ifndef TINYKTX_HAVE_SIZE_T
#include <stddef.h>		// for size_t
#endif
#ifndef TINYKTX_HAVE_MEMCPY
#include <string.h> 	// for memcpy
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TinyKtx2_Context *TinyKtx2_ContextHandle;

#define TINYKTX2_MAX_MIPMAPLEVELS 16

typedef void *(*TinyKtx2_AllocFunc)(void *user, size_t size);
typedef void (*TinyKtx2_FreeFunc)(void *user, void *memory);
typedef size_t (*TinyKtx2_ReadFunc)(void *user, void *buffer, size_t byteCount);
typedef bool (*TinyKtx2_SeekFunc)(void *user, int64_t offset);
typedef int64_t (*TinyKtx2_TellFunc)(void *user);
typedef void (*TinyKtx2_ErrorFunc)(void *user, char const *msg);
typedef bool (*TinyKtx2_SuperDecompress)(void* user, void* const sgdData, void const* src, size_t srcSize, void const* dst, size_t dstSize);

typedef struct TinyKtx2_SuperDecompressTableEntry {
	uint32_t superId;
	TinyKtx2_SuperDecompress decompressor;
} TinyKtx2_SuperDecompressTableEntry;

typedef struct TinyKtx2_Callbacks {
	TinyKtx2_ErrorFunc error;
	TinyKtx2_AllocFunc alloc;
	TinyKtx2_FreeFunc free;
	TinyKtx2_ReadFunc read;
	TinyKtx2_SeekFunc seek;
	TinyKtx2_TellFunc tell;

	size_t numSuperDecompressors;
	TinyKtx2_SuperDecompressTableEntry const* superDecompressors;
} TinyKtx2_Callbacks;

TinyKtx2_ContextHandle TinyKtx2_CreateContext(TinyKtx2_Callbacks const *callbacks, void *user);
void TinyKtx2_DestroyContext(TinyKtx_ContextHandle handle);

// reset lets you reuse the context for another file (saves an alloc/free cycle)
void TinyKtx2_Reset(TinyKtx2_ContextHandle handle);

// call this to read the header file should already be at the start of the KTX data
bool TinyKtx2_ReadHeader(TinyKtx2_ContextHandle handle);

// this is slow linear search. TODO add iterator style reading of key value pairs
bool TinyKtx2_GetValue(TinyKtx2_ContextHandle handle, char const *key, void const **value);

bool TinyKtx2_Is1D(TinyKtx2_ContextHandle handle);
bool TinyKtx2_Is2D(TinyKtx2_ContextHandle handle);
bool TinyKtx2_Is3D(TinyKtx2_ContextHandle handle);
bool TinyKtx2_IsCubemap(TinyKtx2_ContextHandle handle);
bool TinyKtx2_IsArray(TinyKtx2_ContextHandle handle);

bool TinyKtx2_Dimensions(TinyKtx2_ContextHandle handle, uint32_t* width, uint32_t* height, uint32_t* depth, uint32_t* slices);
uint32_t TinyKtx2_Width(TinyKtx2_ContextHandle handle);
uint32_t TinyKtx2_Height(TinyKtx2_ContextHandle handle);
uint32_t TinyKtx2_Depth(TinyKtx2_ContextHandle handle);
uint32_t TinyKtx2_ArraySlices(TinyKtx2_ContextHandle handle);

bool TinyKtx2_GetFormatGL(TinyKtx2_ContextHandle handle, uint32_t *glformat, uint32_t *gltype, uint32_t *glinternalformat, uint32_t* typesize, uint32_t* glbaseinternalformat);

bool TinyKtx2_NeedsGenerationOfMipmaps(TinyKtx2_ContextHandle handle);
bool TinyKtx2_NeedsEndianCorrecting(TinyKtx2_ContextHandle handle);

uint32_t TinyKtx2_NumberOfMipmaps(TinyKtx2_ContextHandle handle);
uint32_t TinyKtx2_ImageSize(TinyKtx2_ContextHandle handle, uint32_t mipmaplevel);

bool TinyKtx2_IsMipMapLevelUnpacked(TinyKtx2_ContextHandle handle, uint32_t mipmaplevel);
// this is required to read Unpacked data correctly
uint32_t TinyKtx2_UnpackedRowStride(TinyKtx2_ContextHandle handle, uint32_t mipmaplevel);

// data return by ImageRawData is owned by the context. Don't free it!
void const *TinyKtx2_ImageRawData(TinyKtx2_ContextHandle handle, uint32_t mipmaplevel);

typedef void (*TinyKtx2_WriteFunc)(void *user, void const *buffer, size_t byteCount);

typedef struct TinyKtx2_WriteCallbacks {
	TinyKtx2_ErrorFunc error;
	TinyKtx2_AllocFunc alloc;
	TinyKtx2_FreeFunc free;
	TinyKtx2_WriteFunc write;
} TinyKtx2_WriteCallbacks;


bool TinyKtx2_WriteImageGL(TinyKtx2_WriteCallbacks const *callbacks,
													void *user,
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
													uint32_t const *mipmapsizes,
													void const **mipmaps);

// ktx v1 is based on GL (slightly confusing imho) texture format system
// there is format, internal format, type etc.

// we try and expose a more dx12/vulkan/metal style of format
// but obviously still need to GL data so bare with me.
// a TinyKTX_Format is the equivilent to GL/KTX Format and Type
// the API doesn't expose the actual values (which come from GL itself)
// but provide an API call to crack them back into the actual GL values).

// Ktx v2 is based on VkFormat and also DFD, so we now base the
// enumeration values of TinyKtx_Format on the Vkformat values where possible

#ifndef TINYKTX_DEFINED
typedef enum TinyImageFormat_VkFormat {
	TIF_VK_FORMAT_UNDEFINED = 0,
	TIF_VK_FORMAT_R4G4_UNORM_PACK8 = 1,
	TIF_VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
	TIF_VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
	TIF_VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
	TIF_VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
	TIF_VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
	TIF_VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
	TIF_VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
	TIF_VK_FORMAT_R8_UNORM = 9,
	TIF_VK_FORMAT_R8_SNORM = 10,
	TIF_VK_FORMAT_R8_USCALED = 11,
	TIF_VK_FORMAT_R8_SSCALED = 12,
	TIF_VK_FORMAT_R8_UINT = 13,
	TIF_VK_FORMAT_R8_SINT = 14,
	TIF_VK_FORMAT_R8_SRGB = 15,
	TIF_VK_FORMAT_R8G8_UNORM = 16,
	TIF_VK_FORMAT_R8G8_SNORM = 17,
	TIF_VK_FORMAT_R8G8_USCALED = 18,
	TIF_VK_FORMAT_R8G8_SSCALED = 19,
	TIF_VK_FORMAT_R8G8_UINT = 20,
	TIF_VK_FORMAT_R8G8_SINT = 21,
	TIF_VK_FORMAT_R8G8_SRGB = 22,
	TIF_VK_FORMAT_R8G8B8_UNORM = 23,
	TIF_VK_FORMAT_R8G8B8_SNORM = 24,
	TIF_VK_FORMAT_R8G8B8_USCALED = 25,
	TIF_VK_FORMAT_R8G8B8_SSCALED = 26,
	TIF_VK_FORMAT_R8G8B8_UINT = 27,
	TIF_VK_FORMAT_R8G8B8_SINT = 28,
	TIF_VK_FORMAT_R8G8B8_SRGB = 29,
	TIF_VK_FORMAT_B8G8R8_UNORM = 30,
	TIF_VK_FORMAT_B8G8R8_SNORM = 31,
	TIF_VK_FORMAT_B8G8R8_USCALED = 32,
	TIF_VK_FORMAT_B8G8R8_SSCALED = 33,
	TIF_VK_FORMAT_B8G8R8_UINT = 34,
	TIF_VK_FORMAT_B8G8R8_SINT = 35,
	TIF_VK_FORMAT_B8G8R8_SRGB = 36,
	TIF_VK_FORMAT_R8G8B8A8_UNORM = 37,
	TIF_VK_FORMAT_R8G8B8A8_SNORM = 38,
	TIF_VK_FORMAT_R8G8B8A8_USCALED = 39,
	TIF_VK_FORMAT_R8G8B8A8_SSCALED = 40,
	TIF_VK_FORMAT_R8G8B8A8_UINT = 41,
	TIF_VK_FORMAT_R8G8B8A8_SINT = 42,
	TIF_VK_FORMAT_R8G8B8A8_SRGB = 43,
	TIF_VK_FORMAT_B8G8R8A8_UNORM = 44,
	TIF_VK_FORMAT_B8G8R8A8_SNORM = 45,
	TIF_VK_FORMAT_B8G8R8A8_USCALED = 46,
	TIF_VK_FORMAT_B8G8R8A8_SSCALED = 47,
	TIF_VK_FORMAT_B8G8R8A8_UINT = 48,
	TIF_VK_FORMAT_B8G8R8A8_SINT = 49,
	TIF_VK_FORMAT_B8G8R8A8_SRGB = 50,
	TIF_VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
	TIF_VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
	TIF_VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
	TIF_VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
	TIF_VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
	TIF_VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
	TIF_VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
	TIF_VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
	TIF_VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
	TIF_VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
	TIF_VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
	TIF_VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
	TIF_VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
	TIF_VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
	TIF_VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
	TIF_VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
	TIF_VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
	TIF_VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
	TIF_VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
	TIF_VK_FORMAT_R16_UNORM = 70,
	TIF_VK_FORMAT_R16_SNORM = 71,
	TIF_VK_FORMAT_R16_USCALED = 72,
	TIF_VK_FORMAT_R16_SSCALED = 73,
	TIF_VK_FORMAT_R16_UINT = 74,
	TIF_VK_FORMAT_R16_SINT = 75,
	TIF_VK_FORMAT_R16_SFLOAT = 76,
	TIF_VK_FORMAT_R16G16_UNORM = 77,
	TIF_VK_FORMAT_R16G16_SNORM = 78,
	TIF_VK_FORMAT_R16G16_USCALED = 79,
	TIF_VK_FORMAT_R16G16_SSCALED = 80,
	TIF_VK_FORMAT_R16G16_UINT = 81,
	TIF_VK_FORMAT_R16G16_SINT = 82,
	TIF_VK_FORMAT_R16G16_SFLOAT = 83,
	TIF_VK_FORMAT_R16G16B16_UNORM = 84,
	TIF_VK_FORMAT_R16G16B16_SNORM = 85,
	TIF_VK_FORMAT_R16G16B16_USCALED = 86,
	TIF_VK_FORMAT_R16G16B16_SSCALED = 87,
	TIF_VK_FORMAT_R16G16B16_UINT = 88,
	TIF_VK_FORMAT_R16G16B16_SINT = 89,
	TIF_VK_FORMAT_R16G16B16_SFLOAT = 90,
	TIF_VK_FORMAT_R16G16B16A16_UNORM = 91,
	TIF_VK_FORMAT_R16G16B16A16_SNORM = 92,
	TIF_VK_FORMAT_R16G16B16A16_USCALED = 93,
	TIF_VK_FORMAT_R16G16B16A16_SSCALED = 94,
	TIF_VK_FORMAT_R16G16B16A16_UINT = 95,
	TIF_VK_FORMAT_R16G16B16A16_SINT = 96,
	TIF_VK_FORMAT_R16G16B16A16_SFLOAT = 97,
	TIF_VK_FORMAT_R32_UINT = 98,
	TIF_VK_FORMAT_R32_SINT = 99,
	TIF_VK_FORMAT_R32_SFLOAT = 100,
	TIF_VK_FORMAT_R32G32_UINT = 101,
	TIF_VK_FORMAT_R32G32_SINT = 102,
	TIF_VK_FORMAT_R32G32_SFLOAT = 103,
	TIF_VK_FORMAT_R32G32B32_UINT = 104,
	TIF_VK_FORMAT_R32G32B32_SINT = 105,
	TIF_VK_FORMAT_R32G32B32_SFLOAT = 106,
	TIF_VK_FORMAT_R32G32B32A32_UINT = 107,
	TIF_VK_FORMAT_R32G32B32A32_SINT = 108,
	TIF_VK_FORMAT_R32G32B32A32_SFLOAT = 109,
	TIF_VK_FORMAT_R64_UINT = 110,
	TIF_VK_FORMAT_R64_SINT = 111,
	TIF_VK_FORMAT_R64_SFLOAT = 112,
	TIF_VK_FORMAT_R64G64_UINT = 113,
	TIF_VK_FORMAT_R64G64_SINT = 114,
	TIF_VK_FORMAT_R64G64_SFLOAT = 115,
	TIF_VK_FORMAT_R64G64B64_UINT = 116,
	TIF_VK_FORMAT_R64G64B64_SINT = 117,
	TIF_VK_FORMAT_R64G64B64_SFLOAT = 118,
	TIF_VK_FORMAT_R64G64B64A64_UINT = 119,
	TIF_VK_FORMAT_R64G64B64A64_SINT = 120,
	TIF_VK_FORMAT_R64G64B64A64_SFLOAT = 121,
	TIF_VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
	TIF_VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
	TIF_VK_FORMAT_D16_UNORM = 124,
	TIF_VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
	TIF_VK_FORMAT_D32_SFLOAT = 126,
	TIF_VK_FORMAT_S8_UINT = 127,
	TIF_VK_FORMAT_D16_UNORM_S8_UINT = 128,
	TIF_VK_FORMAT_D24_UNORM_S8_UINT = 129,
	TIF_VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
	TIF_VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
	TIF_VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
	TIF_VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
	TIF_VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
	TIF_VK_FORMAT_BC2_UNORM_BLOCK = 135,
	TIF_VK_FORMAT_BC2_SRGB_BLOCK = 136,
	TIF_VK_FORMAT_BC3_UNORM_BLOCK = 137,
	TIF_VK_FORMAT_BC3_SRGB_BLOCK = 138,
	TIF_VK_FORMAT_BC4_UNORM_BLOCK = 139,
	TIF_VK_FORMAT_BC4_SNORM_BLOCK = 140,
	TIF_VK_FORMAT_BC5_UNORM_BLOCK = 141,
	TIF_VK_FORMAT_BC5_SNORM_BLOCK = 142,
	TIF_VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
	TIF_VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
	TIF_VK_FORMAT_BC7_UNORM_BLOCK = 145,
	TIF_VK_FORMAT_BC7_SRGB_BLOCK = 146,
	TIF_VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
	TIF_VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
	TIF_VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
	TIF_VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
	TIF_VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
	TIF_VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
	TIF_VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
	TIF_VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
	TIF_VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
	TIF_VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
	TIF_VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
	TIF_VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
	TIF_VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
	TIF_VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
	TIF_VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
	TIF_VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
	TIF_VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
	TIF_VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
	TIF_VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
	TIF_VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
	TIF_VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
	TIF_VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
	TIF_VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
	TIF_VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
	TIF_VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
	TIF_VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
	TIF_VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
	TIF_VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
	TIF_VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
	TIF_VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
	TIF_VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
	TIF_VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
	TIF_VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
	TIF_VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
	TIF_VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
	TIF_VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
	TIF_VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
	TIF_VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,

	TIF_VK_FORMAT_G8B8G8R8_422_UNORM = 1000156000,
	TIF_VK_FORMAT_B8G8R8G8_422_UNORM = 1000156001,
	TIF_VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM = 1000156002,
	TIF_VK_FORMAT_G8_B8R8_2PLANE_420_UNORM = 1000156003,
	TIF_VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM = 1000156004,
	TIF_VK_FORMAT_G8_B8R8_2PLANE_422_UNORM = 1000156005,
	TIF_VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM = 1000156006,
	TIF_VK_FORMAT_R10X6_UNORM_PACK16 = 1000156007,
	TIF_VK_FORMAT_R10X6G10X6_UNORM_2PACK16 = 1000156008,
	TIF_VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,
	TIF_VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
	TIF_VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
	TIF_VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
	TIF_VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
	TIF_VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
	TIF_VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
	TIF_VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
	TIF_VK_FORMAT_R12X4_UNORM_PACK16 = 1000156017,
	TIF_VK_FORMAT_R12X4G12X4_UNORM_2PACK16 = 1000156018,
	TIF_VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
	TIF_VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
	TIF_VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
	TIF_VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
	TIF_VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
	TIF_VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
	TIF_VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
	TIF_VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
	TIF_VK_FORMAT_G16B16G16R16_422_UNORM = 1000156027,
	TIF_VK_FORMAT_B16G16R16G16_422_UNORM = 1000156028,
	TIF_VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM = 1000156029,
	TIF_VK_FORMAT_G16_B16R16_2PLANE_420_UNORM = 1000156030,
	TIF_VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM = 1000156031,
	TIF_VK_FORMAT_G16_B16R16_2PLANE_422_UNORM = 1000156032,
	TIF_VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM = 1000156033,
	TIF_VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
	TIF_VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
	TIF_VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
	TIF_VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
	TIF_VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
	TIF_VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
	TIF_VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
	TIF_VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
} TinyKTX_VkFormat;

#define TINYKTX_MEV(x) TKTX_##x = TIF_VK_FORMAT_##x
typedef enum TinyKtx_Format {
	TINYKTX_MEV(UNDEFINED),
	TINYKTX_MEV(R4G4_UNORM_PACK8),
	TINYKTX_MEV(R4G4B4A4_UNORM_PACK16),
	TINYKTX_MEV(B4G4R4A4_UNORM_PACK16),
	TINYKTX_MEV(R5G6B5_UNORM_PACK16),
	TINYKTX_MEV(B5G6R5_UNORM_PACK16),
	TINYKTX_MEV(R5G5B5A1_UNORM_PACK16),
	TINYKTX_MEV(B5G5R5A1_UNORM_PACK16),
	TINYKTX_MEV(A1R5G5B5_UNORM_PACK16),

	TINYKTX_MEV(R8_UNORM),
	TINYKTX_MEV(R8_SNORM),
	TINYKTX_MEV(R8_UINT),
	TINYKTX_MEV(R8_SINT),
	TINYKTX_MEV(R8_SRGB),

	TINYKTX_MEV(R8G8_UNORM),
	TINYKTX_MEV(R8G8_SNORM),
	TINYKTX_MEV(R8G8_UINT),
	TINYKTX_MEV(R8G8_SINT),
	TINYKTX_MEV(R8G8_SRGB),

	TINYKTX_MEV(R8G8B8_UNORM),
	TINYKTX_MEV(R8G8B8_SNORM),
	TINYKTX_MEV(R8G8B8_UINT),
	TINYKTX_MEV(R8G8B8_SINT),
	TINYKTX_MEV(R8G8B8_SRGB),
	TINYKTX_MEV(B8G8R8_UNORM),
	TINYKTX_MEV(B8G8R8_SNORM),
	TINYKTX_MEV(B8G8R8_UINT),
	TINYKTX_MEV(B8G8R8_SINT),
	TINYKTX_MEV(B8G8R8_SRGB),

	TINYKTX_MEV(R8G8B8A8_UNORM),
	TINYKTX_MEV(R8G8B8A8_SNORM),
	TINYKTX_MEV(R8G8B8A8_UINT),
	TINYKTX_MEV(R8G8B8A8_SINT),
	TINYKTX_MEV(R8G8B8A8_SRGB),
	TINYKTX_MEV(B8G8R8A8_UNORM),
	TINYKTX_MEV(B8G8R8A8_SNORM),
	TINYKTX_MEV(B8G8R8A8_UINT),
	TINYKTX_MEV(B8G8R8A8_SINT),
	TINYKTX_MEV(B8G8R8A8_SRGB),

	TINYKTX_MEV(A8B8G8R8_UNORM_PACK32),
	TINYKTX_MEV(A8B8G8R8_SNORM_PACK32),
	TINYKTX_MEV(A8B8G8R8_UINT_PACK32),
	TINYKTX_MEV(A8B8G8R8_SINT_PACK32),
	TINYKTX_MEV(A8B8G8R8_SRGB_PACK32),

	TINYKTX_MEV(E5B9G9R9_UFLOAT_PACK32),
	TINYKTX_MEV(A2R10G10B10_UNORM_PACK32),
	TINYKTX_MEV(A2R10G10B10_UINT_PACK32),
	TINYKTX_MEV(A2B10G10R10_UNORM_PACK32),
	TINYKTX_MEV(A2B10G10R10_UINT_PACK32),
	TINYKTX_MEV(B10G11R11_UFLOAT_PACK32),

	TINYKTX_MEV(R16_UNORM),
	TINYKTX_MEV(R16_SNORM),
	TINYKTX_MEV(R16_UINT),
	TINYKTX_MEV(R16_SINT),
	TINYKTX_MEV(R16_SFLOAT),
	TINYKTX_MEV(R16G16_UNORM),
	TINYKTX_MEV(R16G16_SNORM),
	TINYKTX_MEV(R16G16_UINT),
	TINYKTX_MEV(R16G16_SINT),
	TINYKTX_MEV(R16G16_SFLOAT),
	TINYKTX_MEV(R16G16B16_UNORM),
	TINYKTX_MEV(R16G16B16_SNORM),
	TINYKTX_MEV(R16G16B16_UINT),
	TINYKTX_MEV(R16G16B16_SINT),
	TINYKTX_MEV(R16G16B16_SFLOAT),
	TINYKTX_MEV(R16G16B16A16_UNORM),
	TINYKTX_MEV(R16G16B16A16_SNORM),
	TINYKTX_MEV(R16G16B16A16_UINT),
	TINYKTX_MEV(R16G16B16A16_SINT),
	TINYKTX_MEV(R16G16B16A16_SFLOAT),
	TINYKTX_MEV(R32_UINT),
	TINYKTX_MEV(R32_SINT),
	TINYKTX_MEV(R32_SFLOAT),
	TINYKTX_MEV(R32G32_UINT),
	TINYKTX_MEV(R32G32_SINT),
	TINYKTX_MEV(R32G32_SFLOAT),
	TINYKTX_MEV(R32G32B32_UINT),
	TINYKTX_MEV(R32G32B32_SINT),
	TINYKTX_MEV(R32G32B32_SFLOAT),
	TINYKTX_MEV(R32G32B32A32_UINT),
	TINYKTX_MEV(R32G32B32A32_SINT),
	TINYKTX_MEV(R32G32B32A32_SFLOAT),

	TINYKTX_MEV(BC1_RGB_UNORM_BLOCK),
	TINYKTX_MEV(BC1_RGB_SRGB_BLOCK),
	TINYKTX_MEV(BC1_RGBA_UNORM_BLOCK),
	TINYKTX_MEV(BC1_RGBA_SRGB_BLOCK),
	TINYKTX_MEV(BC2_UNORM_BLOCK),
	TINYKTX_MEV(BC2_SRGB_BLOCK),
	TINYKTX_MEV(BC3_UNORM_BLOCK),
	TINYKTX_MEV(BC3_SRGB_BLOCK),
	TINYKTX_MEV(BC4_UNORM_BLOCK),
	TINYKTX_MEV(BC4_SNORM_BLOCK),
	TINYKTX_MEV(BC5_UNORM_BLOCK),
	TINYKTX_MEV(BC5_SNORM_BLOCK),
	TINYKTX_MEV(BC6H_UFLOAT_BLOCK),
	TINYKTX_MEV(BC6H_SFLOAT_BLOCK),
	TINYKTX_MEV(BC7_UNORM_BLOCK),
	TINYKTX_MEV(BC7_SRGB_BLOCK),

	TINYKTX_MEV(ETC2_R8G8B8_UNORM_BLOCK),
	TINYKTX_MEV(ETC2_R8G8B8A1_UNORM_BLOCK),
	TINYKTX_MEV(ETC2_R8G8B8A8_UNORM_BLOCK),
	TINYKTX_MEV(ETC2_R8G8B8_SRGB_BLOCK),
	TINYKTX_MEV(ETC2_R8G8B8A1_SRGB_BLOCK),
	TINYKTX_MEV(ETC2_R8G8B8A8_SRGB_BLOCK),
	TINYKTX_MEV(EAC_R11_UNORM_BLOCK),
	TINYKTX_MEV(EAC_R11G11_UNORM_BLOCK),
	TINYKTX_MEV(EAC_R11_SNORM_BLOCK),
	TINYKTX_MEV(EAC_R11G11_SNORM_BLOCK),

	TKTX_PVR_2BPP_BLOCK = TIF_VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
	TKTX_PVR_2BPPA_BLOCK = TIF_VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
	TKTX_PVR_4BPP_BLOCK = TIF_VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
	TKTX_PVR_4BPPA_BLOCK = TIF_VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
	TKTX_PVR_2BPP_SRGB_BLOCK = TIF_VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
	TKTX_PVR_2BPPA_SRGB_BLOCK = TIF_VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
	TKTX_PVR_4BPP_SRGB_BLOCK = TIF_VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
	TKTX_PVR_4BPPA_SRGB_BLOCK = TIF_VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,

	TINYKTX_MEV(ASTC_4x4_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_4x4_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_5x4_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_5x4_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_5x5_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_5x5_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_6x5_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_6x5_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_6x6_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_6x6_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_8x5_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_8x5_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_8x6_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_8x6_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_8x8_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_8x8_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_10x5_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_10x5_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_10x6_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_10x6_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_10x8_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_10x8_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_10x10_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_10x10_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_12x10_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_12x10_SRGB_BLOCK),
	TINYKTX_MEV(ASTC_12x12_UNORM_BLOCK),
	TINYKTX_MEV(ASTC_12x12_SRGB_BLOCK),

} TinyKtx_Format;
#undef TINYKTX_MEV

#define TINYKTX_DEFINED
#endif

TinyKtx_Format TinyKtx_GetFormat(TinyKtx2_ContextHandle handle);
bool TinyKtx2_WriteImage(TinyKtx2_WriteCallbacks const *callbacks,
												void *user,
												uint32_t width,
												uint32_t height,
												uint32_t depth,
												uint32_t slices,
												uint32_t mipmaplevels,
												TinyKtx_Format format,
												bool cubemap,
												uint32_t const *mipmapsizes,
												void const **mipmaps);

#ifdef TINYKTX2_IMPLEMENTATION

typedef struct TinyKtx2_KeyValuePair {
	uint32_t size;
} TinyKtx2_KeyValuePair; // followed by at least size bytes (aligned to 4)

typedef struct TinyKtx2_HeaderV2 {
	uint8_t identifier[12];
	TinyKtx_Format vkFormat;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t arrayElementCount;
	uint32_t faceCount;
	uint32_t levelCount;
	uint32_t supercompressionScheme;

	uint32_t dfdByteOffset;
	uint32_t dfdByteLength;
	uint32_t kvdByteOffset;
	uint32_t kvdByteLength;
	uint64_t sgdByteOffset;
	uint64_t sgdByteLength;
} TinyKtx2_Header;

typedef struct TinyKtx2_Level {
	uint64_t byteOffset;
	uint64_t byteLength;
	uint64_t uncompressedByteLength;
} TinyKtx2_Level;

typedef enum TinyKtx2_SuperCompressionScheme {
	TKTX2_SUPERCOMPRESSION_NONE = 0,
	TKTX2_SUPERCOMPRESSION_CRN = 1,
	TKTX2_SUPERCOMPRESSION_ZLIB = 2,
	TKTX2_SUPERCOMPRESSION_ZSTD = 3,
} TinyKtx2_SuperCompressionScheme;

typedef struct TinyKtx2_Context {
	TinyKtx2_Callbacks callbacks;
	void *user;
	uint64_t headerPos;
	uint64_t firstImagePos;

	TinyKtx2_Header header;

	TinyKtx2_KeyValuePair const *keyData;
	bool headerValid;
	bool sameEndian;
	void* sgdData;

	TinyKtx2_Level levels[TINYKTX_MAX_MIPMAPLEVELS];
	uint8_t const *mipmaps[TINYKTX_MAX_MIPMAPLEVELS];

} TinyKtx2_Context;


static uint8_t TinyKtx2_fileIdentifier[12] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

static void TinyKtx2_NullErrorFunc(void *user, char const *msg) {}

TinyKtx2_ContextHandle TinyKtx2_CreateContext(TinyKtx2_Callbacks const *callbacks, void *user) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) callbacks->alloc(user, sizeof(TinyKtx2_Context));
	if (ctx == NULL)
		return NULL;

	memset(ctx, 0, sizeof(TinyKtx2_Context));
	memcpy(&ctx->callbacks, callbacks, sizeof(TinyKtx2_Callbacks));
	ctx->user = user;
	if (ctx->callbacks.error == NULL) {
		ctx->callbacks.error = &TinyKtx_NullErrorFunc;
	}

	if (ctx->callbacks.read == NULL) {
		ctx->callbacks.error(user, "TinyKtx must have read callback");
		return NULL;
	}
	if (ctx->callbacks.alloc == NULL) {
		ctx->callbacks.error(user, "TinyKtx must have alloc callback");
		return NULL;
	}
	if (ctx->callbacks.free == NULL) {
		ctx->callbacks.error(user, "TinyKtx must have free callback");
		return NULL;
	}
	if (ctx->callbacks.seek == NULL) {
		ctx->callbacks.error(user, "TinyKtx must have seek callback");
		return NULL;
	}
	if (ctx->callbacks.tell == NULL) {
		ctx->callbacks.error(user, "TinyKtx must have tell callback");
		return NULL;
	}

	TinyKtx2_Reset(ctx);

	return ctx;
}

void TinyKtx2_DestroyContext(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return;
	TinyKtx2_Reset(handle);

	ctx->callbacks.free(ctx->user, ctx);
}

void TinyKtx2_Reset(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return;

	// backup user provided callbacks and data
	TinyKtx2_Callbacks callbacks;
	memcpy(&callbacks, &ctx->callbacks, sizeof(TinyKtx_Callbacks));
	void *user = ctx->user;

	// free any super compression global data we've allocated
	if (ctx->sgdData != NULL) {
		callbacks.free(user, (void *) ctx->sgdData);
	}

	// free memory of sub data
	if (ctx->keyData != NULL) {
		callbacks.free(user, (void *) ctx->keyData);
	}

	for (int i = 0; i < TINYKTX_MAX_MIPMAPLEVELS; ++i) {
		if (ctx->mipmaps[i] != NULL) {
			callbacks.free(user, (void *) ctx->mipmaps[i]);
		}
	}

	// reset to default state
	memset(ctx, 0, sizeof(TinyKtx_Context));
	memcpy(&ctx->callbacks, &callbacks, sizeof(TinyKtx_Callbacks));
	ctx->user = user;

}


bool TinyKtx2_ReadHeader(TinyKtx_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;


	ctx->headerPos = ctx->callbacks.tell(ctx->user);
	ctx->callbacks.read(ctx->user, &ctx->header, sizeof(TinyKtx2_Header));

	if (memcmp(&ctx->header.identifier, TinyKtx2_fileIdentifier, 12) != 0) {
		ctx->callbacks.error(ctx->user, "Not a KTX  V2 file or corrupted as identified isn't valid");
		return false;
	}

	if (ctx->header.faceCount != 1 && ctx->header.faceCount != 6) {
		ctx->callbacks.error(ctx->user, "no. of Faces must be 1 or 6");
		return false;
	}
	// cap level to max
	if(ctx->header.levelCount >= TINYKTX2_MAX_MIPMAPLEVELS) {
		ctx->header.levelCount = TINYKTX2_MAX_MIPMAPLEVELS;
	}
	// 0 level count means wants mip maps from the 1 stored
	uint32_t const levelCount = ctx->header.levelCount ? ctx->header.levelCount : 1;

	ctx->callbacks.read(ctx->user, &ctx->levels, sizeof(TinyKtx2_Header) * levelCount);

	if(ctx->header.sgdByteLength > 0) {
		ctx->sgdData = ctx->callbacks.alloc(ctx->user, ctx->header.sgdByteLength);
		ctx->callbacks.seek(ctx->user, ctx->header.sgdByteOffset);
		ctx->callbacks.read(ctx->user, ctx->sgdData, ctx->header.sgdByteLength);
	}

	return true;
}

bool TinyKtx2_GetValue(TinyKtx2_ContextHandle handle, char const *key, void const **value) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	if (ctx->keyData == NULL) {
		ctx->callbacks.error(ctx->user, "No key value data in this KTX");
		return false;
	}

	TinyKtx2_KeyValuePair const *curKey = ctx->keyData;
	while (((uint8_t *) curKey - (uint8_t *) ctx->keyData) < ctx->header.bytesOfKeyValueData) {
		char const *kvp = (char const *) curKey;

		if (strcmp(kvp, key) == 0) {
			size_t sl = strlen(kvp);
			*value = (void const *) (kvp + sl);
			return true;
		}
		curKey = curKey + ((curKey->size + 3u) & ~3u);
	}
	return false;
}

bool TinyKtx2_Is1D(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}
	return (ctx->header.pixelHeight <= 1) && (ctx->header.pixelDepth <= 1 );
}
bool TinyKtx2_Is2D(TinyKtx_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight > 1 && ctx->header.pixelDepth <= 1);
}
bool TinyKtx2_Is3D(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight > 1 && ctx->header.pixelDepth > 1);
}

bool TinyKtx2_IsCubemap(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfFaces == 6);
}
bool TinyKtx2_IsArray(TinyKtx2_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfArrayElements > 1);
}

bool TinyKtx2_Dimensions(TinyKtx2_ContextHandle handle,
												uint32_t *width,
												uint32_t *height,
												uint32_t *depth,
												uint32_t *slices) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}
	if (width)
		*width = ctx->header.pixelWidth;
	if (height)
		*height = ctx->header.pixelWidth;
	if (depth)
		*depth = ctx->header.pixelDepth;
	if (slices)
		*slices = ctx->header.numberOfArrayElements;
	return true;
}

uint32_t TinyKtx2_Width(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}
	return ctx->header.pixelWidth;

}

uint32_t TinyKtx2_Height(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.pixelHeight;
}

uint32_t TinyKtx2_Depth(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}
	return ctx->header.pixelDepth;
}

uint32_t TinyKtx2_ArraySlices(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.numberOfArrayElements;
}

uint32_t TinyKtx2_NumberOfMipmaps(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}
	return ctx->header.levelCount ? ctx->header.levelCount : 1;
}

bool TinyKtx2_NeedsGenerationOfMipmaps(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.levelCount == 0;
}

uint32_t TinyKtx_ImageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;

	if (mipmaplevel >= ctx->header.levelCount) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}
	if (mipmaplevel >= TINYKTX2_MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}

	return ctx->levels[mipmaplevel].uncompressedByteLength;
}

void const *TinyKtx2_ImageRawData(TinyKtx2_ContextHandle handle, uint32_t mipmaplevel) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return NULL;

	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return NULL;
	}

	if (mipmaplevel >= ctx->header.levelCount) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if (mipmaplevel >= TINYKTX2_MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if (ctx->mipmaps[mipmaplevel] != NULL)
		return ctx->mipmaps[mipmaplevel];

	TinyKtx_Level* lvl = &ctx->levels[mipmaplevel];
	if (lvl->byteLength == 0 || lvl->uncompressedByteLength == 0)
		return NULL;

	// allocate decompressed buffer
	ctx->mipmaps[mipmaplevel] = (uint8_t const*) ctx->callbacks.alloc(ctx->user, lvl->uncompressedByteLength);
	if (ctx->mipmaps[mipmaplevel] == NULL)
		return NULL;

	// handle no super compression first (save an buffer allocation)
	if(ctx->header.supercompressionScheme == TKTX2_SUPERCOMPRESSION_NONE) {
		if(lvl->uncompressedByteLength != lvl->byteLength) {
			ctx->callbacks.error(ctx->user, "mipmap image data has no super compression but compressed and uncompressed data sizes are different");
			ctx->callbacks.free(ctx->user, (void*)ctx->mipmaps[mipmaplevel]);
			return NULL;
		}
		ctx->callbacks.seek(ctx->user, lvl->byteOffset);
		ctx->callbacks.read(ctx->user, (void *) ctx->mipmaps[mipmaplevel], lvl->byteLength);
		return ctx->mipmaps[mipmaplevel];
	}

	// this data is super compressed, we need to see if the user provided a decompressor and if so use it

	TinyKtx_SuperDecompress decompressor = NULL;
	// see if the user provided the decompressor we need
	for(size_t i = 0; i < ctx->callbacks.numSuperDecompressors;++i) {
		if(ctx->callbacks.superDecompressors[i].superId == ctx->headerV2.supercompressionScheme) {
			decompressor = ctx->callbacks.superDecompressors[i].decompressor;
		}
	}
	if(decompressor == NULL) {
		ctx->callbacks.error(ctx->user, "user did not provide a decompressor for use with this type of super decompressor");
		ctx->callbacks.free(ctx->user, (void*)ctx->mipmaps[mipmaplevel]);
		return NULL;
	}

	// read the compressed data into its own buffer (free once decompression has occured)
	uint8_t const* compressedBuffer = (uint8_t const*)ctx->callbacks.alloc(ctx->user, lvl->byteLength);
	if(compressedBuffer == NULL) {
		ctx->callbacks.free(ctx->user, (void*)ctx->mipmaps[mipmaplevel]);
		return NULL;
	}
	ctx->callbacks.seek(ctx->user, lvl->byteOffset);
	ctx->callbacks.read(ctx->user, (void *) compressedBuffer, lvl->byteLength);
	bool okay = decompressor(ctx->user, ctx->sgdData, compressedBuffer, lvl->byteLength, ctx->mipmaps[mipmaplevel], lvl->uncompressedByteLength);

	if(!okay) {
		ctx->callbacks.error(ctx->user, "user decompressor failed");
		ctx->callbacks.free(ctx->user, (void *) compressedBuffer);
		ctx->callbacks.free(ctx->user, (void *) ctx->mipmaps[mipmaplevel]);
		return NULL;
	}

	ctx->callbacks.free(ctx->user, (void *) compressedBuffer);
	return ctx->mipmaps[mipmaplevel];

}

TinyKtx_Format TinyKtx2_GetFormat(TinyKtx2_ContextHandle handle) {
	TinyKtx2_Context *ctx = (TinyKtx2_Context *) handle;
	if (ctx == NULL)
		return TKTX_UNDEFINED;

	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return TKTX_UNDEFINED;
	}
	// TODO handle DFD only described formats (VK_FORMAT_UNDEFINED)
	return (TinyKtx_Format)ctx->header.vkFormat;
}
static uint32_t TinyKtx2_MipMapReduce(uint32_t value, uint32_t mipmaplevel) {

	// handle 0 being passed in
	if(value <= 1) return 1;

	// there are better ways of doing this (log2 etc.) but this doesn't require any
	// dependecies and isn't used enough to matter imho
	for (uint32_t i = 0u; i < mipmaplevel;++i) {
		if(value <= 1) return 1;
		value = value / 2;
	}
	return value;
}



bool TinyKtx2_WriteImage(TinyKtx2_WriteCallbacks const *callbacks,
												void *user,
												uint32_t width,
												uint32_t height,
												uint32_t depth,
												uint32_t slices,
												uint32_t mipmaplevels,
												TinyKtx_Format format,
												bool cubemap,
												uint32_t const *mipmapsizes,
												void const **mipmaps) {
	ASSERT(false);
}
#endif

#ifdef __cplusplus
};
#endif

#endif // end header
/*
MIT License

Copyright (c) 2019 DeanoC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
