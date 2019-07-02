# tiny_ktx
Small C based KTX texture loader (inspired by syoyo tiny libraries)

KTX textures can handle
* Almost any format of texture data
* 1D, 2D, 3D and cubemaps textures
* Texture arrays
* Mipmaps
* Key value pairs for custom data extensions

Its an efficient open format for almost every realtime texture data you could want.

## What it does?
* Loads Khronos KTX textures
* Saves Khronos KTX textures
* Optionally provide format in either native GL/KTX style or Vulkan/Dx12/Metal style
* Optionally provides GL defines requires to read KTX files without GL

tiny_ktx is a very low level API as such it only handles these parts, it doesn't process the data in any way

## Requirements
None except a C compiler (TODO test which version 89 or 99)

By default uses 4 std lib headers
* stdint.h -for uint32_t and int64_t
* stdbool.h - for bool
* stddef.h - for size_t
* string.h - for memcpy

However if the types/functions are provided you can opt out
of these being included via *TINYKTX_HAVE_UINTXX_T* etc.

## How to build
* include tinyktx.h from include/tiny_ktx in your project
* in 1 file in your project define *TINYKTX_IMPLEMENTATION* before tinyktx.h

if using cmake and want a library version just add this using add_subdirectory
and add tiny_ktx as a dependency

## Handling KTX format
KTX file are based on OpenGL which has evolved a fairly complex pixel format system

Whilst this may be useful if you are importing into a GL application for other its quite hard to convert.

An optional part of TinyKtx (default is on) will convert these into a more Vulkan/Dx12/Metal format.

Rather than the multiple uint32_t types KTX stores format in, TinyKtx provides a single large enum.

*TinyKtx_GetFormatGL* will give you the format directly as stored in the KTX file

*TinyKtx_GetFormat* will provide a single value converted from the KTX GL provided type.

if TinyKtx_GetFormat can't convert it will return *TKTX_UNDEFINED*

## How to load a KTX
Create a contex using *TinyKtx_CreateContext* passing in callbacks for
* optional error report
* alloc
* free
* read
* seek
* tell

All are provides a void* user data argument for file handle etc.

```
Read the header (TinyKtx_ReadHeader).
    A tell/read should be at the start of the KTX data

Query the dimension, format etc. (TinyKtx_Width etc.)

For each mipmap level in the file (TinyKtx_NumberOfMipmaps)
    get the mipmap size (TinyKtx_ImageSize)
    get the mipmap data (TinyKtx_ImageRawData)
```
Load snippet
```c
static void tinyktxCallbackError(void *user, char const *msg) {
	LOGERRORF("Tiny_Ktx ERROR: %s", msg);
}
static void *tinyktxCallbackAlloc(void *user, size_t size) {
	return MEMORY_MALLOC(size);
}
static void tinyktxCallbackFree(void *user, void *data) {
	MEMORY_FREE(data);
}
static size_t tinyktxCallbackRead(void *user, void* data, size_t size) {
	auto handle = (VFile_Handle) user;
	return VFile_Read(handle, data, size);
}
static bool tinyktxCallbackSeek(void *user, int64_t offset) {
	auto handle = (VFile_Handle) user;
	return VFile_Seek(handle, offset, VFile_SD_Begin);

}
static int64_t tinyktxCallbackTell(void *user) {
	auto handle = (VFile_Handle) user;
	return VFile_Tell(handle);
}

AL2O3_EXTERN_C Image_ImageHeader const *Image_LoadKTX(VFile_Handle handle) {
	TinyKtx_Callbacks callbacks {
			&tinyktxCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	auto ctx =  TinyKtx_CreateContext( &callbacks, handle);
	TinyKtx_ReadHeader(ctx);
	uint32_t w = TinyKtx_Width(ctx);
	uint32_t h = TinyKtx_Height(ctx);
	uint32_t d = TinyKtx_Depth(ctx);
	uint32_t s = TinyKtx_ArraySlices(ctx);
	ImageFormat fmt = ImageFormatToTinyKtxFormat(TinyKtx_GetFormat(ctx));
	if(fmt == ImageFormat_UNDEFINED) {
		TinyKtx_DestroyContext(ctx);
		return nullptr;
	}

	Image_ImageHeader const* topImage = nullptr;
	Image_ImageHeader const* prevImage = nullptr;
	for(auto i = 0u; i < TinyKtx_NumberOfMipmaps(ctx);++i) {
		auto image = Image_CreateNoClear(w, h, d, s, fmt);
		if(i == 0) topImage = image;

		if(Image_ByteCountOf(image) != TinyKtx_ImageSize(ctx, i)) {
			LOGERRORF("KTX file %s mipmap %i size error", VFile_GetName(handle), i);
			Image_Destroy(topImage);
			TinyKtx_DestroyContext(ctx);
			return nullptr;
		}
		memcpy(Image_RawDataPtr(image), TinyKtx_ImageRawData(ctx, i), Image_ByteCountOf(image));
		if(prevImage) {
			auto p = (Image_ImageHeader *)prevImage;
			p->nextType = Image_NextType::Image_IT_MipMaps;
			p->nextImage = image;
		}
		if(w > 1) w = w / 2;
		if(h > 1) h = h / 2;
		if(d > 1) d = d / 2;
		prevImage = image;
	}

	TinyKtx_DestroyContext(ctx);
	return topImage;
}
```


