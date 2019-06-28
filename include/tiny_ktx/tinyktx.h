#pragma once
#ifndef TINY_KTX_TINYKTX_H
#define TINY_KTX_TINYKTX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// GL types
#define TINYKTX_GL_TYPE_COMPRESSED                      0x0
#define TINYKTX_GL_TYPE_BYTE                            0x1400
#define TINYKTX_GL_TYPE_UNSIGNED_BYTE                    0x1401
#define TINYKTX_GL_TYPE_SHORT                            0x1402
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT                  0x1403
#define TINYKTX_GL_TYPE_INT                              0x1404
#define TINYKTX_GL_TYPE_UNSIGNED_INT                    0x1405
#define TINYKTX_GL_TYPE_FLOAT                            0x1406
#define TINYKTX_GL_TYPE_DOUBLE                          0x140A
#define TINYKTX_GL_TYPE_HALF_FLOAT                      0x140B
#define TINYKTX_GL_TYPE_UNSIGNED_BYTE_3_3_2              0x8032
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_4_4_4_4          0x8033
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_5_5_5_1          0x8034
#define TINYKTX_GL_TYPE_UNSIGNED_INT_8_8_8_8            0x8035
#define TINYKTX_GL_TYPE_UNSIGNED_INT_10_10_10_2          0x8036
#define TINYKTX_GL_TYPE_UNSIGNED_BYTE_2_3_3_REV          0x8362
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_5_6_5            0x8363
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_5_6_5_REV        0x8364
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_4_4_4_4_REV      0x8365
#define TINYKTX_GL_TYPE_UNSIGNED_SHORT_1_5_5_5_REV      0x8366
#define TINYKTX_GL_TYPE_UNSIGNED_INT_8_8_8_8_REV        0x8367
#define TINYKTX_GL_TYPE_UNSIGNED_INT_2_10_10_10_REV      0x8368
#define TINYKTX_GL_TYPE_UNSIGNED_INT_24_8                0x84FA
#define TINYKTX_GL_TYPE_UNSIGNED_INT_5_9_9_9_REV        0x8C3E
#define TINYKTX_GL_TYPE_UNSIGNED_INT_10F_11F_11F_REV    0x8C3B
#define TINYKTX_GL_TYPE_FLOAT_32_UNSIGNED_INT_24_8_REV  0x8DAD

// formats
#define TINYKTX_GL_FORMAT_RED                              0x1903
#define TINYKTX_GL_FORMAT_GREEN                            0x1904
#define TINYKTX_GL_FORMAT_BLUE                            0x1905
#define TINYKTX_GL_FORMAT_ALPHA                            0x1906
#define TINYKTX_GL_FORMAT_RGB                              0x1907
#define TINYKTX_GL_FORMAT_RGBA                            0x1908
#define TINYKTX_GL_FORMAT_LUMINANCE                        0x1909
#define TINYKTX_GL_FORMAT_LUMINANCE_ALPHA                  0x190A
#define TINYKTX_GL_FORMAT_ABGR                            0x8000
#define TINYKTX_GL_FORMAT_INTENSITY                        0x8049
#define TINYKTX_GL_FORMAT_BGR                              0x80E0
#define TINYKTX_GL_FORMAT_BGRA                            0x80E1
#define TINYKTX_GL_FORMAT_RG                              0x8227
#define TINYKTX_GL_FORMAT_RG_INTEGER                      0x8228
#define TINYKTX_GL_FORMAT_SRGB                            0x8C40
#define TINYKTX_GL_FORMAT_SRGB_ALPHA                      0x8C42
#define TINYKTX_GL_FORMAT_SLUMINANCE_ALPHA                0x8C44
#define TINYKTX_GL_FORMAT_SLUMINANCE                      0x8C46
#define TINYKTX_GL_FORMAT_RED_INTEGER                      0x8D94
#define TINYKTX_GL_FORMAT_GREEN_INTEGER                    0x8D95
#define TINYKTX_GL_FORMAT_BLUE_INTEGER                    0x8D96
#define TINYKTX_GL_FORMAT_ALPHA_INTEGER                    0x8D97
#define TINYKTX_GL_FORMAT_RGB_INTEGER                      0x8D98
#define TINYKTX_GL_FORMAT_RGBA_INTEGER                    0x8D99
#define TINYKTX_GL_FORMAT_BGR_INTEGER                      0x8D9A
#define TINYKTX_GL_FORMAT_BGRA_INTEGER                    0x8D9B
#define TINYKTX_GL_FORMAT_RED_SNORM                        0x8F90
#define TINYKTX_GL_FORMAT_RG_SNORM                        0x8F91
#define TINYKTX_GL_FORMAT_RGB_SNORM                        0x8F92
#define TINYKTX_GL_FORMAT_RGBA_SNORM                      0x8F93

