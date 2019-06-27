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

	if (memcmp(&ctx->header.identifier, fileIdentifier, 12) == 0) {
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

	// read mipmap level 0 size now as
	ctx->callbacks.read(ctx->user, ctx->mipMapSizes, sizeof(uint32_t));

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
		curKey = curKey + ((curKey->size + 3u) & 4u);
	}
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
uint32_t TinyKtx_Width(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false)
		return false;

	return ctx->header.pixelWidth;
}

uint32_t TinyKtx_Height(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.pixelHeight;
}

uint32_t TinyKtx_Depth(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.pixelDepth;
}

uint32_t TinyKtx_ArraySlices(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.numberOfArrayElements;
}

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle) {
	TinyKtx_Context *ctx = (TinyKtx_Context *) handle;
	if (ctx == NULL)
		return false;
	if (ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
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
bool TinyKtx_CrackFormatToGL(TinyKTX_Format format, uint32_t *glformat, uint32_t* gltype) {
	switch (format) {
	case R4G4_UNORM_PACK8:;
	case R4G4B4A4_UNORM_PACK16: break;
	case B4G4R4A4_UNORM_PACK16: break;
	case R5G6B5_UNORM_PACK16: break;
	case B5G6R5_UNORM_PACK16: break;
	case R5G5B5A1_UNORM_PACK16: break;
	case B5G5R5A1_UNORM_PACK16: break;
	case A1R5G5B5_UNORM_PACK16: break;

	case A2R10G10B10_UNORM_PACK32: break;
	case A2R10G10B10_UINT_PACK32: break;
	case A2B10G10R10_UNORM_PACK32: break;
	case A2B10G10R10_UINT_PACK32: break;

	case R8_UNORM: 		*glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8_SNORM: 		*glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_BYTE; return true;
	case R8_UINT: 		*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8_SINT: 		*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case R8_SRGB: 		break; //*glformat = TINYKTX_GL_SR8; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case R8G8_UNORM: 	*glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8_SNORM: 	*glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8_UINT: 	*glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8_SINT: 	*glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8_SRGB: 	break; //*glformat = TINYKTX_GL_SRG8; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case R8G8B8_UNORM: *glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8B8_SNORM: *glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8B8_UINT:  *glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8B8_SINT:  *glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8B8_SRGB:  *glformat = GL_SRGB; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case B8G8R8_UNORM: *glformat = TINYKTX_GL_BGR; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case B8G8R8_SNORM: *glformat = TINYKTX_GL_BGR; *gltype = TINYKTX_GL_BYTE; return true;
	case B8G8R8_UINT:  *glformat = TINYKTX_GL_BGR_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case B8G8R8_SINT:  *glformat = TINYKTX_GL_BGR_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case B8G8R8_SRGB:  break;//*glformat = TINYKTX_GL_SBGR8; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case R8G8B8A8_UNORM:*glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8B8A8_SNORM:*glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8B8A8_UINT: *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case R8G8B8A8_SINT: *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case R8G8B8A8_SRGB: *glformat = GL_SRGB_ALPHA; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case B8G8R8A8_UNORM:*glformat = TINYKTX_GL_BGRA; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case B8G8R8A8_SNORM:*glformat = TINYKTX_GL_BGRA; *gltype = TINYKTX_GL_BYTE; return true;
	case B8G8R8A8_UINT: *glformat = TINYKTX_GL_BGRA_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case B8G8R8A8_SINT: *glformat = TINYKTX_GL_BGRA_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case B8G8R8A8_SRGB: break; //*glformat = TINYKTX_GL_SBGRA8; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case A8B8G8R8_UNORM_PACK32: break;//*glformat = TINYKTX_GL_ARGB; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case A8B8G8R8_SNORM_PACK32: break;//*glformat = TINYKTX_GL_ARGB; *gltype = TINYKTX_GL_BYTE; return true;
	case A8B8G8R8_UINT_PACK32: break;//*glformat = TINYKTX_GL_ARGB_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;
	case A8B8G8R8_SINT_PACK32: break;//*glformat = TINYKTX_GL_ARGBA_INTEGER; *gltype = TINYKTX_GL_BYTE; return true;
	case A8B8G8R8_SRGB_PACK32: break;//*glformat = TINYKTX_GL_SARGB8; *gltype = TINYKTX_GL_UNSIGNED_BYTE; return true;

	case R16_UNORM: *glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16_SNORM: *glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_SHORT; return true;
	case R16_UINT: 	*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16_SINT: 	*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_SHORT; return true;
	case R16_SFLOAT:*glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_HALF_FLOAT; return true;

	case R16G16_UNORM: *glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16_SNORM: *glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16_UINT:  *glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16_SINT:  *glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16_SFLOAT:*glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_HALF_FLOAT; return true;

	case R16G16B16_UNORM: *glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16B16_SNORM: *glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16B16_UINT:  *glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16B16_SINT:  *glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16B16_SFLOAT:*glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_HALF_FLOAT; return true;

	case R16G16B16A16_UNORM: *glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16B16A16_SNORM: *glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16B16A16_UINT:  *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_SHORT; return true;
	case R16G16B16A16_SINT:  *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_SHORT; return true;
	case R16G16B16A16_SFLOAT:*glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_HALF_FLOAT; return true;

	case R32_UINT: 		*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_INT; return true;
	case R32_SINT: 		*glformat = TINYKTX_GL_RED_INTEGER; *gltype = TINYKTX_GL_INT; return true;
	case R32_SFLOAT: 	*glformat = TINYKTX_GL_RED; *gltype = TINYKTX_GL_FLOAT; return true;

	case R32G32_UINT: 	*glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_INT; return true;
	case R32G32_SINT: 	*glformat = TINYKTX_GL_RG_INTEGER; *gltype = TINYKTX_GL_INT; return true;
	case R32G32_SFLOAT: *glformat = TINYKTX_GL_RG; *gltype = TINYKTX_GL_FLOAT; return true;

	case R32G32B32_UINT: 		*glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_INT; return true;
	case R32G32B32_SINT: 		*glformat = TINYKTX_GL_RGB_INTEGER; *gltype = TINYKTX_GL_INT; return true;
	case R32G32B32_SFLOAT: 	*glformat = TINYKTX_GL_RGB; *gltype = TINYKTX_GL_FLOAT; return true;

	case R32G32B32A32_UINT:  *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_UNSIGNED_INT; return true;
	case R32G32B32A32_SINT:  *glformat = TINYKTX_GL_RGBA_INTEGER; *gltype = TINYKTX_GL_INT; return true;
	case R32G32B32A32_SFLOAT:*glformat = TINYKTX_GL_RGBA; *gltype = TINYKTX_GL_FLOAT; return true;


	case B10G11R11_UFLOAT_PACK32: break;
	case E5B9G9R9_UFLOAT_PACK32: break;
	case D16_UNORM: break;
	case X8_D24_UNORM_PACK32: break;
	case D32_SFLOAT: break;
	case S8_UINT: break;
	case D16_UNORM_S8_UINT: break;
	case D24_UNORM_S8_UINT: break;
	case D32_SFLOAT_S8_UINT: break;
	case BC1_RGB_UNORM_BLOCK: break;
	case BC1_RGB_SRGB_BLOCK: break;
	case BC1_RGBA_UNORM_BLOCK: break;
	case BC1_RGBA_SRGB_BLOCK: break;
	case BC2_UNORM_BLOCK: break;
	case BC2_SRGB_BLOCK: break;
	case BC3_UNORM_BLOCK: break;
	case BC3_SRGB_BLOCK: break;
	case BC4_UNORM_BLOCK: break;
	case BC4_SNORM_BLOCK: break;
	case BC5_UNORM_BLOCK: break;
	case BC5_SNORM_BLOCK: break;
	case BC6H_UFLOAT_BLOCK: break;
	case BC6H_SFLOAT_BLOCK: break;
	case BC7_UNORM_BLOCK: break;
	case BC7_SRGB_BLOCK: break;
	case PVR_2BPP_BLOCK: break;
	case PVR_2BPPA_BLOCK: break;
	case PVR_4BPP_BLOCK: break;
	case PVR_4BPPA_BLOCK: break;
	case PVR_2BPP_SRGB_BLOCK: break;
	case PVR_2BPPA_SRGB_BLOCK: break;
	case PVR_4BPP_SRGB_BLOCK: break;
	case PVR_4BPPA_SRGB_BLOCK: break;
	case ASTC_4x4_UNORM_BLOCK: break;
	case ASTC_4x4_SRGB_BLOCK: break;
	case ASTC_5x4_UNORM_BLOCK: break;
	case ASTC_5x4_SRGB_BLOCK: break;
	case ASTC_5x5_UNORM_BLOCK: break;
	case ASTC_5x5_SRGB_BLOCK: break;
	case ASTC_6x5_UNORM_BLOCK: break;
	case ASTC_6x5_SRGB_BLOCK: break;
	case ASTC_6x6_UNORM_BLOCK: break;
	case ASTC_6x6_SRGB_BLOCK: break;
	case ASTC_8x5_UNORM_BLOCK: break;
	case ASTC_8x5_SRGB_BLOCK: break;
	case ASTC_8x6_UNORM_BLOCK: break;
	case ASTC_8x6_SRGB_BLOCK: break;
	case ASTC_8x8_UNORM_BLOCK: break;
	case ASTC_8x8_SRGB_BLOCK: break;
	case ASTC_10x5_UNORM_BLOCK: break;
	case ASTC_10x5_SRGB_BLOCK: break;
	case ASTC_10x6_UNORM_BLOCK: break;
	case ASTC_10x6_SRGB_BLOCK: break;
	case ASTC_10x8_UNORM_BLOCK: break;
	case ASTC_10x8_SRGB_BLOCK: break;
	case ASTC_10x10_UNORM_BLOCK: break;
	case ASTC_10x10_SRGB_BLOCK: break;
	case ASTC_12x10_UNORM_BLOCK: break;
	case ASTC_12x10_SRGB_BLOCK: break;
	case ASTC_12x12_UNORM_BLOCK: break;
	case ASTC_12x12_SRGB_BLOCK: break;

	// TODO 64 bit formats
	case R64_UINT: 	break;
	case R64_SINT: 	break;
	case R64_SFLOAT: break;

	case R64G64_UINT: break;
	case R64G64_SINT: break;
	case R64G64_SFLOAT: break;

	case R64G64B64_UINT: break;
	case R64G64B64_SINT: break;
	case R64G64B64_SFLOAT: break;

	case R64G64B64A64_UINT: break;
	case R64G64B64A64_SINT: break;
	case R64G64B64A64_SFLOAT: break;

	// not sure how to expres scaled yet in GL terms?
	case R8_USCALED: break;
	case R8_SSCALED: break;
	case R8G8_USCALED: break;
	case R8G8_SSCALED: break;
	case R8G8B8_USCALED: break;
	case R8G8B8_SSCALED: break;
	case B8G8R8_USCALED: break;
	case B8G8R8_SSCALED: break;
	case R8G8B8A8_USCALED: break;
	case R8G8B8A8_SSCALED: break;
	case B8G8R8A8_USCALED: break;
	case B8G8R8A8_SSCALED: break;
	case A8B8G8R8_USCALED_PACK32: break;
	case A8B8G8R8_SSCALED_PACK32: break;
	case A2R10G10B10_USCALED_PACK32: break;
	case A2B10G10R10_USCALED_PACK32: break;
	case R16_USCALED: break;
	case R16_SSCALED: break;
	case R16G16_USCALED: break;
	case R16G16_SSCALED: break;
	case R16G16B16_USCALED: break;
	case R16G16B16_SSCALED: break;
	case R16G16B16A16_USCALED: break;
	case R16G16B16A16_SSCALED: break;

	}
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

	uint64_t currentOffset = ctx->headerPos + sizeof(TinyKtx_Header) + ctx->header.bytesOfKeyValueData;
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
			ctx->callbacks.read(ctx->user, &size, sizeof(uint32_t));

			if (ctx->header.numberOfFaces == 6 && ctx->header.numberOfArrayElements == 0) {
				size = ((size + 3u) & 4u) * 6; // face padding and 6 faces
			}

			ctx->mipMapSizes[i] = size;
		}
		currentOffset += (size + 3u) & 4u; // mip padding
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
	for (uint32_t i = 0u; i < mipmapsizes; ++i) {
		callbacks->write(user, mipmapsizes + i, sizeof(uint32_t));
		callbacks->write(user, mipmaps + i, mipmapsizes[i]);
		callbacks->write(user, padding, ((mipmapsizes[i] + 3u) & 4u) - mipmapsizes[i]);
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