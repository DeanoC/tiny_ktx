#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "tiny_ktx/tinyktx.h"
#include "al2o3_catch2/catch2.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "al2o3_stb/stb_image.h"

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
static int stbIoCallbackRead(void *user, char *data, int size) {
	auto handle = (VFile_Handle) user;
	return (int) VFile_Read(handle, data, size);
}
static void stbIoCallbackSkip(void *user, int n) {
	auto handle = (VFile_Handle) user;
	VFile_Seek(handle, n, VFile_SD_Current);
}
static int stbIoCallbackEof(void *user) {
	auto handle = (VFile_Handle) user;
	return VFile_IsEOF(handle);
}


TEST_CASE("Check Files", "[TinyKtx Loader]") {
#define CHK_FILE_EXISTS(filename) \
	{ VFile::ScopedFile reffile = VFile::File::FromFile(filename, Os_FM_ReadBinary); \
	if(!reffile) { \
		LOGERROR("This must run in the directory input/testimages/ that can be got from http://github/DeanoC/taylor_imagetests"); \
		REQUIRE(reffile); \
	} }

	CHK_FILE_EXISTS("rgb-reference.ktx");
	CHK_FILE_EXISTS("rgb.ppm");
	CHK_FILE_EXISTS("luminance-reference-metadata.ktx");
	CHK_FILE_EXISTS("luminance.pgm");
	CHK_FILE_EXISTS("level0.ppm");
	CHK_FILE_EXISTS("level1.ppm");
	CHK_FILE_EXISTS("level2.ppm");
	CHK_FILE_EXISTS("level3.ppm");
	CHK_FILE_EXISTS("level4.ppm");
	CHK_FILE_EXISTS("level5.ppm");
	CHK_FILE_EXISTS("level6.ppm");

#undef CHK_FILE_EXISTS
}

TEST_CASE("TinyKtx Create/Destroy Context", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	VFile::ScopedFile file = VFile::File::FromFile("rgb-reference.ktx", Os_FM_ReadBinary);
	if(!file) {
		LOGERROR("This must run in the directory input/testimages/ that can be got from http://github/DeanoC/taylor_imagetests");
		REQUIRE(file);
	}

	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);
	REQUIRE(ctx);

	TinyKtx_DestroyContext(ctx);
}

TEST_CASE("TinyKtx readheader & dimensions", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	VFile::ScopedFile file = VFile::File::FromFile("rgb-reference.ktx", Os_FM_ReadBinary);
	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);
	REQUIRE(TinyKtx_ReadHeader(ctx));

	auto w = TinyKtx_Width(ctx);
	auto h = TinyKtx_Height(ctx);
	auto d = TinyKtx_Depth(ctx);
	auto s = TinyKtx_ArraySlices(ctx);

	uint32_t wd, hd, dd, sd;
	TinyKtx_Dimensions(ctx, &wd, &hd, &dd, &sd);

	REQUIRE(w == wd);
	REQUIRE(h == hd);
	REQUIRE(d == dd);
	REQUIRE(s == sd);

	REQUIRE(w == 128);
	REQUIRE(h == 128);
	REQUIRE(d == 0);
	REQUIRE(s == 0);

	REQUIRE(TinyKtx_NumberOfMipmaps(ctx) == 1);

	TinyKtx_DestroyContext(ctx);
}

static bool CmpFlipped(	uint32_t w,
												uint32_t h,
												uint8_t pixByte,
												uint32_t srcStride,
												uint32_t dstStride,
												uint8_t const* src,
												uint8_t const* dst) {

	dst = dst + ((h-1) * dstStride);

	for (auto i = 0u; i < h; ++i) {
		uint8_t const *srcBackup = src;

		for (auto j = 0u; j < w; ++j) {
			src += pixByte;
			for(auto p = 0u; p < pixByte;++p) {
				if(src[p] != dst[p]) return false;
			}
		}

		src = srcBackup + srcStride;
		dst = dst + ((h-1-i) * dstStride);
	}

	return true;
}

TEST_CASE("TinyKtx rgb-reference okay", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	stbi_io_callbacks stbi_callbacks{
			&stbIoCallbackRead,
			&stbIoCallbackSkip,
			&stbIoCallbackEof
	};

	VFile::ScopedFile file = VFile::File::FromFile("rgb-reference.ktx", Os_FM_ReadBinary);
	VFile::ScopedFile reffile = VFile::File::FromFile("rgb.ppm", Os_FM_ReadBinary);

	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);

	REQUIRE(TinyKtx_ReadHeader(ctx));

	size_t origin = VFile_Tell(reffile);

	int w = 0, h = 0, cmp = 0;
	stbi_info_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp);
	REQUIRE(w == TinyKtx_Width(ctx));
	REQUIRE(h == TinyKtx_Height(ctx));
	REQUIRE(TinyKtx_GetFormat(ctx) == TKTX_R8G8B8_UNORM);

	VFile_Seek(reffile, origin, VFile_SD_Begin);
	stbi_uc *refdata = stbi_load_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp, cmp);
	REQUIRE(refdata);

	auto ktxdata =  (uint8_t const*)TinyKtx_ImageRawData(ctx, 0);
	REQUIRE(CmpFlipped(w, h, 3, w * cmp, w * cmp, refdata, ktxdata) == 0);

	MEMORY_FREE((void*)refdata);
	TinyKtx_DestroyContext(ctx);
}

