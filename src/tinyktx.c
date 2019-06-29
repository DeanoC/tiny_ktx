#include "tiny_ktx/tinyktx.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TinyKtx_Header {
	uint8_t identifier[12];
	uint32_t endianness;
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;

} TinyKtx_Header;

typedef struct TinyKtx_KeyValuePair {
	uint32_t size;
} TinyKtx_KeyValuePair; // followed by at least size bytes (aligned to 4)


typedef struct TinyKtx_Context {
	TinyKtx_Callbacks callbacks;
	void *user;
	uint64_t headerPos;
	uint64_t firstImagePos;

	TinyKtx_Header header;
	TinyKtx_KeyValuePair const *keyData;
	bool headerValid;
	bool sameEndian;

	uint32_t mipMapSizes[TINYKTX_MAX_MIPMAPLEVELS];
	uint8_t const *mipmaps[TINYKTX_MAX_MIPMAPLEVELS];

} TinyKtx_Context;

static uint8_t fileIdentifier[12] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};
static void DefaultErrorFunc(void *user, char const *msg) {}

TinyKtx_ContextHandle TinyKtx_CreateContext(TinyKtx_Callbacks const *callbacks, void *user) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) callbacks->alloc(user, sizeof(TinyKtx_Context));
	if (ctx == NULL)
		return NULL;

	memset(ctx, 0, sizeof(TinyKtx_Context));
	memcpy(&ctx->callbacks, callbacks, sizeof(TinyKtx_Callbacks));
	ctx->user = user;
	if (ctx->callbacks.error == NULL) {
		ctx->callbacks.error = &DefaultErrorFunc;
	}

	if (ctx->callbacks.read == NULL) {
		DefaultErrorFunc(user, "TinyKtx must have read callback");
		return NULL;
	}
	if (ctx->callbacks.alloc == NULL) {
		DefaultErrorFunc(user, "TinyKtx must have alloc callback");
		return NULL;
	}
	if (ctx->callbacks.free == NULL) {
		DefaultErrorFunc(user, "TinyKtx must have free callback");
		return NULL;
	}
	if (ctx->callbacks.seek == NULL) {
		DefaultErrorFunc(user, "TinyKtx must have seek callback");
		return NULL;
	}
	if (ctx->callbacks.tell == NULL) {
		DefaultErrorFunc(user, "TinyKtx must have tell callback");
		return NULL;
	}

	TinyKtx_Reset(ctx);

	return ctx;
}

void TinyKtx_DestroyContext(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return;
	ctx->callbacks.free(ctx->user, ctx);
}

void TinyKtx_BeginRead(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return;

	ctx->headerPos = ctx->callbacks.tell(ctx->user);
}

void TinyKtx_Reset(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return;

	// backup user provided callbacks and data
	TinyKtx_Callbacks callbacks;
	memcpy(&callbacks, &ctx->callbacks, sizeof(TinyKtx_Callbacks));
	void *user = ctx->user;

	// free memory of sub data
	if (ctx->keyData != NULL) {
		ctx->callbacks.free(ctx->user, (void *) ctx->keyData);
	}
	for (int i = 0; i < TINYKTX_MAX_MIPMAPLEVELS; ++i) {
		if (ctx->mipmaps[i] != NULL) {
			ctx->callbacks.free(ctx->user, (void *) ctx->mipmaps[i]);
		}
	}

	// reset to default state
	memset(ctx, 0, sizeof(TinyKtx_Context));
	memcpy(&ctx->callbacks, &callbacks, sizeof(TinyKtx_Callbacks));
	ctx->user = user;

}

bool TinyKtx_ReadHeader(TinyKtx_ContextHandle handle) {

	static uint32_t const sameEndianDecider = 0x04030201;
	static uint32_t const differentEndianDecider = 0x01020304;

	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;

	ctx->callbacks.seek(ctx->user, ctx->headerPos);
	ctx->callbacks.read(ctx->user, &ctx->header, sizeof(TinyKtx_Header));

	if (memcmp(&ctx->header.identifier, fileIdentifier, 12) != 0) {
		ctx->callbacks.error(ctx->user, "Not a KTX file or corrupted as identified isn't valid");
		return false;
	}

	if (ctx->header.endianness == sameEndianDecider) {
		ctx->sameEndian = true;
	} else if (ctx->header.endianness == differentEndianDecider) {
		ctx->sameEndian = false;
	} else {
		// corrupt or mid endian?
		ctx->callbacks.error(ctx->user, "Endian Error");
		return false;
	}

	if (ctx->header.numberOfFaces != 1 && ctx->header.numberOfFaces != 6) {
		ctx->callbacks.error(ctx->user, "no. of Faces must be 1 or 6");
		return false;
	}

	ctx->keyData = (TinyKtx_KeyValuePair const *) ctx->callbacks.alloc(ctx->user, ctx->header.bytesOfKeyValueData);
	ctx->callbacks.read(ctx->user, (void *) ctx->keyData, ctx->header.bytesOfKeyValueData);

	ctx->firstImagePos = ctx->callbacks.tell(ctx->user);

	ctx->headerValid = true;
	return true;
}
bool TinyKtx_GetValue(TinyKtx_ContextHandle handle, char const *key, void const **value) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
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

	TinyKtx_KeyValuePair const *curKey = ctx->keyData;
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

bool TinyKtx_Is1D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight == 1 && ctx->header.pixelDepth == 1);
}
bool TinyKtx_Is2D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight != 1 && ctx->header.pixelDepth == 1);
}
bool TinyKtx_Is3D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight != 1 && ctx->header.pixelDepth != 1);
}

bool TinyKtx_IsCubemap(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfFaces == 6);
}
bool TinyKtx_IsArray(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfArrayElements > 1);
}
bool TinyKtx_Dimensions(TinyKtx_ContextHandle handle,
												uint32_t *width,
												uint32_t *height,
												uint32_t *depth,
												uint32_t *slices) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
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

uint32_t TinyKtx_Width(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.pixelWidth;
}

uint32_t TinyKtx_Height(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.pixelHeight;
}

uint32_t TinyKtx_Depth(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.pixelDepth;
}

uint32_t TinyKtx_ArraySlices(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.numberOfArrayElements;
}

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}

	return ctx->header.numberOfMipmapLevels ? ctx->header.numberOfMipmapLevels : 1;
}

bool TinyKtx_NeedsGenerationOfMipmaps(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.numberOfMipmapLevels == 0;
}

bool TinyKtx_NeedsEndianCorrecting(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->sameEndian == false;
}

#define FT(fmt, type, intfmt, size) *glformat = TINYKTX_GL_FORMAT_##fmt; \
                                    *gltype = TINYKTX_GL_TYPE_##type; \
                                    *glinternalformat = TINYKTX_GL_INTFORMAT_##intfmt; \
                                    *typesize = size; \
                                    return true;
#define FTC(fmt, intfmt) *glformat = TINYKTX_GL_FORMAT_##fmt; \
                                    *gltype = TINYKTX_GL_TYPE_COMPRESSED; \
                                    *glinternalformat = TINYKTX_GL_COMPRESSED_##intfmt; \
                                    *typesize = 1; \
                                    return true;