#define TINYKTX_GL_INTFORMAT_ALPHA4                          0x803B
#define TINYKTX_GL_INTFORMAT_ALPHA8                          0x803C
#define TINYKTX_GL_INTFORMAT_ALPHA12                          0x803D
#define TINYKTX_GL_INTFORMAT_ALPHA16                          0x803E
#define TINYKTX_GL_INTFORMAT_LUMINANCE4                      0x803F
#define TINYKTX_GL_INTFORMAT_LUMINANCE8                      0x8040
#define TINYKTX_GL_INTFORMAT_LUMINANCE12                      0x8041
#define TINYKTX_GL_INTFORMAT_LUMINANCE16                      0x8042
#define TINYKTX_GL_INTFORMAT_LUMINANCE4_ALPHA4                0x8043
#define TINYKTX_GL_INTFORMAT_LUMINANCE6_ALPHA2                0x8044
#define TINYKTX_GL_INTFORMAT_LUMINANCE8_ALPHA8                0x8045
#define TINYKTX_GL_INTFORMAT_LUMINANCE12_ALPHA4              0x8046
#define TINYKTX_GL_INTFORMAT_LUMINANCE12_ALPHA12              0x8047
#define TINYKTX_GL_INTFORMAT_LUMINANCE16_ALPHA16              0x8048
#define TINYKTX_GL_INTFORMAT_INTENSITY4                      0x804A
#define TINYKTX_GL_INTFORMAT_INTENSITY8                      0x804B
#define TINYKTX_GL_INTFORMAT_INTENSITY12                      0x804C
#define TINYKTX_GL_INTFORMAT_INTENSITY16                      0x804D
#define TINYKTX_GL_INTFORMAT_RGB2                            0x804E
#define TINYKTX_GL_INTFORMAT_RGB4                            0x804F
#define TINYKTX_GL_INTFORMAT_RGB5                            0x8050
#define TINYKTX_GL_INTFORMAT_RGB8                              0x8051
#define TINYKTX_GL_INTFORMAT_RGB10                            0x8052
#define TINYKTX_GL_INTFORMAT_RGB12                            0x8053
#define TINYKTX_GL_INTFORMAT_RGB16                            0x8054
#define TINYKTX_GL_INTFORMAT_RGBA2                            0x8055
#define TINYKTX_GL_INTFORMAT_RGBA4                            0x8056
#define TINYKTX_GL_INTFORMAT_RGB5_A1                          0x8057
#define TINYKTX_GL_INTFORMAT_RGBA8                            0x8058
#define TINYKTX_GL_INTFORMAT_RGB10_A2                        0x8059
#define TINYKTX_GL_INTFORMAT_RGBA12                          0x805A
#define TINYKTX_GL_INTFORMAT_RGBA16                          0x805B
#define TINYKTX_GL_INTFORMAT_R8                              0x8229
#define TINYKTX_GL_INTFORMAT_R16                              0x822A
#define TINYKTX_GL_INTFORMAT_RG8                              0x822B
#define TINYKTX_GL_INTFORMAT_RG16                            0x822C
#define TINYKTX_GL_INTFORMAT_R16F                            0x822D
#define TINYKTX_GL_INTFORMAT_R32F                            0x822E
#define TINYKTX_GL_INTFORMAT_RG16F                            0x822F
#define TINYKTX_GL_INTFORMAT_RG32F                            0x8230
#define TINYKTX_GL_INTFORMAT_R8I                              0x8231
#define TINYKTX_GL_INTFORMAT_R8UI                            0x8232
#define TINYKTX_GL_INTFORMAT_R16I                            0x8233
#define TINYKTX_GL_INTFORMAT_R16UI                            0x8234
#define TINYKTX_GL_INTFORMAT_R32I                            0x8235
#define TINYKTX_GL_INTFORMAT_R32UI                            0x8236
#define TINYKTX_GL_INTFORMAT_RG8I                            0x8237
#define TINYKTX_GL_INTFORMAT_RG8UI                            0x8238
#define TINYKTX_GL_INTFORMAT_RG16I                            0x8239
#define TINYKTX_GL_INTFORMAT_RG16UI                          0x823A
#define TINYKTX_GL_INTFORMAT_RG32I                            0x823B
#define TINYKTX_GL_INTFORMAT_RG32UI                          0x823C
#define TINYKTX_GL_INTFORMAT_RGBA32F                          0x8814
#define TINYKTX_GL_INTFORMAT_RGB32F                          0x8815
#define TINYKTX_GL_INTFORMAT_RGBA16F                          0x881A
#define TINYKTX_GL_INTFORMAT_RGB16F                          0x881B
#define TINYKTX_GL_INTFORMAT_R11F_G11F_B10F                  0x8C3A
#define TINYKTX_GL_INTFORMAT_UNSIGNED_INT_10F_11F_11F_REV      0x8C3B
#define TINYKTX_GL_INTFORMAT_RGB9_E5                          0x8C3D
#define TINYKTX_GL_INTFORMAT_SRGB8                            0x8C41
#define TINYKTX_GL_INTFORMAT_SRGB8_ALPHA8                      0x8C43
#define TINYKTX_GL_INTFORMAT_SLUMINANCE8_ALPHA8              0x8C45
#define TINYKTX_GL_INTFORMAT_SLUMINANCE8                      0x8C47
#define TINYKTX_GL_INTFORMAT_RGB565                          0x8D62
#define TINYKTX_GL_INTFORMAT_RGBA32UI                        0x8D70
#define TINYKTX_GL_INTFORMAT_RGB32UI                          0x8D71
#define TINYKTX_GL_INTFORMAT_RGBA16UI                        0x8D76
#define TINYKTX_GL_INTFORMAT_RGB16UI                          0x8D77
#define TINYKTX_GL_INTFORMAT_RGBA8UI                          0x8D7C
#define TINYKTX_GL_INTFORMAT_RGB8UI                          0x8D7D
#define TINYKTX_GL_INTFORMAT_RGBA32I                          0x8D82
#define TINYKTX_GL_INTFORMAT_RGB32I                          0x8D83
#define TINYKTX_GL_INTFORMAT_RGBA16I                          0x8D88
#define TINYKTX_GL_INTFORMAT_RGB16I                          0x8D89
#define TINYKTX_GL_INTFORMAT_RGBA8I                          0x8D8E
#define TINYKTX_GL_INTFORMAT_RGB8I                            0x8D8F
#define TINYKTX_GL_INTFORMAT_FLOAT_32_UNSIGNED_INT_24_8_REV  0x8DAD
#define TINYKTX_GL_INTFORMAT_R8_SNORM                        0x8F94
#define TINYKTX_GL_INTFORMAT_RG8_SNORM                        0x8F95
#define TINYKTX_GL_INTFORMAT_RGB8_SNORM                      0x8F96
#define TINYKTX_GL_INTFORMAT_RGBA8_SNORM                      0x8F97
#define TINYKTX_GL_INTFORMAT_R16_SNORM                        0x8F98
#define TINYKTX_GL_INTFORMAT_RG16_SNORM                      0x8F99
#define TINYKTX_GL_INTFORMAT_RGB16_SNORM                      0x8F9A
#define TINYKTX_GL_INTFORMAT_RGBA16_SNORM                    0x8F9B
#define TINYKTX_GL_INTFORMAT_ALPHA8_SNORM                    0x9014
#define TINYKTX_GL_INTFORMAT_LUMINANCE8_SNORM                0x9015
#define TINYKTX_GL_INTFORMAT_LUMINANCE8_ALPHA8_SNORM          0x9016
#define TINYKTX_GL_INTFORMAT_INTENSITY8_SNORM                0x9017
#define TINYKTX_GL_INTFORMAT_ALPHA16_SNORM                    0x9018
#define TINYKTX_GL_INTFORMAT_LUMINANCE16_SNORM                0x9019
#define TINYKTX_GL_INTFORMAT_LUMINANCE16_ALPHA16_SNORM        0x901A
#define TINYKTX_GL_INTFORMAT_INTENSITY16_SNORM                0x901B

