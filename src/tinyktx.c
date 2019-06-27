#include "tiny_ktx/tinyktx.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MIPMAPLEVELS 16

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
	void* user;
	uint64_t headerPos;
	TinyKtx_Header header;
	TinyKtx_KeyValuePair const* keyData;
	bool headerValid;
	bool sameEndian;

	uint32_t mipMapSizes[MAX_MIPMAPLEVELS];
	uint8_t const* mipmaps[MAX_MIPMAPLEVELS];

} TinyKtx_Context;

static void DefaultErrorFunc(void* user, char const* msg) {}

TinyKtx_ContextHandle TinyKtx_CreateContext(TinyKtx_Callbacks const* callbacks, void* user) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) callbacks->alloc(user, sizeof(TinyKtx_Context));
	if(ctx == NULL) return NULL;

	memcpy(&ctx->callbacks, callbacks, sizeof(TinyKtx_Callbacks));
	ctx->user = user;
	if(ctx->callbacks.error == NULL) { ctx->callbacks.error = &DefaultErrorFunc; }

	if(ctx->callbacks.read == NULL) { DefaultErrorFunc(user, "TinyKtx must have read callback"); return NULL; }
	if(ctx->callbacks.alloc == NULL) { DefaultErrorFunc(user, "TinyKtx must have alloc callback"); return NULL; }
	if(ctx->callbacks.free == NULL) { DefaultErrorFunc(user, "TinyKtx must have free callback"); return NULL; }
	if(ctx->callbacks.seek == NULL) { DefaultErrorFunc(user, "TinyKtx must have seek callback"); return NULL; }
	if(ctx->callbacks.tell == NULL) { DefaultErrorFunc(user, "TinyKtx must have tell callback"); return NULL; }

	TinyKtx_Reset(ctx);

	return ctx;
}

void TinyKtx_DestroyContext(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return;
	ctx->callbacks.free(ctx->user, ctx);
}

void TinyKtx_BeginRead(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return;

	ctx->headerPos = ctx->callbacks.tell(ctx->user);
}

void TinyKtx_Reset(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return;

	// backup user provided callbacks and data
	TinyKtx_Callbacks callbacks;
	memcpy(&callbacks, &ctx->callbacks, sizeof(TinyKtx_Callbacks));
	void* user = ctx->user;

	// free memory of sub data
	if(ctx->keyData != NULL) {
		ctx->callbacks.free(ctx->user, (void*)ctx->keyData);
	}
	for(int i = 0; i < MAX_MIPMAPLEVELS;++i) {
		if(ctx->mipmaps[i] != NULL) {
			ctx->callbacks.free(ctx->user, (void*)ctx->mipmaps[i]);
		}
	}

	// reset to default state
	memset(ctx, 0, sizeof(TinyKtx_Context));
	memcpy(&ctx->callbacks, &callbacks, sizeof(TinyKtx_Callbacks));
	ctx->user = user;

}

bool TinyKtx_ReadHeader(TinyKtx_ContextHandle handle) {
	static uint8_t fileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
	};
	static uint32_t const sameEndianDecider  			= 0x04030201;
	static uint32_t const differentEndianDecider  = 0x01020304;

	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;

	ctx->callbacks.seek(ctx->user, ctx->headerPos);
	ctx->callbacks.read(ctx->user, &ctx->header, sizeof(TinyKtx_Header));

	if( memcmp(&ctx->header.identifier, fileIdentifier, 12) == 0) {
		ctx->callbacks.error(ctx->user, "Not a KTX file or corrupted as identified isn't valid");
		return false;
	}


	if( ctx->header.endianness == sameEndianDecider) {
		ctx->sameEndian = true;
	} else 	if(ctx->header.endianness == differentEndianDecider) {
		ctx->sameEndian = false;
	} else {
		// corrupt or mid endian?
		ctx->callbacks.error(ctx->user, "Endian Error");
		return false;
	}

	if(ctx->header.numberOfFaces != 1 && ctx->header.numberOfFaces != 6) {
		ctx->callbacks.error(ctx->user, "no. of Faces must be 1 or 6");
		return false;
	}

	ctx->keyData = (TinyKtx_KeyValuePair const *) ctx->callbacks.alloc(ctx->user, ctx->header.bytesOfKeyValueData);
	ctx->callbacks.read(ctx->user, (void*)ctx->keyData, ctx->header.bytesOfKeyValueData);

	// read mipmap level 0 size now as
	ctx->callbacks.read(ctx->user, ctx->mipMapSizes, sizeof(uint32_t));

	ctx->headerValid = true;
	return true;
}
bool TinyKtx_GetValue(TinyKtx_ContextHandle handle, char const* key, void const** value) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	if(ctx->keyData == NULL) {
		ctx->callbacks.error(ctx->user, "No key value data in this KTX");
		return false;
	}

	TinyKtx_KeyValuePair const* curKey = ctx->keyData;
	while(((uint8_t*)curKey - (uint8_t*)ctx->keyData) < ctx->header.bytesOfKeyValueData) {
		char const* kvp = (char const*) curKey;

		if (strcmp(kvp, key) == 0) {
			size_t sl = strlen(kvp);
			*value = (void const *) (kvp + sl);
			return true;
		}
		curKey = curKey + ((curKey->size + 3u) & 4u);
	}
}