bool TinyKtx_CrackFormatToGL(TinyKtx_Format format,
														 uint32_t *glformat,
														 uint32_t *gltype,
														 uint32_t *glinternalformat,
														 uint32_t *typesize) {
	switch (format) {
	case TKTX_R4G4_UNORM_PACK8: break;
	case TKTX_R4G4B4A4_UNORM_PACK16: FT(RGBA, UNSIGNED_SHORT_4_4_4_4, RGB4, 1)
	case TKTX_B4G4R4A4_UNORM_PACK16: FT(BGRA, UNSIGNED_SHORT_4_4_4_4_REV, RGB4, 1)
	case TKTX_R5G6B5_UNORM_PACK16: FT(RGB, UNSIGNED_SHORT_5_6_5, RGB565, 1)
	case TKTX_B5G6R5_UNORM_PACK16: FT(BGR, UNSIGNED_SHORT_5_6_5_REV, RGB565, 1)
	case TKTX_R5G5B5A1_UNORM_PACK16: FT(RGBA, UNSIGNED_SHORT_5_5_5_1, RGB5_A1, 1)
	case TKTX_A1R5G5B5_UNORM_PACK16: FT(BGRA, UNSIGNED_SHORT_1_5_5_5_REV, RGB5_A1, 1)
	case TKTX_B5G5R5A1_UNORM_PACK16: FT(BGRA, UNSIGNED_SHORT_5_5_5_1, RGB5_A1, 1)

	case TKTX_A2R10G10B10_UNORM_PACK32: FT(BGRA, UNSIGNED_INT_2_10_10_10_REV, RGB10_A2, 1)
	case TKTX_A2R10G10B10_UINT_PACK32: break; //FT(BGRA_INTEGER, UNSIGNED_INT_2_10_10_10_REV, RGB10_A2UI)
	case TKTX_A2B10G10R10_UNORM_PACK32: break; //FT(ABGR, UNSIGNED_INT_2_10_10_10_REV)
	case TKTX_A2B10G10R10_UINT_PACK32: break; //FT(ABGR_INTEGER, UNSIGNED_INT_2_10_10_10_REV)

	case TKTX_R8_UNORM: FT(RED, UNSIGNED_BYTE, R8, 1)
	case TKTX_R8_SNORM: FT(RED, BYTE, R8_SNORM, 1)
	case TKTX_R8_UINT: FT(RED_INTEGER, UNSIGNED_BYTE, R8UI, 1)
	case TKTX_R8_SINT: FT(RED_INTEGER, BYTE, R8I, 1)
	case TKTX_R8_SRGB: break; //FT(SRED, SR8, 1)

	case TKTX_R8G8_UNORM: FT(RG, UNSIGNED_BYTE, RG8, 1)
	case TKTX_R8G8_SNORM: FT(RG, BYTE, RG8_SNORM, 1)
	case TKTX_R8G8_UINT: FT(RG_INTEGER, UNSIGNED_BYTE, RG8UI, 1)
	case TKTX_R8G8_SINT: FT(RG_INTEGER, BYTE, RG8I, 1)
	case TKTX_R8G8_SRGB: break; //FT(SRG, SRG8)

	case TKTX_R8G8B8_UNORM: FT(RGB, UNSIGNED_BYTE, RGB8, 1)
	case TKTX_R8G8B8_SNORM: FT(RGB, BYTE, RGB8_SNORM, 1)
	case TKTX_R8G8B8_UINT: FT(RGB_INTEGER, UNSIGNED_BYTE, RGB8UI, 1)
	case TKTX_R8G8B8_SINT: FT(RGB_INTEGER, BYTE, RGB8I, 1)
	case TKTX_R8G8B8_SRGB: FT(SRGB, UNSIGNED_BYTE, RGB8, 1)

	case TKTX_B8G8R8_UNORM: FT(BGR, UNSIGNED_BYTE, RGB8, 1)
	case TKTX_B8G8R8_SNORM: FT(BGR, BYTE, RGB8_SNORM, 1)
	case TKTX_B8G8R8_UINT: FT(BGR_INTEGER, UNSIGNED_BYTE, RGB8UI, 1)
	case TKTX_B8G8R8_SINT: FT(BGR_INTEGER, BYTE, RGB8I, 1)
	case TKTX_B8G8R8_SRGB: break;//FT(SBGR, SRGB8)

	case TKTX_R8G8B8A8_UNORM:FT(RGBA, UNSIGNED_BYTE, RGBA8, 1)
	case TKTX_R8G8B8A8_SNORM:FT(RGBA, BYTE, RGBA8_SNORM, 1)
	case TKTX_R8G8B8A8_UINT: FT(RGBA_INTEGER, UNSIGNED_BYTE, RGBA8UI, 1)
	case TKTX_R8G8B8A8_SINT: FT(RGBA_INTEGER, BYTE, RGBA8I, 1)
	case TKTX_R8G8B8A8_SRGB: FT(SRGB_ALPHA, UNSIGNED_BYTE, RGBA8, 1)

	case TKTX_B8G8R8A8_UNORM: FT(BGRA, UNSIGNED_BYTE, RGBA8, 1)
	case TKTX_B8G8R8A8_SNORM: FT(BGRA, BYTE, RGBA8_SNORM, 1)
	case TKTX_B8G8R8A8_UINT: FT(BGRA_INTEGER, UNSIGNED_BYTE, RGBA8UI, 1)
	case TKTX_B8G8R8A8_SINT: FT(BGRA_INTEGER, BYTE, RGBA8I, 1)
	case TKTX_B8G8R8A8_SRGB: break; //FT(SBGR_ALPHA, UNSIGNED_BYTE)

	case TKTX_E5B9G9R9_UFLOAT_PACK32: FT(BGR, UNSIGNED_INT_5_9_9_9_REV, RGB9_E5, 1);
	case TKTX_A8B8G8R8_UNORM_PACK32: FT(ABGR, UNSIGNED_BYTE, RGBA8, 1)
	case TKTX_A8B8G8R8_SNORM_PACK32: FT(ABGR, BYTE, RGBA8, 1)
	case TKTX_A8B8G8R8_UINT_PACK32: break;//FT(ABGR_INTEGER, UNSIGNED_BYTE)
	case TKTX_A8B8G8R8_SINT_PACK32: break;//FT(ABGR_INTEGER, BYTE)
	case TKTX_A8B8G8R8_SRGB_PACK32: break;//FT(ALPHA_SBGR, UNSIGNED_BYTE)
	case TKTX_B10G11R11_UFLOAT_PACK32: FT(BGR, UNSIGNED_INT_10F_11F_11F_REV, R11F_G11F_B10F, 1)

	case TKTX_R16_UNORM: FT(RED, UNSIGNED_SHORT, R16, 2)
	case TKTX_R16_SNORM: FT(RED, SHORT, R16_SNORM, 2)
	case TKTX_R16_UINT: FT(RED_INTEGER, UNSIGNED_SHORT, R16UI, 2)
	case TKTX_R16_SINT: FT(RED_INTEGER, SHORT, R16I, 2)
	case TKTX_R16_SFLOAT:FT(RED, HALF_FLOAT, R16F, 2)

	case TKTX_R16G16_UNORM: FT(RG, UNSIGNED_SHORT, RG16, 2)
	case TKTX_R16G16_SNORM: FT(RG, SHORT, RG16_SNORM, 2)
	case TKTX_R16G16_UINT: FT(RG_INTEGER, UNSIGNED_SHORT, RG16UI, 2)
	case TKTX_R16G16_SINT: FT(RG_INTEGER, SHORT, RG16I, 2)
	case TKTX_R16G16_SFLOAT:FT(RG, HALF_FLOAT, RG16F, 2)

	case TKTX_R16G16B16_UNORM: FT(RGB, UNSIGNED_SHORT, RGB16, 2)
	case TKTX_R16G16B16_SNORM: FT(RGB, SHORT, RGB16_SNORM, 2)
	case TKTX_R16G16B16_UINT: FT(RGB_INTEGER, UNSIGNED_SHORT, RGB16UI, 2)
	case TKTX_R16G16B16_SINT: FT(RGB_INTEGER, SHORT, RGB16I, 2)
	case TKTX_R16G16B16_SFLOAT: FT(RGB, HALF_FLOAT, RGB16F, 2)

	case TKTX_R16G16B16A16_UNORM: FT(RGBA, UNSIGNED_SHORT, RGBA16, 2)
	case TKTX_R16G16B16A16_SNORM: FT(RGBA, SHORT, RGBA16_SNORM, 2)
	case TKTX_R16G16B16A16_UINT: FT(RGBA_INTEGER, UNSIGNED_SHORT, RGBA16UI, 2)
	case TKTX_R16G16B16A16_SINT: FT(RGBA_INTEGER, SHORT, RGBA16I, 2)
	case TKTX_R16G16B16A16_SFLOAT:FT(RGBA, HALF_FLOAT, RGBA16F, 2)

	case TKTX_R32_UINT: FT(RED_INTEGER, UNSIGNED_INT, R32UI, 4)
	case TKTX_R32_SINT: FT(RED_INTEGER, INT, R32I, 4)
	case TKTX_R32_SFLOAT: FT(RED, FLOAT, R32F, 4)

	case TKTX_R32G32_UINT: FT(RG_INTEGER, UNSIGNED_INT, RG32UI, 4)
	case TKTX_R32G32_SINT: FT(RG_INTEGER, INT, RG32I, 4)
	case TKTX_R32G32_SFLOAT: FT(RG, FLOAT, RG32F, 4)

	case TKTX_R32G32B32_UINT: FT(RGB_INTEGER, UNSIGNED_INT, RGB32UI, 4)
	case TKTX_R32G32B32_SINT: FT(RGB_INTEGER, INT, RGB32I, 4)
	case TKTX_R32G32B32_SFLOAT: FT(RGB_INTEGER, FLOAT, RGB32F, 4)

	case TKTX_R32G32B32A32_UINT: FT(RGBA_INTEGER, UNSIGNED_INT, RGBA32UI, 4)
	case TKTX_R32G32B32A32_SINT: FT(RGBA_INTEGER, INT, RGBA32I, 4)
	case TKTX_R32G32B32A32_SFLOAT:FT(RGBA, FLOAT, RGBA32F, 4)

	case TKTX_BC1_RGB_UNORM_BLOCK: FTC(RGB, RGB_S3TC_DXT1)
	case TKTX_BC1_RGB_SRGB_BLOCK: FTC(RGB, SRGB_S3TC_DXT1)
	case TKTX_BC1_RGBA_UNORM_BLOCK: FTC(RGBA, RGBA_S3TC_DXT1)
	case TKTX_BC1_RGBA_SRGB_BLOCK: FTC(RGBA, SRGB_ALPHA_S3TC_DXT1)
	case TKTX_BC2_UNORM_BLOCK: FTC(RGBA, RGBA_S3TC_DXT3)
	case TKTX_BC2_SRGB_BLOCK: FTC(RGBA, SRGB_ALPHA_S3TC_DXT3)
	case TKTX_BC3_UNORM_BLOCK: FTC(RGBA, RGBA_S3TC_DXT5)
	case TKTX_BC3_SRGB_BLOCK: FTC(RGBA, SRGB_ALPHA_S3TC_DXT5)
	case TKTX_BC4_UNORM_BLOCK: FTC(RED, RED_RGTC1)
	case TKTX_BC4_SNORM_BLOCK: FTC(RED, SIGNED_RED_RGTC1)
	case TKTX_BC5_UNORM_BLOCK: FTC(RG, RED_GREEN_RGTC2)
	case TKTX_BC5_SNORM_BLOCK: FTC(RG, SIGNED_RED_GREEN_RGTC2)
	case TKTX_BC6H_UFLOAT_BLOCK: FTC(RGB, RGB_BPTC_UNSIGNED_FLOAT)
	case TKTX_BC6H_SFLOAT_BLOCK: FTC(RGB, RGB_BPTC_SIGNED_FLOAT)
	case TKTX_BC7_UNORM_BLOCK: FTC(RGBA, RGBA_BPTC_UNORM)
	case TKTX_BC7_SRGB_BLOCK: FTC(RGBA, SRGB_ALPHA_BPTC_UNORM)

	case TKTX_ETC2_R8G8B8_UNORM_BLOCK: FTC(RGB, RGB8_ETC2)
	case TKTX_ETC2_R8G8B8A1_UNORM_BLOCK: FTC(RGBA, RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
	case TKTX_ETC2_R8G8B8A8_UNORM_BLOCK: FTC(RGBA, RGBA8_ETC2_EAC)
	case TKTX_ETC2_R8G8B8_SRGB_BLOCK: FTC(SRGB, SRGB8_ETC2)
	case TKTX_ETC2_R8G8B8A1_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_PUNCHTHROUGH_ALPHA1_ETC2)
	case TKTX_ETC2_R8G8B8A8_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ETC2_EAC)
	case TKTX_EAC_R11_UNORM_BLOCK: FTC(RED, R11_EAC)
	case TKTX_EAC_R11G11_UNORM_BLOCK: FTC(RG, RG11_EAC)
	case TKTX_EAC_R11_SNORM_BLOCK: FTC(RED, SIGNED_R11_EAC)
	case TKTX_EAC_R11G11_SNORM_BLOCK: FTC(RG, SIGNED_RG11_EAC)

	case TKTX_PVR_2BPP_BLOCK: FTC(RGB, RGB_PVRTC_2BPPV1)
	case TKTX_PVR_2BPPA_BLOCK: FTC(RGBA, RGBA_PVRTC_2BPPV1);
	case TKTX_PVR_4BPP_BLOCK: FTC(RGB, RGB_PVRTC_4BPPV1)
	case TKTX_PVR_4BPPA_BLOCK: FTC(RGB, RGB_PVRTC_4BPPV1)
	case TKTX_PVR_2BPP_SRGB_BLOCK: FTC(SRGB, SRGB_PVRTC_2BPPV1)
	case TKTX_PVR_2BPPA_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB_ALPHA_PVRTC_2BPPV1);
	case TKTX_PVR_4BPP_SRGB_BLOCK: FTC(SRGB, SRGB_PVRTC_2BPPV1)
	case TKTX_PVR_4BPPA_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB_ALPHA_PVRTC_2BPPV1);

	case TKTX_ASTC_4x4_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_4x4)
	case TKTX_ASTC_4x4_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_4x4)
	case TKTX_ASTC_5x4_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_5x4)
	case TKTX_ASTC_5x4_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_5x4)
	case TKTX_ASTC_5x5_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_5x5)
	case TKTX_ASTC_5x5_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_5x5)
	case TKTX_ASTC_6x5_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_6x5)
	case TKTX_ASTC_6x5_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_6x5)
	case TKTX_ASTC_6x6_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_6x6)
	case TKTX_ASTC_6x6_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_6x6)
	case TKTX_ASTC_8x5_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_8x5)
	case TKTX_ASTC_8x5_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_8x5)
	case TKTX_ASTC_8x6_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_8x6)
	case TKTX_ASTC_8x6_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_8x6)
	case TKTX_ASTC_8x8_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_8x8)
	case TKTX_ASTC_8x8_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_8x8)
	case TKTX_ASTC_10x5_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_10x5)
	case TKTX_ASTC_10x5_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_10x5)
	case TKTX_ASTC_10x6_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_10x6)
	case TKTX_ASTC_10x6_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_10x6)
	case TKTX_ASTC_10x8_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_10x8);
	case TKTX_ASTC_10x8_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_10x8)
	case TKTX_ASTC_10x10_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_10x10)
	case TKTX_ASTC_10x10_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_10x10)
	case TKTX_ASTC_12x10_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_12x10)
	case TKTX_ASTC_12x10_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_12x10)
	case TKTX_ASTC_12x12_UNORM_BLOCK: FTC(RGBA, RGBA_ASTC_12x12)
	case TKTX_ASTC_12x12_SRGB_BLOCK: FTC(SRGB_ALPHA, SRGB8_ALPHA8_ASTC_12x12)

	default:break;
	}
	return false;
}
#undef FT
#undef FTC

