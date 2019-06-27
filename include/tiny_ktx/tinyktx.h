#pragma once
#ifndef TINY_KTX_TINYKTX_H
#define TINY_KTX_TINYKTX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TINYKTX_GL_ALPHA                        0x1906
#define TINYKTX_GL_LUMINANCE                    0x1909
#define TINYKTX_GL_LUMINANCE_ALPHA              0x190A
#define TINYKTX_GL_INTENSITY                    0x8049

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

#define TINYKTX_GL_TEXTURE_1D                   0x0DE0
#define TINYKTX_GL_TEXTURE_3D                   0x806F
#define TINYKTX_GL_TEXTURE_CUBE_MAP             0x8513
#define TINYKTX_GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
#define TINYKTX_GL_TEXTURE_CUBE_MAP_ARRAY       0x9009
#define TINYKTX_GL_TEXTURE_1D_ARRAY_EXT         0x8C18
#define TINYKTX_GL_TEXTURE_2D_ARRAY_EXT         0x8C1A
#define TINYKTX_GL_GENERATE_MIPMAP              0x8191

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
#define TINYKTX_GL_INT 0x1404
#define TINYKTX_GL_UNSIGNED_INT 0x1405
typedef unsigned short TINYKTX_GLhalf;

#define TINYKTX_GL_HALF_FLOAT                   0x140B
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
#define TINYKTX_GL_DEPTH_STENCIL                0x84F9
#define TINYKTX_GL_UNSIGNED_INT_24_8            0x84FA
#define TINYKTX_GL_UNSIGNED_INT_5_9_9_9_REV     0x8C3E
#define TINYKTX_GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define TINYKTX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV   0x8DAD
#define TINYKTX_GL_ETC1_RGB8_OES                0x8D64
#define TINYKTX_GL_COMPRESSED_R11_EAC                            0x9270
#define TINYKTX_GL_COMPRESSED_SIGNED_R11_EAC                     0x9271
#define TINYKTX_GL_COMPRESSED_RG11_EAC                           0x9272
#define TINYKTX_GL_COMPRESSED_SIGNED_RG11_EAC                    0x9273
#define TINYKTX_GL_COMPRESSED_RGB8_ETC2                          0x9274
#define TINYKTX_GL_COMPRESSED_SRGB8_ETC2                         0x9275
#define TINYKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2      0x9276
#define TINYKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2     0x9277
#define TINYKTX_GL_COMPRESSED_RGBA8_ETC2_EAC                     0x9278
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC              0x9279
#define TINYKTX_GL_R16_SNORM                    0x8F98
#define TINYKTX_GL_RG16_SNORM                   0x8F99
#define TINYKTX_GL_RED                          0x1903
#define TINYKTX_GL_GREEN                        0x1904
#define TINYKTX_GL_BLUE                         0x1905
#define TINYKTX_GL_RG                           0x8227
#define TINYKTX_GL_RG_INTEGER                   0x8228
#define TINYKTX_GL_R16                          0x822A
#define TINYKTX_GL_RG16                         0x822C
#define TINYKTX_GL_RGB8                         0x8051
#define TINYKTX_GL_RGBA8                        0x8058
#define TINYKTX_GL_SRGB8                        0x8C41
#define TINYKTX_GL_SRGB8_ALPHA8                 0x8C43
#define TINYKTX_GL_MAJOR_VERSION                0x821B
#define TINYKTX_GL_MINOR_VERSION                0x821C
#define TINYKTX_GL_CONTEXT_PROFILE_MASK              0x9126
#define TINYKTX_GL_CONTEXT_CORE_PROFILE_BIT          0x00000001
#define TINYKTX_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define TINYKTX_GL_NUM_EXTENSIONS              0x821D

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

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle);

bool TinyKtx_NeedsGenerationOfMipmaps(TinyKtx_ContextHandle handle);
bool TinyKtx_NeedsEndianCorrecting(TinyKtx_ContextHandle handle);

uint32_t TinyKtx_ImageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);

// don't data return by ImageRawData is owned by the context. Don't free it!
void const* TinyKtx_ImageRawData(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);

#ifdef __cplusplus
};
#endif

#endif // end header