#define TINYKTX_GL_PALETTE4_RGB8_OES              0x8B90
#define TINYKTX_GL_PALETTE4_RGBA8_OES             0x8B91
#define TINYKTX_GL_PALETTE4_R5_G6_B5_OES          0x8B92
#define TINYKTX_GL_PALETTE4_RGBA4_OES             0x8B93
#define TINYKTX_GL_PALETTE4_RGB5_A1_OES           0x8B94
#define TINYKTX_GL_PALETTE8_RGB8_OES              0x8B95
#define TINYKTX_GL_PALETTE8_RGBA8_OES             0x8B96
#define TINYKTX_GL_PALETTE8_R5_G6_B5_OES          0x8B97
#define TINYKTX_GL_PALETTE8_RGBA4_OES             0x8B98
#define TINYKTX_GL_PALETTE8_RGB5_A1_OES           0x8B99

// compressed formats

#define TINYKTX_GL_COMPRESSED_RGB_S3TC_DXT1                  	0x83F0
#define TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT1                  0x83F1
#define TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT3                  0x83F2
#define TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT5                  0x83F3
#define TINYKTX_GL_COMPRESSED_3DC_X_AMD                       0x87F9
#define TINYKTX_GL_COMPRESSED_3DC_XY_AMD                      0x87FA
#define TINYKTX_GL_COMPRESSED_ATC_RGBA_INTERPOLATED_ALPHA    	0x87EE
#define TINYKTX_GL_COMPRESSED_SRGB_PVRTC_2BPPV1               0x8A54
#define TINYKTX_GL_COMPRESSED_SRGB_PVRTC_4BPPV1               0x8A55
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1         0x8A56
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1         0x8A57
#define TINYKTX_GL_COMPRESSED_RGB_PVRTC_4BPPV1                0x8C00
#define TINYKTX_GL_COMPRESSED_RGB_PVRTC_2BPPV1                0x8C01
#define TINYKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV1               0x8C02
#define TINYKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV1               0x8C03
#define TINYKTX_GL_COMPRESSED_SRGB_S3TC_DXT1                  0x8C4C
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1            0x8C4D
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3            0x8C4E
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5            0x8C4F
#define TINYKTX_GL_COMPRESSED_LUMINANCE_LATC1                	0x8C70
#define TINYKTX_GL_COMPRESSED_SIGNED_LUMINANCE_LATC1          0x8C71
#define TINYKTX_GL_COMPRESSED_LUMINANCE_ALPHA_LATC2           0x8C72
#define TINYKTX_GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2    0x8C73
#define TINYKTX_GL_COMPRESSED_ATC_RGB                         0x8C92
#define TINYKTX_GL_COMPRESSED_ATC_RGBA_EXPLICIT_ALPHA         0x8C93
#define TINYKTX_GL_COMPRESSED_RED_RGTC1                       0x8DBB
#define TINYKTX_GL_COMPRESSED_SIGNED_RED_RGTC1                0x8DBC
#define TINYKTX_GL_COMPRESSED_RED_GREEN_RGTC2                	0x8DBD
#define TINYKTX_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2          0x8DBE
#define TINYKTX_GL_COMPRESSED_ETC1_RGB8_OES                   0x8D64
#define TINYKTX_GL_COMPRESSED_RGBA_BPTC_UNORM                	0x8E8C
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM          	0x8E8D
#define TINYKTX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT          	0x8E8E
#define TINYKTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT        	0x8E8F
#define TINYKTX_GL_COMPRESSED_R11_EAC                        	0x9270
#define TINYKTX_GL_COMPRESSED_SIGNED_R11_EAC                  0x9271
#define TINYKTX_GL_COMPRESSED_RG11_EAC                        0x9272
#define TINYKTX_GL_COMPRESSED_SIGNED_RG11_EAC                	0x9273
#define TINYKTX_GL_COMPRESSED_RGB8_ETC2                      	0x9274
#define TINYKTX_GL_COMPRESSED_SRGB8_ETC2                      0x9275
#define TINYKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2   0x9276
#define TINYKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2  0x9277
#define TINYKTX_GL_COMPRESSED_RGBA8_ETC2_EAC                  0x9278
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC           0x9279
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2         0x93F0
#define TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2         0x93F1
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_4x4		   							0x93B0
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_5x4		              	0x93B1
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_5x5		              	0x93B2
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_6x5		              	0x93B3
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_6x6		              	0x93B4
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x5		              	0x93B5
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x6		              	0x93B6
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x8		              	0x93B7
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x5		              0x93B8
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x6		              0x93B9
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x8		              0x93BA
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x10	            		0x93BB
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_12x10	            		0x93BC
#define TINYKTX_GL_COMPRESSED_RGBA_ASTC_12x12	            		0x93BD
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4			      0x93D0
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4			      0x93D1
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5			      0x93D2
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5						0x93D3
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6						0x93D4
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5						0x93D5
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6						0x93D6
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8						0x93D7
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5					0x93D8
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6					0x93D9
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8					0x93DA
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10					0x93DB
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10     		0x93DC
#define TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12     		0x93DD