TinyKtx_Format TinyKtx_CrackFormatFromGL(uint32_t const glformat,
																				 uint32_t const gltype,
																				 uint32_t const glinternalformat,
																				 uint32_t const typesize) {
	switch (glinternalformat) {
	case TINYKTX_GL_COMPRESSED_RGB_S3TC_DXT1: return TKTX_BC1_RGB_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT1: return TKTX_BC1_RGBA_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT3: return TKTX_BC2_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_S3TC_DXT5: return TKTX_BC3_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_3DC_X_AMD: return TKTX_BC4_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_3DC_XY_AMD: return TKTX_BC5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_PVRTC_2BPPV1: return TKTX_PVR_2BPP_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_PVRTC_4BPPV1: return TKTX_PVR_2BPPA_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1: return TKTX_PVR_2BPPA_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1: return TKTX_PVR_4BPPA_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB_PVRTC_4BPPV1: return TKTX_PVR_4BPP_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB_PVRTC_2BPPV1: return TKTX_PVR_2BPP_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_PVRTC_4BPPV1: return TKTX_PVR_4BPPA_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_PVRTC_2BPPV1: return TKTX_PVR_2BPPA_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_S3TC_DXT1: return TKTX_BC1_RGB_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1: return TKTX_BC1_RGBA_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3: return TKTX_BC2_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5: return TKTX_BC3_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_LUMINANCE_LATC1: return TKTX_BC4_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_LUMINANCE_LATC1: return TKTX_BC4_SNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_LUMINANCE_ALPHA_LATC2: return TKTX_BC5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2: return TKTX_BC5_SNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RED_RGTC1: return TKTX_BC4_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_RED_RGTC1: return TKTX_BC4_SNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RED_GREEN_RGTC2: return TKTX_BC5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2: return TKTX_BC5_SNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_ETC1_RGB8_OES: return TKTX_ETC2_R8G8B8_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_BPTC_UNORM: return TKTX_BC7_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: return TKTX_BC7_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: return TKTX_BC6H_SFLOAT_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return TKTX_BC6H_UFLOAT_BLOCK;
	case TINYKTX_GL_COMPRESSED_R11_EAC: return TKTX_EAC_R11_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_R11_EAC: return TKTX_EAC_R11_SNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RG11_EAC: return TKTX_EAC_R11G11_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SIGNED_RG11_EAC: return TKTX_EAC_R11G11_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB8_ETC2: return TKTX_ETC2_R8G8B8_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ETC2: return TKTX_ETC2_R8G8B8_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2: return TKTX_ETC2_R8G8B8A1_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: return TKTX_ETC2_R8G8B8A1_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA8_ETC2_EAC: return TKTX_ETC2_R8G8B8A8_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return TKTX_ETC2_R8G8B8A8_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_4x4: 	return TKTX_ASTC_4x4_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_5x4: 	return TKTX_ASTC_5x4_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_5x5: 	return TKTX_ASTC_5x5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_6x5: 	return TKTX_ASTC_6x5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_6x6: 	return TKTX_ASTC_6x6_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x5: 	return TKTX_ASTC_8x5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x6: 	return TKTX_ASTC_8x6_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_8x8: 	return TKTX_ASTC_8x8_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x5: 	return TKTX_ASTC_10x5_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x6: 	return TKTX_ASTC_10x6_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x8: 	return TKTX_ASTC_10x8_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_10x10: return TKTX_ASTC_10x10_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_12x10: return TKTX_ASTC_12x10_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_RGBA_ASTC_12x12: return TKTX_ASTC_12x12_UNORM_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4: 	return TKTX_ASTC_4x4_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4: 	return TKTX_ASTC_5x4_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5: 	return TKTX_ASTC_5x5_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5: 	return TKTX_ASTC_6x5_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6: 	return TKTX_ASTC_6x6_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5: 	return TKTX_ASTC_8x5_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6: 	return TKTX_ASTC_8x6_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8: 	return TKTX_ASTC_8x8_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5: 	return TKTX_ASTC_10x5_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6: 	return TKTX_ASTC_10x6_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8: 	return TKTX_ASTC_10x8_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10: return TKTX_ASTC_10x10_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10: return TKTX_ASTC_12x10_SRGB_BLOCK;
	case TINYKTX_GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12: return TKTX_ASTC_12x12_SRGB_BLOCK;

	// non compressed but 'simple' internal format mapping
	case TINYKTX_GL_INTFORMAT_ALPHA8: return TKTX_R8_UNORM;
	case TINYKTX_GL_INTFORMAT_ALPHA16: return TKTX_R16_UNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE8: return TKTX_R8_UNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE16: return TKTX_R16_UNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE8_ALPHA8: return TKTX_R8G8_UNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE16_ALPHA16: return TKTX_R16G16_UNORM;
	case TINYKTX_GL_INTFORMAT_INTENSITY8: return TKTX_R8_UNORM;
	case TINYKTX_GL_INTFORMAT_INTENSITY16: return TKTX_R16_UNORM;
	case TINYKTX_GL_INTFORMAT_RGB8: return TKTX_R8G8B8_UNORM;
	case TINYKTX_GL_INTFORMAT_RGB16: return TKTX_R16G16B16_UNORM;
	case TINYKTX_GL_INTFORMAT_RGBA4: return TKTX_R4G4B4A4_UNORM_PACK16;
	case TINYKTX_GL_INTFORMAT_RGB5_A1: return TKTX_R5G5B5A1_UNORM_PACK16;
	case TINYKTX_GL_INTFORMAT_RGBA8: return TKTX_R8G8B8A8_UNORM;
	case TINYKTX_GL_INTFORMAT_RGB10_A2: return TKTX_A2R10G10B10_UNORM_PACK32; //??
	case TINYKTX_GL_INTFORMAT_RGBA16: return TKTX_R16G16B16A16_UNORM;
	case TINYKTX_GL_INTFORMAT_R8: return TKTX_R8_UNORM;
	case TINYKTX_GL_INTFORMAT_R16: return TKTX_R16_UNORM;
	case TINYKTX_GL_INTFORMAT_RG8: return TKTX_R8G8_UNORM;
	case TINYKTX_GL_INTFORMAT_RG16: return TKTX_R16G16_UNORM;
	case TINYKTX_GL_INTFORMAT_R16F: return TKTX_R16_SFLOAT;
	case TINYKTX_GL_INTFORMAT_R32F: return TKTX_R32_SFLOAT;
	case TINYKTX_GL_INTFORMAT_RG16F: return TKTX_R16G16_SFLOAT;
	case TINYKTX_GL_INTFORMAT_RG32F: return TKTX_R32_SFLOAT;
	case TINYKTX_GL_INTFORMAT_R8I: return TKTX_R8_SINT;
	case TINYKTX_GL_INTFORMAT_R8UI: return TKTX_R8_UINT;
	case TINYKTX_GL_INTFORMAT_R16I: return TKTX_R16_SINT;
	case TINYKTX_GL_INTFORMAT_R16UI: return TKTX_R16_UINT;
	case TINYKTX_GL_INTFORMAT_R32I: return TKTX_R32_SINT;
	case TINYKTX_GL_INTFORMAT_R32UI: return TKTX_R32_UINT;
	case TINYKTX_GL_INTFORMAT_RG8I: return TKTX_R8G8_SINT;
	case TINYKTX_GL_INTFORMAT_RG8UI: return TKTX_R8G8_UINT;
	case TINYKTX_GL_INTFORMAT_RG16I: return TKTX_R16G16_SINT;
	case TINYKTX_GL_INTFORMAT_RG16UI: return TKTX_R16G16_UINT;
	case TINYKTX_GL_INTFORMAT_RG32I: return TKTX_R32G32_SINT;
	case TINYKTX_GL_INTFORMAT_RG32UI: return TKTX_R32G32_UINT;
	case TINYKTX_GL_INTFORMAT_RGBA32F: return TKTX_R32G32B32A32_SFLOAT;
	case TINYKTX_GL_INTFORMAT_RGB32F: return TKTX_R32G32B32_SFLOAT;
	case TINYKTX_GL_INTFORMAT_RGBA16F: return TKTX_R16G16B16A16_SFLOAT;
	case TINYKTX_GL_INTFORMAT_RGB16F: return TKTX_R16G16B16_SFLOAT;
	case TINYKTX_GL_INTFORMAT_R11F_G11F_B10F: return TKTX_B10G11R11_UFLOAT_PACK32; //??
	case TINYKTX_GL_INTFORMAT_UNSIGNED_INT_10F_11F_11F_REV: return TKTX_B10G11R11_UFLOAT_PACK32; //?
	case TINYKTX_GL_INTFORMAT_RGB9_E5: return TKTX_E5B9G9R9_UFLOAT_PACK32;
	case TINYKTX_GL_INTFORMAT_SRGB8: return TKTX_R8G8B8_SRGB;
	case TINYKTX_GL_INTFORMAT_SRGB8_ALPHA8: return TKTX_R8G8B8A8_SRGB;
	case TINYKTX_GL_INTFORMAT_SLUMINANCE8_ALPHA8: return TKTX_R8G8_SRGB;
	case TINYKTX_GL_INTFORMAT_SLUMINANCE8: return TKTX_R8_SRGB;
	case TINYKTX_GL_INTFORMAT_RGB565: return TKTX_R5G6B5_UNORM_PACK16;
	case TINYKTX_GL_INTFORMAT_RGBA32UI: return TKTX_R32G32B32A32_UINT;
	case TINYKTX_GL_INTFORMAT_RGB32UI: return TKTX_R32G32B32A32_UINT;
	case TINYKTX_GL_INTFORMAT_RGBA16UI: return TKTX_R16G16B16A16_UINT;
	case TINYKTX_GL_INTFORMAT_RGB16UI:return TKTX_R16G16B16_UINT;
	case TINYKTX_GL_INTFORMAT_RGBA8UI:return TKTX_R8G8B8A8_UINT;
	case TINYKTX_GL_INTFORMAT_RGB8UI:return  TKTX_R8G8B8_UINT;
	case TINYKTX_GL_INTFORMAT_RGBA32I:return TKTX_R32G32B32A32_SINT;
	case TINYKTX_GL_INTFORMAT_RGB32I:return TKTX_R32G32B32A32_SINT;
	case TINYKTX_GL_INTFORMAT_RGBA16I:return TKTX_R16G16B16A16_SINT;
	case TINYKTX_GL_INTFORMAT_RGB16I:return TKTX_R16G16B16_SINT;
	case TINYKTX_GL_INTFORMAT_RGBA8I:return TKTX_R8G8B8A8_UINT;
	case TINYKTX_GL_INTFORMAT_RGB8I:return  TKTX_R8G8B8_UINT;
	case TINYKTX_GL_INTFORMAT_R8_SNORM: return TKTX_R8_SNORM;
	case TINYKTX_GL_INTFORMAT_RG8_SNORM: return TKTX_R8G8_SNORM;
	case TINYKTX_GL_INTFORMAT_RGB8_SNORM: return TKTX_R8G8B8_SNORM;
	case TINYKTX_GL_INTFORMAT_RGBA8_SNORM: return TKTX_R8G8B8A8_SNORM;
	case TINYKTX_GL_INTFORMAT_R16_SNORM:return TKTX_R16_SNORM;
	case TINYKTX_GL_INTFORMAT_RG16_SNORM: return TKTX_R16G16_SNORM;
	case TINYKTX_GL_INTFORMAT_RGB16_SNORM: return TKTX_R16G16B16_SNORM;
	case TINYKTX_GL_INTFORMAT_RGBA16_SNORM: return TKTX_R16G16B16_SNORM;
	case TINYKTX_GL_INTFORMAT_ALPHA8_SNORM: return TKTX_R8_SNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE8_SNORM: return TKTX_R8_SNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE8_ALPHA8_SNORM: return TKTX_R8G8_SNORM;
	case TINYKTX_GL_INTFORMAT_INTENSITY8_SNORM: return TKTX_R8_SNORM;
	case TINYKTX_GL_INTFORMAT_ALPHA16_SNORM: return TKTX_R16_SNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE16_SNORM: return TKTX_R16_SNORM;
	case TINYKTX_GL_INTFORMAT_LUMINANCE16_ALPHA16_SNORM: return TKTX_R16G16_SNORM;
	case TINYKTX_GL_INTFORMAT_INTENSITY16_SNORM: return TKTX_R16_SNORM;

	// can't handle yet
	case TINYKTX_GL_INTFORMAT_ALPHA4:
	case TINYKTX_GL_INTFORMAT_ALPHA12:
	case TINYKTX_GL_INTFORMAT_LUMINANCE4:
	case TINYKTX_GL_INTFORMAT_LUMINANCE12:
	case TINYKTX_GL_INTFORMAT_LUMINANCE4_ALPHA4:
	case TINYKTX_GL_INTFORMAT_LUMINANCE6_ALPHA2:
	case TINYKTX_GL_INTFORMAT_LUMINANCE12_ALPHA4:
	case TINYKTX_GL_INTFORMAT_LUMINANCE12_ALPHA12:
	case TINYKTX_GL_INTFORMAT_INTENSITY4:
	case TINYKTX_GL_INTFORMAT_INTENSITY12:
	case TINYKTX_GL_INTFORMAT_RGB2:
	case TINYKTX_GL_INTFORMAT_RGB4:
	case TINYKTX_GL_INTFORMAT_RGB5:
	case TINYKTX_GL_INTFORMAT_RGB10:
	case TINYKTX_GL_INTFORMAT_RGB12:
	case TINYKTX_GL_INTFORMAT_RGBA2:
	case TINYKTX_GL_INTFORMAT_RGBA12:
	case TINYKTX_GL_INTFORMAT_FLOAT_32_UNSIGNED_INT_24_8_REV:
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2:
	case TINYKTX_GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2:
	case TINYKTX_GL_COMPRESSED_ATC_RGB:
	case TINYKTX_GL_COMPRESSED_ATC_RGBA_EXPLICIT_ALPHA:
	case TINYKTX_GL_COMPRESSED_ATC_RGBA_INTERPOLATED_ALPHA:
	default: return TKTX_UNDEFINED;
	}
}