TEST_CASE("TinyKtx luminance-reference okay", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	stbi_io_callbacks stbi_callbacks{
			&stbIoCallbackRead,
			&stbIoCallbackSkip,
			&stbIoCallbackEof
	};

	VFile::ScopedFile file = VFile::File::FromFile("luminance-reference-metadata.ktx", Os_FM_ReadBinary);
	VFile::ScopedFile reffile = VFile::File::FromFile("luminance.pgm", Os_FM_ReadBinary);

	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);

	REQUIRE(TinyKtx_ReadHeader(ctx));

	size_t origin = VFile_Tell(reffile);

	int w = 0, h = 0, cmp = 0;
	stbi_info_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp);
	REQUIRE(w == TinyKtx_Width(ctx));
	REQUIRE(h == TinyKtx_Height(ctx));
	REQUIRE(TinyKtx_GetFormat(ctx) == TKTX_R8_UNORM);

	VFile_Seek(reffile, origin, VFile_SD_Begin);
	stbi_uc *refdata = stbi_load_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp, cmp);
	REQUIRE(refdata);

	auto ktxdata =  (uint8_t const*)TinyKtx_ImageRawData(ctx, 0);
	REQUIRE(CmpFlipped(w, h, 3, w * cmp, w * cmp, refdata, ktxdata) == 0);

	MEMORY_FREE((void*)refdata);
	TinyKtx_DestroyContext(ctx);
}

TEST_CASE("TinyKtx git hub #2 (image size before image raw data broken) fix test", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	stbi_io_callbacks stbi_callbacks{
			&stbIoCallbackRead,
			&stbIoCallbackSkip,
			&stbIoCallbackEof
	};

	VFile::ScopedFile file = VFile::File::FromFile("rgb-reference.ktx", Os_FM_ReadBinary);
	VFile::ScopedFile reffile = VFile::File::FromFile("rgb.ppm", Os_FM_ReadBinary);
	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);

	REQUIRE(TinyKtx_ReadHeader(ctx));


	size_t origin = VFile_Tell(reffile);

	int w = 0, h = 0, cmp = 0;
	stbi_info_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp);

	uint64_t memoryRequirement = sizeof(stbi_uc) * w * h * cmp;

	// perform an image size op, this shouldn't break the later image raw data if this if fixed
	REQUIRE(memoryRequirement == TinyKtx_ImageSize(ctx, 0));

	VFile_Seek(reffile, origin, VFile_SD_Begin);
	stbi_uc const *refdata = stbi_load_from_callbacks(&stbi_callbacks, (void*)reffile.owned, &w, &h, &cmp, cmp);
	REQUIRE(refdata);

	auto ktxdata =  (uint8_t const*)TinyKtx_ImageRawData(ctx, 0);
	REQUIRE(CmpFlipped(w, h, 3, w * cmp, w * cmp, refdata, ktxdata) == 0);

	MEMORY_FREE((void*)refdata);
	TinyKtx_DestroyContext(ctx);
}

TEST_CASE("TinyKtx mipmap reference check", "[TinyKtx Loader]") {
	TinyKtx_Callbacks callbacks {
			&tinyktxddsCallbackError,
			&tinyktxCallbackAlloc,
			&tinyktxCallbackFree,
			tinyktxCallbackRead,
			&tinyktxCallbackSeek,
			&tinyktxCallbackTell
	};

	stbi_io_callbacks stbi_callbacks{
			&stbIoCallbackRead,
			&stbIoCallbackSkip,
			&stbIoCallbackEof
	};

	VFile::ScopedFile file = VFile::File::FromFile("rgb-mipmap-reference.ktx", Os_FM_ReadBinary);
	VFile::ScopedFile reffile[7] {
			VFile::File::FromFile("level0.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level1.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level2.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level3.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level4.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level5.ppm", Os_FM_ReadBinary),
			VFile::File::FromFile("level6.ppm", Os_FM_ReadBinary),
	};
	auto ctx = TinyKtx_CreateContext(&callbacks, (void*)file.owned);
	REQUIRE(TinyKtx_ReadHeader(ctx));

	REQUIRE(TinyKtx_NumberOfMipmaps(ctx) == 7);

	for (auto i = 0u; i < 7; ++i) {
		size_t origin = VFile_Tell(reffile[i]);
		int w = 0, h = 0, cmp = 0;

		stbi_info_from_callbacks(&stbi_callbacks, (void*)reffile[i].owned, &w, &h, &cmp);
		VFile_Seek(reffile[i], origin, VFile_SD_Begin);
		stbi_uc const *refdata = stbi_load_from_callbacks(&stbi_callbacks, (void*)reffile[i].owned, &w, &h, &cmp, cmp);
		REQUIRE(refdata);

		uint32_t const srcStride = w * cmp;
		uint32_t dstStride = srcStride;

		if( i < 5) {
			uint64_t const memoryRequirement = sizeof(stbi_uc) * w * h * cmp;
			REQUIRE(memoryRequirement == TinyKtx_ImageSize(ctx, i));
			REQUIRE(!TinyKtx_IsMipMapLevelUnpacked(ctx,i));
		} else {
			REQUIRE(TinyKtx_IsMipMapLevelUnpacked(ctx,i));
			dstStride = TinyKtx_UnpackedRowStride(ctx,i);
			if(i == 5) {
				REQUIRE(dstStride == 8);
			} else if(i == 6) {
				REQUIRE(dstStride == 4);
			}
		}

		auto ktxdata =  (uint8_t const*)TinyKtx_ImageRawData(ctx, i);
		REQUIRE(CmpFlipped(w, h, 3, srcStride, dstStride, refdata, ktxdata) == 0);
		MEMORY_FREE((void*)refdata);
	}

	TinyKtx_DestroyContext(ctx);
}