#define TINYKTX_MAX_MIPMAPLEVELS 16

#ifdef __cplusplus
extern "C" {
#endif
// ktx is based on GL (slightly confusing imho) texture format system
// there is format, internal format, type etc.

// we try and expose a more dx12/vulkan/metal style of format
// but obviously still need to GL data so bare with me.
// a TinyKTX_Format is the equivilent to GL/KTX Format and Type
// the API doesn't expose the actual values (which come from GL itself)
// but provide an API call to crack them back into the actual GL values).

// this may look very similar to Vulkan/Dx12 format (its not but related)
// these are even more closely related to my gfx_imageformat library...

typedef enum TinyKtx_Format {
	TKTX_UNDEFINED = 0,
	TKTX_R4G4_UNORM_PACK8,
	TKTX_R4G4B4A4_UNORM_PACK16,
	TKTX_B4G4R4A4_UNORM_PACK16,
	TKTX_R5G6B5_UNORM_PACK16,
	TKTX_B5G6R5_UNORM_PACK16,
	TKTX_R5G5B5A1_UNORM_PACK16,
	TKTX_B5G5R5A1_UNORM_PACK16,
	TKTX_A1R5G5B5_UNORM_PACK16,

	TKTX_R8_UNORM,
	TKTX_R8_SNORM,
	TKTX_R8_UINT,
	TKTX_R8_SINT,
	TKTX_R8_SRGB,

	TKTX_R8G8_UNORM,
	TKTX_R8G8_SNORM,
	TKTX_R8G8_UINT,
	TKTX_R8G8_SINT,
	TKTX_R8G8_SRGB,

	TKTX_R8G8B8_UNORM,
	TKTX_R8G8B8_SNORM,
	TKTX_R8G8B8_UINT,
	TKTX_R8G8B8_SINT,
	TKTX_R8G8B8_SRGB,
	TKTX_B8G8R8_UNORM,
	TKTX_B8G8R8_SNORM,
	TKTX_B8G8R8_UINT,
	TKTX_B8G8R8_SINT,
	TKTX_B8G8R8_SRGB,

	TKTX_R8G8B8A8_UNORM,
	TKTX_R8G8B8A8_SNORM,
	TKTX_R8G8B8A8_UINT,
	TKTX_R8G8B8A8_SINT,
	TKTX_R8G8B8A8_SRGB,
	TKTX_B8G8R8A8_UNORM,
	TKTX_B8G8R8A8_SNORM,
	TKTX_B8G8R8A8_UINT,
	TKTX_B8G8R8A8_SINT,
	TKTX_B8G8R8A8_SRGB,

	TKTX_A8B8G8R8_UNORM_PACK32,
	TKTX_A8B8G8R8_SNORM_PACK32,
	TKTX_A8B8G8R8_UINT_PACK32,
	TKTX_A8B8G8R8_SINT_PACK32,
	TKTX_A8B8G8R8_SRGB_PACK32,

	TKTX_E5B9G9R9_UFLOAT_PACK32,
	TKTX_A2R10G10B10_UNORM_PACK32,
	TKTX_A2R10G10B10_UINT_PACK32,
	TKTX_A2B10G10R10_UNORM_PACK32,
	TKTX_A2B10G10R10_UINT_PACK32,
	TKTX_B10G11R11_UFLOAT_PACK32,

	TKTX_R16_UNORM,
	TKTX_R16_SNORM,
	TKTX_R16_UINT,
	TKTX_R16_SINT,
	TKTX_R16_SFLOAT,
	TKTX_R16G16_UNORM,
	TKTX_R16G16_SNORM,
	TKTX_R16G16_UINT,
	TKTX_R16G16_SINT,
	TKTX_R16G16_SFLOAT,
	TKTX_R16G16B16_UNORM,
	TKTX_R16G16B16_SNORM,
	TKTX_R16G16B16_UINT,
	TKTX_R16G16B16_SINT,
	TKTX_R16G16B16_SFLOAT,
	TKTX_R16G16B16A16_UNORM,
	TKTX_R16G16B16A16_SNORM,
	TKTX_R16G16B16A16_UINT,
	TKTX_R16G16B16A16_SINT,
	TKTX_R16G16B16A16_SFLOAT,
	TKTX_R32_UINT,
	TKTX_R32_SINT,
	TKTX_R32_SFLOAT,
	TKTX_R32G32_UINT,
	TKTX_R32G32_SINT,
	TKTX_R32G32_SFLOAT,
	TKTX_R32G32B32_UINT,
	TKTX_R32G32B32_SINT,
	TKTX_R32G32B32_SFLOAT,
	TKTX_R32G32B32A32_UINT,
	TKTX_R32G32B32A32_SINT,
	TKTX_R32G32B32A32_SFLOAT,

	TKTX_BC1_RGB_UNORM_BLOCK,
	TKTX_BC1_RGB_SRGB_BLOCK,
	TKTX_BC1_RGBA_UNORM_BLOCK,
	TKTX_BC1_RGBA_SRGB_BLOCK,
	TKTX_BC2_UNORM_BLOCK,
	TKTX_BC2_SRGB_BLOCK,
	TKTX_BC3_UNORM_BLOCK,
	TKTX_BC3_SRGB_BLOCK,
	TKTX_BC4_UNORM_BLOCK,
	TKTX_BC4_SNORM_BLOCK,
	TKTX_BC5_UNORM_BLOCK,
	TKTX_BC5_SNORM_BLOCK,
	TKTX_BC6H_UFLOAT_BLOCK,
	TKTX_BC6H_SFLOAT_BLOCK,
	TKTX_BC7_UNORM_BLOCK,
	TKTX_BC7_SRGB_BLOCK,

	TKTX_ETC2_R8G8B8_UNORM_BLOCK,
	TKTX_ETC2_R8G8B8A1_UNORM_BLOCK,
	TKTX_ETC2_R8G8B8A8_UNORM_BLOCK,
	TKTX_ETC2_R8G8B8_SRGB_BLOCK,
	TKTX_ETC2_R8G8B8A1_SRGB_BLOCK,
	TKTX_ETC2_R8G8B8A8_SRGB_BLOCK,
	TKTX_EAC_R11_UNORM_BLOCK,
	TKTX_EAC_R11G11_UNORM_BLOCK,
	TKTX_EAC_R11_SNORM_BLOCK,
	TKTX_EAC_R11G11_SNORM_BLOCK,

	TKTX_PVR_2BPP_BLOCK,
	TKTX_PVR_2BPPA_BLOCK,
	TKTX_PVR_4BPP_BLOCK,
	TKTX_PVR_4BPPA_BLOCK,
	TKTX_PVR_2BPP_SRGB_BLOCK,
	TKTX_PVR_2BPPA_SRGB_BLOCK,
	TKTX_PVR_4BPP_SRGB_BLOCK,
	TKTX_PVR_4BPPA_SRGB_BLOCK,

	TKTX_ASTC_4x4_UNORM_BLOCK,
	TKTX_ASTC_4x4_SRGB_BLOCK,
	TKTX_ASTC_5x4_UNORM_BLOCK,
	TKTX_ASTC_5x4_SRGB_BLOCK,
	TKTX_ASTC_5x5_UNORM_BLOCK,
	TKTX_ASTC_5x5_SRGB_BLOCK,
	TKTX_ASTC_6x5_UNORM_BLOCK,
	TKTX_ASTC_6x5_SRGB_BLOCK,
	TKTX_ASTC_6x6_UNORM_BLOCK,
	TKTX_ASTC_6x6_SRGB_BLOCK,
	TKTX_ASTC_8x5_UNORM_BLOCK,
	TKTX_ASTC_8x5_SRGB_BLOCK,
	TKTX_ASTC_8x6_UNORM_BLOCK,
	TKTX_ASTC_8x6_SRGB_BLOCK,
	TKTX_ASTC_8x8_UNORM_BLOCK,
	TKTX_ASTC_8x8_SRGB_BLOCK,
	TKTX_ASTC_10x5_UNORM_BLOCK,
	TKTX_ASTC_10x5_SRGB_BLOCK,
	TKTX_ASTC_10x6_UNORM_BLOCK,
	TKTX_ASTC_10x6_SRGB_BLOCK,
	TKTX_ASTC_10x8_UNORM_BLOCK,
	TKTX_ASTC_10x8_SRGB_BLOCK,
	TKTX_ASTC_10x10_UNORM_BLOCK,
	TKTX_ASTC_10x10_SRGB_BLOCK,
	TKTX_ASTC_12x10_UNORM_BLOCK,
	TKTX_ASTC_12x10_SRGB_BLOCK,
	TKTX_ASTC_12x12_UNORM_BLOCK,
	TKTX_ASTC_12x12_SRGB_BLOCK,

} TinyKtx_Format;

typedef struct TinyKtx_Context *TinyKtx_ContextHandle;

typedef void *(*TinyKtx_AllocFunc)(void *user, size_t size);
typedef void (*TinyKtx_FreeFunc)(void *user, void *memory);
typedef size_t (*TinyKtx_ReadFunc)(void *user, void *buffer, size_t byteCount);
typedef bool (*TinyKtx_SeekFunc)(void *user, int64_t offset);
typedef int64_t (*TinyKtx_TellFunc)(void *user);
typedef void (*TinyKtx_ErrorFunc)(void *user, char const *msg);

typedef struct TinyKtx_Callbacks {
	TinyKtx_ErrorFunc error;
	TinyKtx_AllocFunc alloc;
	TinyKtx_FreeFunc free;
	TinyKtx_ReadFunc read;
	TinyKtx_SeekFunc seek;
	TinyKtx_TellFunc tell;
} TinyKtx_Callbacks;

TinyKtx_ContextHandle TinyKtx_CreateContext(TinyKtx_Callbacks const *callbacks, void *user);
void TinyKtx_DestroyContext(TinyKtx_ContextHandle handle);

// begin should be used if KTX header isn't at offset 0 (pack files etc.)
void TinyKtx_BeginRead(TinyKtx_ContextHandle handle);
// reset like you reuse the context for another file (saves an alloc/free cycle)
void TinyKtx_Reset(TinyKtx_ContextHandle handle);

bool TinyKtx_ReadHeader(TinyKtx_ContextHandle handle);

// this is slow linear search. TODO add iterator style reading of key value pairs
bool TinyKtx_GetValue(TinyKtx_ContextHandle handle, char const *key, void const **value);

bool TinyKtx_Is1D(TinyKtx_ContextHandle handle);
bool TinyKtx_Is2D(TinyKtx_ContextHandle handle);
bool TinyKtx_Is3D(TinyKtx_ContextHandle handle);
bool TinyKtx_IsCubemap(TinyKtx_ContextHandle handle);
bool TinyKtx_IsArray(TinyKtx_ContextHandle handle);

bool TinyKtx_Dimensions(TinyKtx_ContextHandle handle, uint32_t* width, uint32_t* height, uint32_t* depth, uint32_t* slices);
uint32_t TinyKtx_Width(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_Height(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_Depth(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_ArraySlices(TinyKtx_ContextHandle handle);
TinyKtx_Format TinyKtx_GetFormat(TinyKtx_ContextHandle handle);

bool TinyKtx_GetFormatGL(TinyKtx_ContextHandle handle, uint32_t *glformat, uint32_t *gltype, uint32_t *glinternalformat, uint32_t* typesize, uint32_t* glbaseinternalformat);

bool TinyKtx_CrackFormatToGL(TinyKtx_Format format, uint32_t *glformat, uint32_t *gltype, uint32_t *glinternalformat, uint32_t* typesize);

bool TinyKtx_NeedsGenerationOfMipmaps(TinyKtx_ContextHandle handle);
bool TinyKtx_NeedsEndianCorrecting(TinyKtx_ContextHandle handle);

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle);
uint32_t TinyKtx_ImageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);

// don't data return by ImageRawData is owned by the context. Don't free it!
void const *TinyKtx_ImageRawData(TinyKtx_ContextHandle handle, uint32_t mipmaplevel);

typedef void (*TinyKtx_WriteFunc)(void *user, void const *buffer, size_t byteCount);

typedef struct TinyKtx_WriteCallbacks {
	TinyKtx_ErrorFunc error;
	TinyKtx_AllocFunc alloc;
	TinyKtx_FreeFunc free;
	TinyKtx_WriteFunc write;
} TinyKtx_WriteCallbacks;

bool TinyKtx_WriteImage(TinyKtx_WriteCallbacks const *callbacks,
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

bool TinyKtx_WriteImageGL(TinyKtx_WriteCallbacks const *callbacks,
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
#ifdef __cplusplus
};
#endif

#endif // end header