TinyKtx_Format TinyKtx_GetFormat(TinyKtx_ContextHandle handle) {
	uint32_t glformat;
	uint32_t gltype;
	uint32_t glinternalformat;
	uint32_t typesize;
	uint32_t glbaseinternalformat;

	if(TinyKtx_GetFormatGL(handle, &glformat, &gltype, &glinternalformat, &typesize, &glbaseinternalformat) == false)
		return TKTX_UNDEFINED;

	return TinyKtx_CrackFormatFromGL(glformat, gltype, glinternalformat, typesize);
}

bool TinyKtx_GetFormatGL(TinyKtx_ContextHandle handle, uint32_t *glformat, uint32_t *gltype, uint32_t *glinternalformat, uint32_t* typesize, uint32_t* glbaseinternalformat) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	*glformat = ctx->header.glFormat;
	*gltype = ctx->header.glType;
	*glinternalformat = ctx->header.glInternalFormat;
	*glbaseinternalformat = ctx->header.glBaseInternalFormat;
	*typesize = ctx->header.glBaseInternalFormat;

	return true;
}

static uint32_t imageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel, bool seekLast) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return 0;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}
	if (mipmaplevel >= ctx->header.numberOfMipmapLevels) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}
	if (mipmaplevel >= TINYKTX_MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}
	if (ctx->mipMapSizes[mipmaplevel] != 0)
		return ctx->mipMapSizes[mipmaplevel];

	uint64_t currentOffset = ctx->firstImagePos;
	for (uint32_t i = 0; i <= mipmaplevel; ++i) {
		uint32_t size;
		// if we have already read this mipmaps size use it
		if (ctx->mipMapSizes[i] != 0) {
			size = ctx->mipMapSizes[i];
			if (seekLast && i == mipmaplevel) {
				ctx->callbacks.seek(ctx->user, currentOffset + sizeof(uint32_t));
			}
		} else {
			// otherwise seek and read it
			ctx->callbacks.seek(ctx->user, currentOffset);
			size_t readchk = ctx->callbacks.read(ctx->user, &size, sizeof(uint32_t));
			if(readchk != 4) {
				ctx->callbacks.error(ctx->user, "Reading image size error");
				return 0;
			}

			if (ctx->header.numberOfFaces == 6 && ctx->header.numberOfArrayElements == 0) {
				size = ((size + 3u) & ~3u) * 6; // face padding and 6 faces
			}

			ctx->mipMapSizes[i] = size;
		}
		currentOffset += (size + sizeof(uint32_t) + 3u) & ~3u; // size + mip padding
	}

	return ctx->mipMapSizes[mipmaplevel];
}
uint32_t TinyKtx_ImageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel) {
	return imageSize(handle, mipmaplevel, false);
}