## How to save a KTX
 Saving doesn't need a context just a *TinyKtx_WriteCallbacks* with
 * error reporting
 * alloc (not currently used)
 * free (not currently used)
 * write

```
A TinyKtx_WriteImage or TinyKtx_WriteImageGL are the only API entry point for saving a KTX file.

Provide it with the format (in either style), dimensions and whether its a cube map or not

Pass the number of mipmaps and arrays filled with the size of each mipmap image and a pointer to the data
```
Save snippet
```c
static void tinyktxCallbackError(void *user, char const *msg) {
	LOGERRORF("Tiny_Ktx ERROR: %s", msg);
}

static void *tinyktxCallbackAlloc(void *user, size_t size) {
	return MEMORY_MALLOC(size);
}

static void tinyktxCallbackFree(void *user, void *data) {
	MEMORY_FREE(data);
}

static void tinyktxCallbackWrite(void *user, void const *data, size_t size) {
	auto handle = (VFile_Handle) user;
	VFile_Write(handle, data, size);
}
AL2O3_EXTERN_C bool Image_SaveKTX(Image_ImageHeader *image, VFile_Handle handle) {
	using namespace Image;
	TinyKtx_WriteCallbacks callback{
		&tinyktxCallbackError,
		&tinyktxCallbackAlloc,
		&tinyktxCallbackFree,
		&tinyktxCallbackWrite,
	};

	TinyKtx_Format fmt = ImageFormatToTinyKtxFormat(image->format);
	if(fmt == TKTX_UNDEFINED) return false;

	uint32_t numMipmaps = (image->nextType == Image_NextType::Image_IT_None) ? 1 : (uint32_t)Image_LinkedImageCountOf(image);

	uint32_t mipmapsizes[TINYKTX_MAX_MIPMAPLEVELS];
	void const* mipmaps[TINYKTX_MAX_MIPMAPLEVELS];
	memset(mipmapsizes, 0, sizeof(uint32_t)*TINYKTX_MAX_MIPMAPLEVELS);
	memset(mipmaps, 0, sizeof(void const*)*TINYKTX_MAX_MIPMAPLEVELS);

	for(size_t i = 0; i < numMipmaps; ++i) {
		mipmapsizes[i] = (uint32_t) Image_LinkedImageOf(image, i)->dataSize;
		mipmaps[i] = Image_RawDataPtr(Image_LinkedImageOf(image, i));
	}

	return TinyKtx_WriteImage(&callback,
                                 handle,
                                 image->width,
                                 image->height,
                                 image->depth,
                                 image->slices,
                                 numMipmaps,
                                 fmt,
                                 Image_IsCubemap(image),
                                 mipmapsizes,
                                 mipmaps );
}
```

## Tests
Testing is done using my Taylor scriptable content processor.

[taylor_imagetest - script and data](https://github.com/DeanoC/taylor_imagetests)

[taylor - app that runs the test script) ](https://github.com/DeanoC/taylor)

## TODO
Lots of validation/tests

Save key data pairs (currently will load them but doesn't write any)

Handle endianness?

## Higher level
tiny_ktx is the lowel level part of my gfx_imageio and gfx_image libraries.

They handle conversion or data, reading and writing and load/save other format ast well

taylor is a lua scripted content command line program that uses the above library for processing.

If you want higher level of tiny_ktx or how tiny_ktx is used see the links below

The snippets above are from gfx_imageio

[gfx_imageio - higher level import/export image using tiny_ktx (and other formats)](https://github.com/DeanoC/gfx_imageio)

[taylor - lua scripted image processer using gfx_imageio](https://github.com/DeanoC/taylor)
