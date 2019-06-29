# tiny_ktx
Small C based KTX texture loader (inspired by syoyo tiny libraries)

## What it does?
* Loads Khronos KTX textures
* Saves Khronos KTX textures
* Optionally provide format conversion from GL/KTX style to Vulkan/Dx12/Metal style
* Optionally provides GL defines requires to read KTX files without GL

## How to build
include tinyktx.h from include/tiny_ktx in your project
in 1 file define TINYKTX_IMPLEMENTATION before tinyktx.h

## How to use
Create a Tinyktx_Context passing in call