void const *TinyKtx_ImageRawData(TinyKtx_ContextHandle handle, uint32_t mipmaplevel) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return NULL;

	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return NULL;
	}

	if (mipmaplevel >= ctx->header.numberOfMipmapLevels) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if (mipmaplevel >= TINYKTX_MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if (ctx->mipmaps[mipmaplevel] != NULL)
		return ctx->mipmaps[mipmaplevel];

	uint32_t size = imageSize(handle, mipmaplevel, true);
	if (size == 0)
		return NULL;

	ctx->mipmaps[mipmaplevel] = ctx->callbacks.alloc(ctx->user, size);
	if (ctx->mipmaps[mipmaplevel]) {
		ctx->callbacks.read(ctx->user, (void *) ctx->mipmaps[mipmaplevel], size);
	}

	return ctx->mipmaps[mipmaplevel];
}

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
												void const **mipmaps) {
	uint32_t glformat;
	uint32_t glinternalFormat;
	uint32_t gltype;
	uint32_t gltypeSize;
	if (TinyKtx_CrackFormatToGL(format, &glformat, &gltype, &glinternalFormat, &gltypeSize) == false)
		return false;

	return TinyKtx_WriteImageGL(callbacks,
															user,
															width,
															height,
															depth,
															slices,
															mipmaplevels,
															glformat,
															glinternalFormat,
															glinternalFormat, //??
															gltype,
															gltypeSize,
															cubemap,
															mipmapsizes,
															mipmaps
	);

}

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
													void const **mipmaps) {

	TinyKtx_Header header;
	memcpy(header.identifier, fileIdentifier, 12);
	header.endianness = 0x04030201;
	header.glFormat = format;
	header.glInternalFormat = internalFormat;
	header.glBaseInternalFormat = baseFormat;
	header.glType = type;
	header.glTypeSize = typeSize;
	header.pixelWidth = width;
	header.pixelHeight = height;
	header.pixelDepth = depth;
	header.numberOfArrayElements = slices;
	header.numberOfFaces = cubemap ? 6 : 1;
	header.numberOfMipmapLevels = mipmaplevels;
	// TODO keyvalue pair data
	header.bytesOfKeyValueData = 0;
	callbacks->write(user, &header, sizeof(TinyKtx_Header));

	static uint8_t const padding[4] = {0, 0, 0, 0};

	// TODO this might be wrong for non array cubemaps with < 4 bytes per pixel...
	// cubemap padding needs factoring in.
	for (uint32_t i = 0u; i < mipmaplevels; ++i) {
		callbacks->write(user, mipmapsizes + i, sizeof(uint32_t));
		callbacks->write(user, mipmaps[i], mipmapsizes[i]);
		callbacks->write(user, padding, ((mipmapsizes[i] + 3u) & ~3u) - mipmapsizes[i]);
	}

	return true;
}

#ifdef __cplusplus
};
#endif

#if 0
static inline VkFormat vkGetFormatFromOpenGLFormat( const GLenum format, const GLenum type )
{
	switch ( type )
	{
		//
		// 8 bits per component
		//
	case GL_UNSIGNED_BYTE:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R8_UNORM;
		case GL_RG:						return VK_FORMAT_R8G8_UNORM;
		case GL_RGB:					return VK_FORMAT_R8G8B8_UNORM;
		case GL_BGR:					return VK_FORMAT_B8G8R8_UNORM;
		case GL_RGBA:					return VK_FORMAT_R8G8B8A8_UNORM;
		case GL_BGRA:					return VK_FORMAT_B8G8R8A8_UNORM;
		case GL_RED_INTEGER:			return VK_FORMAT_R8_UINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R8G8_UINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R8G8B8_UINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_B8G8R8_UINT;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R8G8B8A8_UINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_B8G8R8A8_UINT;
		}
		break;
	}
	case GL_BYTE:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R8_SNORM;
		case GL_RG:						return VK_FORMAT_R8G8_SNORM;
		case GL_RGB:					return VK_FORMAT_R8G8B8_SNORM;
		case GL_BGR:					return VK_FORMAT_B8G8R8_SNORM;
		case GL_RGBA:					return VK_FORMAT_R8G8B8A8_SNORM;
		case GL_BGRA:					return VK_FORMAT_B8G8R8A8_SNORM;
		case GL_RED_INTEGER:			return VK_FORMAT_R8_SINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R8G8_SINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R8G8B8_SINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_B8G8R8_SINT;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R8G8B8A8_SINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_B8G8R8A8_SINT;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}

		//
		// 16 bits per component
		//
	case GL_UNSIGNED_SHORT:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R16_UNORM;
		case GL_RG:						return VK_FORMAT_R16G16_UNORM;
		case GL_RGB:					return VK_FORMAT_R16G16B16_UNORM;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R16G16B16A16_UNORM;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R16_UINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R16G16_UINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R16G16B16_UINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R16G16B16A16_UINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_D16_UNORM;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_D16_UNORM_S8_UINT;
		}
		break;
	}
	case GL_SHORT:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R16_SNORM;
		case GL_RG:						return VK_FORMAT_R16G16_SNORM;
		case GL_RGB:					return VK_FORMAT_R16G16B16_SNORM;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R16G16B16A16_SNORM;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R16_SINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R16G16_SINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R16G16B16_SINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R16G16B16A16_SINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}
	case GL_HALF_FLOAT:
	case GL_HALF_FLOAT_OES:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R16_SFLOAT;
		case GL_RG:						return VK_FORMAT_R16G16_SFLOAT;
		case GL_RGB:					return VK_FORMAT_R16G16B16_SFLOAT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R16G16B16A16_SFLOAT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RG_INTEGER:				return VK_FORMAT_UNDEFINED;
		case GL_RGB_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}

		//
		// 32 bits per component
		//
	case GL_UNSIGNED_INT:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R32_UINT;
		case GL_RG:						return VK_FORMAT_R32G32_UINT;
		case GL_RGB:					return VK_FORMAT_R32G32B32_UINT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R32G32B32A32_UINT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R32_UINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R32G32_UINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R32G32B32_UINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R32G32B32A32_UINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_X8_D24_UNORM_PACK32;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_D24_UNORM_S8_UINT;
		}
		break;
	}
	case GL_INT:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R32_SINT;
		case GL_RG:						return VK_FORMAT_R32G32_SINT;
		case GL_RGB:					return VK_FORMAT_R32G32B32_SINT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R32G32B32A32_SINT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R32_SINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R32G32_SINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R32G32B32_SINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R32G32B32A32_SINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}
	case GL_FLOAT:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R32_SFLOAT;
		case GL_RG:						return VK_FORMAT_R32G32_SFLOAT;
		case GL_RGB:					return VK_FORMAT_R32G32B32_SFLOAT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R32G32B32A32_SFLOAT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RG_INTEGER:				return VK_FORMAT_UNDEFINED;
		case GL_RGB_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_D32_SFLOAT;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		}
		break;
	}

		//
		// 64 bits per component
		//
	case GL_UNSIGNED_INT64:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R64_UINT;
		case GL_RG:						return VK_FORMAT_R64G64_UINT;
		case GL_RGB:					return VK_FORMAT_R64G64B64_UINT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R64G64B64A64_UINT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RG_INTEGER:				return VK_FORMAT_UNDEFINED;
		case GL_RGB_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}
	case GL_INT64:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R64_SINT;
		case GL_RG:						return VK_FORMAT_R64G64_SINT;
		case GL_RGB:					return VK_FORMAT_R64G64B64_SINT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R64G64B64A64_SINT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R64_SINT;
		case GL_RG_INTEGER:				return VK_FORMAT_R64G64_SINT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R64G64B64_SINT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R64G64B64A64_SINT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}
	case GL_DOUBLE:
	{
		switch ( format )
		{
		case GL_RED:					return VK_FORMAT_R64_SFLOAT;
		case GL_RG:						return VK_FORMAT_R64G64_SFLOAT;
		case GL_RGB:					return VK_FORMAT_R64G64B64_SFLOAT;
		case GL_BGR:					return VK_FORMAT_UNDEFINED;
		case GL_RGBA:					return VK_FORMAT_R64G64B64A64_SFLOAT;
		case GL_BGRA:					return VK_FORMAT_UNDEFINED;
		case GL_RED_INTEGER:			return VK_FORMAT_R64_SFLOAT;
		case GL_RG_INTEGER:				return VK_FORMAT_R64G64_SFLOAT;
		case GL_RGB_INTEGER:			return VK_FORMAT_R64G64B64_SFLOAT;
		case GL_BGR_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_RGBA_INTEGER:			return VK_FORMAT_R64G64B64A64_SFLOAT;
		case GL_BGRA_INTEGER:			return VK_FORMAT_UNDEFINED;
		case GL_STENCIL_INDEX:			return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_COMPONENT:		return VK_FORMAT_UNDEFINED;
		case GL_DEPTH_STENCIL:			return VK_FORMAT_UNDEFINED;
		}
		break;
	}

		//
		// Packed
		//
	case GL_UNSIGNED_BYTE_3_3_2:
		assert( format == GL_RGB || format == GL_RGB_INTEGER );
		return VK_FORMAT_UNDEFINED;
	case GL_UNSIGNED_BYTE_2_3_3_REV:
		assert( format == GL_BGR || format == GL_BGR_INTEGER );
		return VK_FORMAT_UNDEFINED;
	case GL_UNSIGNED_SHORT_5_6_5:
		assert( format == GL_RGB || format == GL_RGB_INTEGER );
		return VK_FORMAT_R5G6B5_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_5_6_5_REV:
		assert( format == GL_BGR || format == GL_BGR_INTEGER );
		return VK_FORMAT_B5G6R5_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
	case GL_UNSIGNED_INT_8_8_8_8:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return ( format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER ) ? VK_FORMAT_R8G8B8A8_UINT : VK_FORMAT_R8G8B8A8_UNORM;
	case GL_UNSIGNED_INT_8_8_8_8_REV:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return ( format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER ) ? VK_FORMAT_A8B8G8R8_UINT_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32;
	case GL_UNSIGNED_INT_10_10_10_2:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return ( format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER ) ? VK_FORMAT_A2R10G10B10_UINT_PACK32 : VK_FORMAT_A2R10G10B10_UNORM_PACK32;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		assert( format == GL_RGB || format == GL_BGRA || format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER );
		return ( format == GL_RGB_INTEGER || format == GL_BGRA_INTEGER ) ? VK_FORMAT_A2B10G10R10_UINT_PACK32 : VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
		assert( format == GL_RGB || format == GL_BGR );
		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case GL_UNSIGNED_INT_5_9_9_9_REV:
		assert( format == GL_RGB || format == GL_BGR );
		return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
	case GL_UNSIGNED_INT_24_8:
		assert( format == GL_DEPTH_STENCIL );
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
		assert( format == GL_DEPTH_STENCIL );
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	return VK_FORMAT_UNDEFINED;
}