bool TinyKtx_Is1D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight == 1 && ctx->header.pixelDepth == 1);
}
bool TinyKtx_Is2D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight != 1 && ctx->header.pixelDepth == 1);
}
bool TinyKtx_Is3D(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.pixelHeight != 1 && ctx->header.pixelDepth != 1);
}

bool TinyKtx_IsCubemap(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfFaces == 6);
}
bool TinyKtx_IsArray(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return (ctx->header.numberOfArrayElements > 1);
}
uint32_t TinyKtx_Width(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) return false;

	return ctx->header.pixelWidth;
}

uint32_t TinyKtx_Height(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.pixelHeight;
}

uint32_t TinyKtx_Depth(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.pixelDepth;
}

uint32_t TinyKtx_ArraySlices(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.numberOfArrayElements;
}

uint32_t TinyKtx_NumberOfMipmaps(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.numberOfMipmapLevels ? ctx->header.numberOfMipmapLevels : 1;
}

bool TinyKtx_NeedsGenerationOfMipmaps(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->header.numberOfMipmapLevels == 0;
}

bool TinyKtx_NeedsEndianCorrecting(TinyKtx_ContextHandle handle) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return false;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return false;
	}

	return ctx->sameEndian == false;
}

static uint32_t imageSize(TinyKtx_ContextHandle handle, uint32_t mipmaplevel, bool seekLast) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return 0;
	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return 0;
	}
	if(mipmaplevel >= ctx->header.numberOfMipmapLevels) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}
	if(mipmaplevel >= MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return 0;
	}
	if(ctx->mipMapSizes[mipmaplevel] != 0) return ctx->mipMapSizes[mipmaplevel];

	uint64_t currentOffset = ctx->headerPos + sizeof(TinyKtx_Header) + ctx->header.bytesOfKeyValueData;
	for(uint32_t i = 0; i <= mipmaplevel; ++i) {
		uint32_t size;
		// if we have already read this mipmaps size use it
		if(ctx->mipMapSizes[i] != 0) {
			size = ctx->mipMapSizes[i];
			if(seekLast && i == mipmaplevel) {
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

void const* TinyKtx_ImageRawData(TinyKtx_ContextHandle handle, uint32_t mipmaplevel) {
	TinyKtx_Context* ctx = (TinyKtx_Context*) handle;
	if(ctx == NULL) return NULL;

	if(ctx->headerValid == false) {
		ctx->callbacks.error(ctx->user, "Header data hasn't been read yet or its invalid");
		return NULL;
	}

	if(mipmaplevel >= ctx->header.numberOfMipmapLevels) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if(mipmaplevel >= MAX_MIPMAPLEVELS) {
		ctx->callbacks.error(ctx->user, "Invalid mipmap level");
		return NULL;
	}

	if(ctx->mipmaps[mipmaplevel] != NULL) return ctx->mipmaps[mipmaplevel];

	uint32_t size = imageSize(handle, mipmaplevel, true);
	if(size == 0) return NULL;

	ctx->mipmaps[mipmaplevel] = ctx->callbacks.alloc(ctx->user, size);
	if(ctx->mipmaps[mipmaplevel]) {
		ctx->callbacks.read(ctx->user, (void *) ctx->mipmaps[mipmaplevel], size);
	}

	return ctx->mipmaps[mipmaplevel];
}

#ifdef __cplusplus
};
#endif