static inline VkFormat vkGetFormatFromOpenGLType( const GLenum type, const GLuint numComponents, const GLboolean normalized )
{
	switch ( type )
	{
		//
		// 8 bits per component
		//
	case GL_UNSIGNED_BYTE:
	{
		switch ( numComponents )
		{
		case 1:							return normalized ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_UINT;
		case 2:							return normalized ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8G8_UINT;
		case 3:							return normalized ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8_UINT;
		case 4:							return normalized ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UINT;
		}
		break;
	}
	case GL_BYTE:
	{
		switch ( numComponents )
		{
		case 1:							return normalized ? VK_FORMAT_R8_SNORM : VK_FORMAT_R8_SINT;
		case 2:							return normalized ? VK_FORMAT_R8G8_SNORM : VK_FORMAT_R8G8_SINT;
		case 3:							return normalized ? VK_FORMAT_R8G8B8_SNORM : VK_FORMAT_R8G8B8_SINT;
		case 4:							return normalized ? VK_FORMAT_R8G8B8A8_SNORM : VK_FORMAT_R8G8B8A8_SINT;
		}
		break;
	}

		//
		// 16 bits per component
		//
	case GL_UNSIGNED_SHORT:
	{
		switch ( numComponents )
		{
		case 1:							return normalized ? VK_FORMAT_R16_UNORM : VK_FORMAT_R16_UINT;
		case 2:							return normalized ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R16G16_UINT;
		case 3:							return normalized ? VK_FORMAT_R16G16B16_UNORM : VK_FORMAT_R16G16B16_UINT;
		case 4:							return normalized ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_R16G16B16A16_UINT;
		}
		break;
	}
	case GL_SHORT:
	{
		switch ( numComponents )
		{
		case 1:							return normalized ? VK_FORMAT_R16_SNORM : VK_FORMAT_R16_SINT;
		case 2:							return normalized ? VK_FORMAT_R16G16_SNORM : VK_FORMAT_R16G16_SINT;
		case 3:							return normalized ? VK_FORMAT_R16G16B16_SNORM : VK_FORMAT_R16G16B16_SINT;
		case 4:							return normalized ? VK_FORMAT_R16G16B16A16_SNORM : VK_FORMAT_R16G16B16A16_SINT;
		}
		break;
	}
	case GL_HALF_FLOAT:
	case GL_HALF_FLOAT_OES:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R16_SFLOAT;
		case 2:							return VK_FORMAT_R16G16_SFLOAT;
		case 3:							return VK_FORMAT_R16G16B16_SFLOAT;
		case 4:							return VK_FORMAT_R16G16B16A16_SFLOAT;
		}
		break;
	}

		//
		// 32 bits per component
		//
	case GL_UNSIGNED_INT:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R32_UINT;
		case 2:							return VK_FORMAT_R32G32_UINT;
		case 3:							return VK_FORMAT_R32G32B32_UINT;
		case 4:							return VK_FORMAT_R32G32B32A32_UINT;
		}
		break;
	}
	case GL_INT:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R32_SINT;
		case 2:							return VK_FORMAT_R32G32_SINT;
		case 3:							return VK_FORMAT_R32G32B32_SINT;
		case 4:							return VK_FORMAT_R32G32B32A32_SINT;
		}
		break;
	}
	case GL_FLOAT:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R32_SFLOAT;
		case 2:							return VK_FORMAT_R32G32_SFLOAT;
		case 3:							return VK_FORMAT_R32G32B32_SFLOAT;
		case 4:							return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		break;
	}

		//
		// 64 bits per component
		//
	case GL_UNSIGNED_INT64:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R64_UINT;
		case 2:							return VK_FORMAT_R64G64_UINT;
		case 3:							return VK_FORMAT_R64G64B64_UINT;
		case 4:							return VK_FORMAT_R64G64B64A64_UINT;
		}
		break;
	}
	case GL_INT64:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R64_SINT;
		case 2:							return VK_FORMAT_R64G64_SINT;
		case 3:							return VK_FORMAT_R64G64B64_SINT;
		case 4:							return VK_FORMAT_R64G64B64A64_SINT;
		}
		break;
	}
	case GL_DOUBLE:
	{
		switch ( numComponents )
		{
		case 1:							return VK_FORMAT_R64_SFLOAT;
		case 2:							return VK_FORMAT_R64G64_SFLOAT;
		case 3:							return VK_FORMAT_R64G64B64_SFLOAT;
		case 4:							return VK_FORMAT_R64G64B64A64_SFLOAT;
		}
		break;
	}

		//
		// Packed
		//
	case GL_UNSIGNED_BYTE_3_3_2:			return VK_FORMAT_UNDEFINED;
	case GL_UNSIGNED_BYTE_2_3_3_REV:		return VK_FORMAT_UNDEFINED;
	case GL_UNSIGNED_SHORT_5_6_5:			return VK_FORMAT_R5G6B5_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_5_6_5_REV:		return VK_FORMAT_B5G6R5_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_4_4_4_4:			return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:		return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_5_5_5_1:			return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:		return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
	case GL_UNSIGNED_INT_8_8_8_8:			return normalized ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UINT;
	case GL_UNSIGNED_INT_8_8_8_8_REV:		return normalized ? VK_FORMAT_A8B8G8R8_UNORM_PACK32 : VK_FORMAT_A8B8G8R8_UINT_PACK32;
	case GL_UNSIGNED_INT_10_10_10_2:		return normalized ? VK_FORMAT_A2R10G10B10_UNORM_PACK32 : VK_FORMAT_A2R10G10B10_UINT_PACK32;
	case GL_UNSIGNED_INT_2_10_10_10_REV:	return normalized ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_A2B10G10R10_UINT_PACK32;
	case GL_UNSIGNED_INT_10F_11F_11F_REV:	return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case GL_UNSIGNED_INT_5_9_9_9_REV:		return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
	case GL_UNSIGNED_INT_24_8:				return VK_FORMAT_D24_UNORM_S8_UINT;
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:	return VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	return VK_FORMAT_UNDEFINED;
}

static inline VkFormat vkGetFormatFromOpenGLInternalFormat( const GLenum internalFormat )
{
	switch ( internalFormat )
	{
		//
		// 8 bits per component
		//
	case GL_R8:												return VK_FORMAT_R8_UNORM;					// 1-component, 8-bit unsigned normalized
	case GL_RG8:											return VK_FORMAT_R8G8_UNORM;				// 2-component, 8-bit unsigned normalized
	case GL_RGB8:											return VK_FORMAT_R8G8B8_UNORM;				// 3-component, 8-bit unsigned normalized
	case GL_RGBA8:											return VK_FORMAT_R8G8B8A8_UNORM;			// 4-component, 8-bit unsigned normalized

	case GL_R8_SNORM:										return VK_FORMAT_R8_SNORM;					// 1-component, 8-bit signed normalized
	case GL_RG8_SNORM:										return VK_FORMAT_R8G8_SNORM;				// 2-component, 8-bit signed normalized
	case GL_RGB8_SNORM:										return VK_FORMAT_R8G8B8_SNORM;				// 3-component, 8-bit signed normalized
	case GL_RGBA8_SNORM:									return VK_FORMAT_R8G8B8A8_SNORM;			// 4-component, 8-bit signed normalized

	case GL_R8UI:											return VK_FORMAT_R8_UINT;					// 1-component, 8-bit unsigned integer
	case GL_RG8UI:											return VK_FORMAT_R8G8_UINT;					// 2-component, 8-bit unsigned integer
	case GL_RGB8UI:											return VK_FORMAT_R8G8B8_UINT;				// 3-component, 8-bit unsigned integer
	case GL_RGBA8UI:										return VK_FORMAT_R8G8B8A8_UINT;				// 4-component, 8-bit unsigned integer

	case GL_R8I:											return VK_FORMAT_R8_SINT;					// 1-component, 8-bit signed integer
	case GL_RG8I:											return VK_FORMAT_R8G8_SINT;					// 2-component, 8-bit signed integer
	case GL_RGB8I:											return VK_FORMAT_R8G8B8_SINT;				// 3-component, 8-bit signed integer
	case GL_RGBA8I:											return VK_FORMAT_R8G8B8A8_SINT;				// 4-component, 8-bit signed integer

	case GL_SR8:											return VK_FORMAT_R8_SRGB;					// 1-component, 8-bit sRGB
	case GL_SRG8:											return VK_FORMAT_R8G8_SRGB;					// 2-component, 8-bit sRGB
	case GL_SRGB8:											return VK_FORMAT_R8G8B8_SRGB;				// 3-component, 8-bit sRGB
	case GL_SRGB8_ALPHA8:									return VK_FORMAT_R8G8B8A8_SRGB;				// 4-component, 8-bit sRGB

		//
		// 16 bits per component
		//
	case GL_R16:											return VK_FORMAT_R16_UNORM;					// 1-component, 16-bit unsigned normalized
	case GL_RG16:											return VK_FORMAT_R16G16_UNORM;				// 2-component, 16-bit unsigned normalized
	case GL_RGB16:											return VK_FORMAT_R16G16B16_UNORM;			// 3-component, 16-bit unsigned normalized
	case GL_RGBA16:											return VK_FORMAT_R16G16B16A16_UNORM;		// 4-component, 16-bit unsigned normalized

	case GL_R16_SNORM:										return VK_FORMAT_R16_SNORM;					// 1-component, 16-bit signed normalized
	case GL_RG16_SNORM:										return VK_FORMAT_R16G16_SNORM;				// 2-component, 16-bit signed normalized
	case GL_RGB16_SNORM:									return VK_FORMAT_R16G16B16_SNORM;			// 3-component, 16-bit signed normalized
	case GL_RGBA16_SNORM:									return VK_FORMAT_R16G16B16A16_SNORM;		// 4-component, 16-bit signed normalized

	case GL_R16UI:											return VK_FORMAT_R16_UINT;					// 1-component, 16-bit unsigned integer
	case GL_RG16UI:											return VK_FORMAT_R16G16_UINT;				// 2-component, 16-bit unsigned integer
	case GL_RGB16UI:										return VK_FORMAT_R16G16B16_UINT;			// 3-component, 16-bit unsigned integer
	case GL_RGBA16UI:										return VK_FORMAT_R16G16B16A16_UINT;			// 4-component, 16-bit unsigned integer

	case GL_R16I:											return VK_FORMAT_R16_SINT;					// 1-component, 16-bit signed integer
	case GL_RG16I:											return VK_FORMAT_R16G16_SINT;				// 2-component, 16-bit signed integer
	case GL_RGB16I:											return VK_FORMAT_R16G16B16_SINT;			// 3-component, 16-bit signed integer
	case GL_RGBA16I:										return VK_FORMAT_R16G16B16A16_SINT;			// 4-component, 16-bit signed integer

	case GL_R16F:											return VK_FORMAT_R16_SFLOAT;				// 1-component, 16-bit floating-point
	case GL_RG16F:											return VK_FORMAT_R16G16_SFLOAT;				// 2-component, 16-bit floating-point
	case GL_RGB16F:											return VK_FORMAT_R16G16B16_SFLOAT;			// 3-component, 16-bit floating-point
	case GL_RGBA16F:										return VK_FORMAT_R16G16B16A16_SFLOAT;		// 4-component, 16-bit floating-point

		//
		// 32 bits per component
		//
	case GL_R32UI:											return VK_FORMAT_R32_UINT;					// 1-component, 32-bit unsigned integer
	case GL_RG32UI:											return VK_FORMAT_R32G32_UINT;				// 2-component, 32-bit unsigned integer
	case GL_RGB32UI:										return VK_FORMAT_R32G32B32_UINT;			// 3-component, 32-bit unsigned integer
	case GL_RGBA32UI:										return VK_FORMAT_R32G32B32A32_UINT;			// 4-component, 32-bit unsigned integer

	case GL_R32I:											return VK_FORMAT_R32_SINT;					// 1-component, 32-bit signed integer
	case GL_RG32I:											return VK_FORMAT_R32G32_SINT;				// 2-component, 32-bit signed integer
	case GL_RGB32I:											return VK_FORMAT_R32G32B32_SINT;			// 3-component, 32-bit signed integer
	case GL_RGBA32I:										return VK_FORMAT_R32G32B32A32_SINT;			// 4-component, 32-bit signed integer

	case GL_R32F:											return VK_FORMAT_R32_SFLOAT;				// 1-component, 32-bit floating-point
	case GL_RG32F:											return VK_FORMAT_R32G32_SFLOAT;				// 2-component, 32-bit floating-point
	case GL_RGB32F:											return VK_FORMAT_R32G32B32_SFLOAT;			// 3-component, 32-bit floating-point
	case GL_RGBA32F:										return VK_FORMAT_R32G32B32A32_SFLOAT;		// 4-component, 32-bit floating-point

		//
		// Packed
		//
	case GL_R3_G3_B2:										return VK_FORMAT_UNDEFINED;					// 3-component 3:3:2,       unsigned normalized
	case GL_RGB4:											return VK_FORMAT_UNDEFINED;					// 3-component 4:4:4,       unsigned normalized
	case GL_RGB5:											return VK_FORMAT_R5G5B5A1_UNORM_PACK16;		// 3-component 5:5:5,       unsigned normalized
	case GL_RGB565:											return VK_FORMAT_R5G6B5_UNORM_PACK16;		// 3-component 5:6:5,       unsigned normalized
	case GL_RGB10:											return VK_FORMAT_A2R10G10B10_UNORM_PACK32;	// 3-component 10:10:10,    unsigned normalized
	case GL_RGB12:											return VK_FORMAT_UNDEFINED;					// 3-component 12:12:12,    unsigned normalized
	case GL_RGBA2:											return VK_FORMAT_UNDEFINED;					// 4-component 2:2:2:2,     unsigned normalized
	case GL_RGBA4:											return VK_FORMAT_R4G4B4A4_UNORM_PACK16;		// 4-component 4:4:4:4,     unsigned normalized
	case GL_RGBA12:											return VK_FORMAT_UNDEFINED;					// 4-component 12:12:12:12, unsigned normalized
	case GL_RGB5_A1:										return VK_FORMAT_A1R5G5B5_UNORM_PACK16;		// 4-component 5:5:5:1,     unsigned normalized
	case GL_RGB10_A2:										return VK_FORMAT_A2R10G10B10_UNORM_PACK32;	// 4-component 10:10:10:2,  unsigned normalized
	case GL_RGB10_A2UI:										return VK_FORMAT_A2R10G10B10_UINT_PACK32;	// 4-component 10:10:10:2,  unsigned integer
	case GL_R11F_G11F_B10F:									return VK_FORMAT_B10G11R11_UFLOAT_PACK32;	// 3-component 11:11:10,    floating-point
	case GL_RGB9_E5:										return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;	// 3-component/exp 9:9:9/5, floating-point

		//
		// S3TC/DXT/BC
		//

	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGB_UNORM_BLOCK;		// line through 3D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;		// line through 3D space plus 1-bit alpha, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:					return VK_FORMAT_BC2_UNORM_BLOCK;			// line through 3D space plus line through 1D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:					return VK_FORMAT_BC3_UNORM_BLOCK;			// line through 3D space plus 4-bit alpha, 4x4 blocks, unsigned normalized

	case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:					return VK_FORMAT_BC1_RGB_SRGB_BLOCK;		// line through 3D space, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;		// line through 3D space plus 1-bit alpha, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:			return VK_FORMAT_BC2_SRGB_BLOCK;			// line through 3D space plus line through 1D space, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:			return VK_FORMAT_BC3_SRGB_BLOCK;			// line through 3D space plus 4-bit alpha, 4x4 blocks, sRGB

	case GL_COMPRESSED_LUMINANCE_LATC1_EXT:					return VK_FORMAT_BC4_UNORM_BLOCK;			// line through 1D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:			return VK_FORMAT_BC5_UNORM_BLOCK;			// two lines through 1D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:			return VK_FORMAT_BC4_SNORM_BLOCK;			// line through 1D space, 4x4 blocks, signed normalized
	case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:	return VK_FORMAT_BC5_SNORM_BLOCK;			// two lines through 1D space, 4x4 blocks, signed normalized

	case GL_COMPRESSED_RED_RGTC1:							return VK_FORMAT_BC4_UNORM_BLOCK;			// line through 1D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RG_RGTC2:							return VK_FORMAT_BC5_UNORM_BLOCK;			// two lines through 1D space, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_SIGNED_RED_RGTC1:					return VK_FORMAT_BC4_SNORM_BLOCK;			// line through 1D space, 4x4 blocks, signed normalized
	case GL_COMPRESSED_SIGNED_RG_RGTC2:						return VK_FORMAT_BC5_SNORM_BLOCK;			// two lines through 1D space, 4x4 blocks, signed normalized

	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:				return VK_FORMAT_BC6H_UFLOAT_BLOCK;			// 3-component, 4x4 blocks, unsigned floating-point
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:				return VK_FORMAT_BC6H_SFLOAT_BLOCK;			// 3-component, 4x4 blocks, signed floating-point
	case GL_COMPRESSED_RGBA_BPTC_UNORM:						return VK_FORMAT_BC7_UNORM_BLOCK;			// 4-component, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:				return VK_FORMAT_BC7_SRGB_BLOCK;			// 4-component, 4x4 blocks, sRGB

		//
		// ETC
		//
	case GL_ETC1_RGB8_OES:									return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;	// 3-component ETC1, 4x4 blocks, unsigned normalized

	case GL_COMPRESSED_RGB8_ETC2:							return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;	// 3-component ETC2, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;	// 4-component ETC2 with 1-bit alpha, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA8_ETC2_EAC:						return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;	// 4-component ETC2, 4x4 blocks, unsigned normalized

	case GL_COMPRESSED_SRGB8_ETC2:							return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;	// 3-component ETC2, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:		return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;	// 4-component ETC2 with 1-bit alpha, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:				return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;	// 4-component ETC2, 4x4 blocks, sRGB

	case GL_COMPRESSED_R11_EAC:								return VK_FORMAT_EAC_R11_UNORM_BLOCK;		// 1-component ETC, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RG11_EAC:							return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;	// 2-component ETC, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_SIGNED_R11_EAC:						return VK_FORMAT_EAC_R11_SNORM_BLOCK;		// 1-component ETC, 4x4 blocks, signed normalized
	case GL_COMPRESSED_SIGNED_RG11_EAC:						return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;	// 2-component ETC, 4x4 blocks, signed normalized

		//
		// PVRTC
		//
	case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:				return VK_FORMAT_UNDEFINED;					// 3-component PVRTC, 16x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:				return VK_FORMAT_UNDEFINED;					// 3-component PVRTC,  8x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:				return VK_FORMAT_UNDEFINED;					// 4-component PVRTC, 16x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:				return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  8x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:				return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  8x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:				return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  4x4 blocks, unsigned normalized

	case GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT:				return VK_FORMAT_UNDEFINED;					// 3-component PVRTC, 16x8 blocks, sRGB
	case GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT:				return VK_FORMAT_UNDEFINED;					// 3-component PVRTC,  8x8 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT:			return VK_FORMAT_UNDEFINED;					// 4-component PVRTC, 16x8 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT:			return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  8x8 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG:			return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  8x4 blocks, sRGB
	case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG:			return VK_FORMAT_UNDEFINED;					// 4-component PVRTC,  4x4 blocks, sRGB

		//
		// ASTC
		//
	case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:					return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;		// 4-component ASTC, 4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:					return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;		// 4-component ASTC, 5x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:					return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;		// 4-component ASTC, 5x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:					return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;		// 4-component ASTC, 6x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:					return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;		// 4-component ASTC, 6x6 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:					return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;		// 4-component ASTC, 8x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:					return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;		// 4-component ASTC, 8x6 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:					return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;		// 4-component ASTC, 8x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:					return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;		// 4-component ASTC, 10x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:					return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;		// 4-component ASTC, 10x6 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:					return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;		// 4-component ASTC, 10x8 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:					return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;	// 4-component ASTC, 10x10 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:					return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;	// 4-component ASTC, 12x10 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:					return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;	// 4-component ASTC, 12x12 blocks, unsigned normalized

	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:			return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;		// 4-component ASTC, 4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:			return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;		// 4-component ASTC, 5x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:			return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;		// 4-component ASTC, 5x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:			return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;		// 4-component ASTC, 6x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:			return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;		// 4-component ASTC, 6x6 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:			return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;		// 4-component ASTC, 8x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:			return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;		// 4-component ASTC, 8x6 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:			return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;		// 4-component ASTC, 8x8 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:			return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;		// 4-component ASTC, 10x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:			return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;		// 4-component ASTC, 10x6 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:			return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;		// 4-component ASTC, 10x8 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:			return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;		// 4-component ASTC, 10x10 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:			return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;		// 4-component ASTC, 12x10 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:			return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;		// 4-component ASTC, 12x12 blocks, sRGB

	case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 3x3x3 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x3x3 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x3 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x4x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x4 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x5x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x5 blocks, unsigned normalized
	case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES:					return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x6 blocks, unsigned normalized

	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 3x3x3 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x3x3 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x3 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 4x4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x4x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x4 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 5x5x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x5x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x5 blocks, sRGB
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES:			return VK_FORMAT_UNDEFINED;					// 4-component ASTC, 6x6x6 blocks, sRGB

		//
		// ATC
		//
	case GL_ATC_RGB_AMD:									return VK_FORMAT_UNDEFINED;					// 3-component, 4x4 blocks, unsigned normalized
	case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:					return VK_FORMAT_UNDEFINED;					// 4-component, 4x4 blocks, unsigned normalized
	case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:				return VK_FORMAT_UNDEFINED;					// 4-component, 4x4 blocks, unsigned normalized

		//
		// Palletized
		//
	case GL_PALETTE4_RGB8_OES:								return VK_FORMAT_UNDEFINED;					// 3-component 8:8:8,   4-bit palette, unsigned normalized
	case GL_PALETTE4_RGBA8_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 8:8:8:8, 4-bit palette, unsigned normalized
	case GL_PALETTE4_R5_G6_B5_OES:							return VK_FORMAT_UNDEFINED;					// 3-component 5:6:5,   4-bit palette, unsigned normalized
	case GL_PALETTE4_RGBA4_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 4:4:4:4, 4-bit palette, unsigned normalized
	case GL_PALETTE4_RGB5_A1_OES:							return VK_FORMAT_UNDEFINED;					// 4-component 5:5:5:1, 4-bit palette, unsigned normalized
	case GL_PALETTE8_RGB8_OES:								return VK_FORMAT_UNDEFINED;					// 3-component 8:8:8,   8-bit palette, unsigned normalized
	case GL_PALETTE8_RGBA8_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 8:8:8:8, 8-bit palette, unsigned normalized
	case GL_PALETTE8_R5_G6_B5_OES:							return VK_FORMAT_UNDEFINED;					// 3-component 5:6:5,   8-bit palette, unsigned normalized
	case GL_PALETTE8_RGBA4_OES:								return VK_FORMAT_UNDEFINED;					// 4-component 4:4:4:4, 8-bit palette, unsigned normalized
	case GL_PALETTE8_RGB5_A1_OES:							return VK_FORMAT_UNDEFINED;					// 4-component 5:5:5:1, 8-bit palette, unsigned normalized

		//
		// Depth/stencil
		//
	case GL_DEPTH_COMPONENT16:								return VK_FORMAT_D16_UNORM;
	case GL_DEPTH_COMPONENT24:								return VK_FORMAT_X8_D24_UNORM_PACK32;
	case GL_DEPTH_COMPONENT32:								return VK_FORMAT_UNDEFINED;
	case GL_DEPTH_COMPONENT32F:								return VK_FORMAT_D32_SFLOAT;
	case GL_DEPTH_COMPONENT32F_NV:							return VK_FORMAT_D32_SFLOAT;
	case GL_STENCIL_INDEX1:									return VK_FORMAT_UNDEFINED;
	case GL_STENCIL_INDEX4:									return VK_FORMAT_UNDEFINED;
	case GL_STENCIL_INDEX8:									return VK_FORMAT_S8_UINT;
	case GL_STENCIL_INDEX16:								return VK_FORMAT_UNDEFINED;
	case GL_DEPTH24_STENCIL8:								return VK_FORMAT_D24_UNORM_S8_UINT;
	case GL_DEPTH32F_STENCIL8:								return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case GL_DEPTH32F_STENCIL8_NV:							return VK_FORMAT_D32_SFLOAT_S8_UINT;

	default:												return VK_FORMAT_UNDEFINED;
	}
}

typedef enum VkFormatSizeFlagBits {
	VK_FORMAT_SIZE_PACKED_BIT				= 0x00000001,
	VK_FORMAT_SIZE_COMPRESSED_BIT			= 0x00000002,
	VK_FORMAT_SIZE_PALETTIZED_BIT			= 0x00000004,
	VK_FORMAT_SIZE_DEPTH_BIT				= 0x00000008,
	VK_FORMAT_SIZE_STENCIL_BIT				= 0x00000010,
} VkFormatSizeFlagBits;

typedef VkFlags VkFormatSizeFlags;

typedef struct VkFormatSize {
	VkFormatSizeFlags	flags;
	unsigned int		paletteSizeInBits;
	unsigned int		blockSizeInBits;
	unsigned int		blockWidth;			// in texels
	unsigned int		blockHeight;		// in texels
	unsigned int		blockDepth;			// in texels
} VkFormatSize;

static inline void vkGetFormatSize( const VkFormat format, VkFormatSize * pFormatSize )
{
	switch ( format )
	{
	case VK_FORMAT_R4G4_UNORM_PACK8:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 1 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
	case VK_FORMAT_B5G6R5_UNORM_PACK16:
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 2 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 1 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 2 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 3 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SNORM:
	case VK_FORMAT_R8G8B8A8_USCALED:
	case VK_FORMAT_R8G8B8A8_SSCALED:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_USCALED:
	case VK_FORMAT_B8G8R8A8_SSCALED:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R16_UNORM:
	case VK_FORMAT_R16_SNORM:
	case VK_FORMAT_R16_USCALED:
	case VK_FORMAT_R16_SSCALED:
	case VK_FORMAT_R16_UINT:
	case VK_FORMAT_R16_SINT:
	case VK_FORMAT_R16_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 2 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R16G16_UNORM:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_R16G16_USCALED:
	case VK_FORMAT_R16G16_SSCALED:
	case VK_FORMAT_R16G16_UINT:
	case VK_FORMAT_R16G16_SINT:
	case VK_FORMAT_R16G16_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R16G16B16_UNORM:
	case VK_FORMAT_R16G16B16_SNORM:
	case VK_FORMAT_R16G16B16_USCALED:
	case VK_FORMAT_R16G16B16_SSCALED:
	case VK_FORMAT_R16G16B16_UINT:
	case VK_FORMAT_R16G16B16_SINT:
	case VK_FORMAT_R16G16B16_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 6 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R16G16B16A16_UNORM:
	case VK_FORMAT_R16G16B16A16_SNORM:
	case VK_FORMAT_R16G16B16A16_USCALED:
	case VK_FORMAT_R16G16B16A16_SSCALED:
	case VK_FORMAT_R16G16B16A16_UINT:
	case VK_FORMAT_R16G16B16A16_SINT:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R32_UINT:
	case VK_FORMAT_R32_SINT:
	case VK_FORMAT_R32_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R32G32_UINT:
	case VK_FORMAT_R32G32_SINT:
	case VK_FORMAT_R32G32_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R32G32B32_UINT:
	case VK_FORMAT_R32G32B32_SINT:
	case VK_FORMAT_R32G32B32_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 12 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_R32G32B32A32_SINT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R64_UINT:
	case VK_FORMAT_R64_SINT:
	case VK_FORMAT_R64_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R64G64_UINT:
	case VK_FORMAT_R64G64_SINT:
	case VK_FORMAT_R64G64_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R64G64B64_UINT:
	case VK_FORMAT_R64G64B64_SINT:
	case VK_FORMAT_R64G64B64_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 24 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_R64G64B64A64_UINT:
	case VK_FORMAT_R64G64B64A64_SINT:
	case VK_FORMAT_R64G64B64A64_SFLOAT:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 32 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_D16_UNORM:
		pFormatSize->flags = VK_FORMAT_SIZE_DEPTH_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 2 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_X8_D24_UNORM_PACK32:
		pFormatSize->flags = VK_FORMAT_SIZE_PACKED_BIT | VK_FORMAT_SIZE_DEPTH_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_D32_SFLOAT:
		pFormatSize->flags = VK_FORMAT_SIZE_DEPTH_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_S8_UINT:
		pFormatSize->flags = VK_FORMAT_SIZE_STENCIL_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 1 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_D16_UNORM_S8_UINT:
		pFormatSize->flags = VK_FORMAT_SIZE_DEPTH_BIT | VK_FORMAT_SIZE_STENCIL_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 3 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		pFormatSize->flags = VK_FORMAT_SIZE_DEPTH_BIT | VK_FORMAT_SIZE_STENCIL_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 4 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		pFormatSize->flags = VK_FORMAT_SIZE_DEPTH_BIT | VK_FORMAT_SIZE_STENCIL_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 4;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_BC2_UNORM_BLOCK:
	case VK_FORMAT_BC2_SRGB_BLOCK:
	case VK_FORMAT_BC3_UNORM_BLOCK:
	case VK_FORMAT_BC3_SRGB_BLOCK:
	case VK_FORMAT_BC4_UNORM_BLOCK:
	case VK_FORMAT_BC4_SNORM_BLOCK:
	case VK_FORMAT_BC5_UNORM_BLOCK:
	case VK_FORMAT_BC5_SNORM_BLOCK:
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
	case VK_FORMAT_BC7_UNORM_BLOCK:
	case VK_FORMAT_BC7_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 4;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 8 * 8;
		pFormatSize->blockWidth = 4;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 4;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 4;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 5;
		pFormatSize->blockHeight = 4;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 5;
		pFormatSize->blockHeight = 5;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 6;
		pFormatSize->blockHeight = 5;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 6;
		pFormatSize->blockHeight = 6;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 8;
		pFormatSize->blockHeight = 5;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 8;
		pFormatSize->blockHeight = 6;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 8;
		pFormatSize->blockHeight = 8;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 10;
		pFormatSize->blockHeight = 5;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 10;
		pFormatSize->blockHeight = 6;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 10;
		pFormatSize->blockHeight = 8;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 10;
		pFormatSize->blockHeight = 10;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 12;
		pFormatSize->blockHeight = 10;
		pFormatSize->blockDepth = 1;
		break;
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
		pFormatSize->flags = VK_FORMAT_SIZE_COMPRESSED_BIT;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 16 * 8;
		pFormatSize->blockWidth = 12;
		pFormatSize->blockHeight = 12;
		pFormatSize->blockDepth = 1;
		break;
	default:
		pFormatSize->flags = 0;
		pFormatSize->paletteSizeInBits = 0;
		pFormatSize->blockSizeInBits = 0 * 8;
		pFormatSize->blockWidth = 1;
		pFormatSize->blockHeight = 1;
		pFormatSize->blockDepth = 1;
		break;
	}
}
#endif