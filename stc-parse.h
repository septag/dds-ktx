//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/st-image#license-bsd-2-clause
//
// stc-parse.h - v0.9.0 - Parser for DDS/KTX formats
//      Parses dds and ktx files from a memory blob, written in C99
//
//      Overriable defines:
//          STC_API     Define any function specifier for public functions (default: extern)
//          stc_memcpy  default: memcpy(dst, src, size)
//          stc_memset  default: memset(dst, v, size)
//          stc_assert  default: assert(a)
//          stc_strcpy  default: strcpy(dst, src)
//          
//      API:
//          bool stc_parse(stc_texture_container* tc, const void* file_data, int size, stc_error* err);
//              Parses texture container file and fills the stc_texture_container from texture data
//              Returns true if successfully parsed, false if failed with an error message inside stc_error parameter (optional)
//              After format is parsed, you can read the contents of stc_texture_format and create your GPU texture
//              To get pointer to mips and slices see stc_get_sub function
//          
//          void stc_get_sub(const stc_texture_container* tex, stc_sub_data* buff, 
//                           const void* file_data, int size,
//                           int layer_idx, int mip_idx);
//              Gets sub-image data, form a parsed texture file
//              user must provided the container object and the original file data which was passed to stc_parse
//              layer_idx: texture layer, array number, slice number or cube map face  (0..num_layers-1 or 0..depth-1)
//              mip_idx: mip index (0..num_mips-1 in stc_texture_container)
//          
//          const char* stc_format_str(stc_texture_format format);
//              Converts a format enumeration to string
//
//          bool stc_format_compressed(stc_texture_format format);
//              Returns true if format is compressed
//
//      Example:
//          int size;
//          void* dds_data = load_file("test.dds", &size);
//          assert(dds_data);
//          stc_texture_container tc = {0};
//          if (stc_parse(&tc, dds_data, size, NULL)) {
//              // Create GPU texture from tc data
//              for (int layer = 0; layer < tc->num_layers; layer++) {
//                  for (int mip = 0; mip < tc->num_mips; mip++) {
//                      stc_get_sub(&tc, dds_data, size, layer, mip);
//                      // Fill/Set texture sub resource data
//                  }
//              }
//          }
//          free(dds_data);     // memory must be valid during stc_ calls
//
// Version history:
//      0.9.0       Initial release, ktx is incomplete
//
// TODO
//      - KTX parser
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef STC_API
#   ifdef __cplusplus
#       define STC_API extern "C" 
#   else
#       define STC_API  
#   endif
#endif

typedef struct stc_sub_data
{
    const void* buff;
    int         width;
    int         height;
    int         depth;
    int         size_bytes;
    int         row_pitch_bytes;
    int         slice_pitch_bytes;
} stc_sub_data;

typedef enum stc_texture_format
{
    STC_FORMAT_BC1,         // DXT1
    STC_FORMAT_BC2,         // DXT3
    STC_FORMAT_BC3,         // DXT5
    STC_FORMAT_BC4,         // ATI1
    STC_FORMAT_BC5,         // ATI2
    STC_FORMAT_BC6H,        // BC6H
    STC_FORMAT_BC7,         // BC7
    STC_FORMAT_ETC1,        // ETC1 RGB8
    STC_FORMAT_ETC2,        // ETC2 RGB8
    STC_FORMAT_ETC2A,       // ETC2 RGBA8
    STC_FORMAT_ETC2A1,      // ETC2 RGBA8A1
    STC_FORMAT_PTC12,       // PVRTC1 RGB 2bpp
    STC_FORMAT_PTC14,       // PVRTC1 RGB 4bpp
    STC_FORMAT_PTC12A,      // PVRTC1 RGBA 2bpp
    STC_FORMAT_PTC14A,      // PVRTC1 RGBA 4bpp
    STC_FORMAT_PTC22,       // PVRTC2 RGBA 2bpp
    STC_FORMAT_PTC24,       // PVRTC2 RGBA 4bpp
    _STC_FORMAT_COMPRESSED,
    STC_FORMAT_A8,
    STC_FORMAT_R8,
    STC_FORMAT_RGBA8,
    STC_FORMAT_RGBA8S,
    STC_FORMAT_RG16,
    STC_FORMAT_RGB8,
    STC_FORMAT_R16,
    STC_FORMAT_R32F,
    STC_FORMAT_R16F,
    STC_FORMAT_RG16F,
    STC_FORMAT_RG16S,
    STC_FORMAT_RGBA16F,
    STC_FORMAT_RGBA16,
    STC_FORMAT_BGRA8,
    STC_FORMAT_RGB10A2,
    STC_FORMAT_RG11B10F,
    STC_FORMAT_RG8,
    STC_FORMAT_RG8S,
    _STC_FORMAT_COUNT
} stc_texture_format;

typedef enum stc_texture_flags
{
    STC_TEXTURE_FLAG_CUBEMAP = 0x01,       
    STC_TEXTURE_FLAG_SRGB    = 0x02,        
    STC_TEXTURE_FLAG_ALPHA   = 0x04,       // Has alpha channel
    STC_TEXTURE_FLAG_DDS     = 0x08,       // container was DDS file
    STC_TEXTURE_FLAG_KTX     = 0x10        // container was KTX file
} stc_texture_flags;

typedef struct stc_texture_container
{
    int                 data_offset;   // start offset of pixel data
    int                 size_bytes;
    stc_texture_format  format;
    unsigned int        flags;         // stc_texture_flags
    int                 width;
    int                 height;
    int                 depth;
    int                 num_layers;
    int                 num_mips;
    int                 bpp;
} stc_texture_container;

typedef enum stc_cube_face
{
    STC_CUBE_FACE_X_POSITIVE = 0,
    STC_CUBE_FACE_X_NEGATIVE,
    STC_CUBE_FACE_Y_POSITIVE,
    STC_CUBE_FACE_Y_NEGATIVE,
    STC_CUBE_FACE_Z_POSITIVE,
    STC_CUBE_FACE_Z_NEGATIVE
} stc_cube_face;

typedef struct stc_error
{
    char msg[256];
} stc_error;

#ifdef __cplusplus
#   define stc_default(_v) =_v
#else
#   define stc_default(_v)
#endif

STC_API bool stc_parse(stc_texture_container* tc, const void* file_data, int size, stc_error* err stc_default(NULL));
STC_API void stc_get_sub(const stc_texture_container* tex, stc_sub_data* buff, 
                         const void* file_data, int size,
                         int layer_idx, int mip_idx);
STC_API const char* stc_format_str(stc_texture_format format);
STC_API bool        stc_format_compressed(stc_texture_format format);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
#ifdef STC_IMPLEMENT

#define stc__makefourcc(_a, _b, _c, _d) ( ( (uint32_t)(_a) | ( (uint32_t)(_b) << 8) | \
                                        ( (uint32_t)(_c) << 16) | ( (uint32_t)(_d) << 24) ) )

#define STC__DDS_HEADER_SIZE 124
#define STC__DDS_MAGIC  stc__makefourcc('D', 'D', 'S', ' ')
#define STC__DDS_DXT1   stc__makefourcc('D', 'X', 'T', '1')
#define STC__DDS_DXT2   stc__makefourcc('D', 'X', 'T', '2')
#define STC__DDS_DXT3   stc__makefourcc('D', 'X', 'T', '3')
#define STC__DDS_DXT4   stc__makefourcc('D', 'X', 'T', '4')
#define STC__DDS_DXT5   stc__makefourcc('D', 'X', 'T', '5')
#define STC__DDS_ATI1   stc__makefourcc('A', 'T', 'I', '1')
#define STC__DDS_BC4U   stc__makefourcc('B', 'C', '4', 'U')
#define STC__DDS_ATI2   stc__makefourcc('A', 'T', 'I', '2')
#define STC__DDS_BC5U   stc__makefourcc('B', 'C', '5', 'U')
#define STC__DDS_DX10   stc__makefourcc('D', 'X', '1', '0')

#define STC__DDS_R8G8B8         20
#define STC__DDS_A8R8G8B8       21
#define STC__DDS_R5G6B5         23
#define STC__DDS_A1R5G5B5       25
#define STC__DDS_A4R4G4B4       26
#define STC__DDS_A2B10G10R10    31
#define STC__DDS_G16R16         34
#define STC__DDS_A2R10G10B10    35
#define STC__DDS_A16B16G16R16   36
#define STC__DDS_A8L8           51
#define STC__DDS_R16F           111
#define STC__DDS_G16R16F        112
#define STC__DDS_A16B16G16R16F  113
#define STC__DDS_R32F           114
#define STC__DDS_G32R32F        115
#define STC__DDS_A32B32G32R32F  116

#define STC__DDS_FORMAT_R32G32B32A32_FLOAT  2
#define STC__DDS_FORMAT_R32G32B32A32_UINT   3
#define STC__DDS_FORMAT_R16G16B16A16_FLOAT  10
#define STC__DDS_FORMAT_R16G16B16A16_UNORM  11
#define STC__DDS_FORMAT_R16G16B16A16_UINT   12
#define STC__DDS_FORMAT_R32G32_FLOAT        16
#define STC__DDS_FORMAT_R32G32_UINT         17
#define STC__DDS_FORMAT_R10G10B10A2_UNORM   24
#define STC__DDS_FORMAT_R11G11B10_FLOAT     26
#define STC__DDS_FORMAT_R8G8B8A8_UNORM      28
#define STC__DDS_FORMAT_R8G8B8A8_UNORM_SRGB 29
#define STC__DDS_FORMAT_R16G16_FLOAT        34
#define STC__DDS_FORMAT_R16G16_UNORM        35
#define STC__DDS_FORMAT_R32_FLOAT           41
#define STC__DDS_FORMAT_R32_UINT            42
#define STC__DDS_FORMAT_R8G8_UNORM          49
#define STC__DDS_FORMAT_R16_FLOAT           54
#define STC__DDS_FORMAT_R16_UNORM           56
#define STC__DDS_FORMAT_R8_UNORM            61
#define STC__DDS_FORMAT_R1_UNORM            66
#define STC__DDS_FORMAT_BC1_UNORM           71
#define STC__DDS_FORMAT_BC1_UNORM_SRGB      72
#define STC__DDS_FORMAT_BC2_UNORM           74
#define STC__DDS_FORMAT_BC2_UNORM_SRGB      75
#define STC__DDS_FORMAT_BC3_UNORM           77
#define STC__DDS_FORMAT_BC3_UNORM_SRGB      78
#define STC__DDS_FORMAT_BC4_UNORM           80
#define STC__DDS_FORMAT_BC5_UNORM           83
#define STC__DDS_FORMAT_B5G6R5_UNORM        85
#define STC__DDS_FORMAT_B5G5R5A1_UNORM      86
#define STC__DDS_FORMAT_B8G8R8A8_UNORM      87
#define STC__DDS_FORMAT_B8G8R8A8_UNORM_SRGB 91
#define STC__DDS_FORMAT_BC6H_SF16           96
#define STC__DDS_FORMAT_BC7_UNORM           98
#define STC__DDS_FORMAT_BC7_UNORM_SRGB      99
#define STC__DDS_FORMAT_B4G4R4A4_UNORM      115

#define STC__DDS_DX10_DIMENSION_TEXTURE2D 3
#define STC__DDS_DX10_DIMENSION_TEXTURE3D 4
#define STC__DDS_DX10_MISC_TEXTURECUBE    4

#define STC__DDSD_CAPS                  0x00000001
#define STC__DDSD_HEIGHT                0x00000002
#define STC__DDSD_WIDTH                 0x00000004
#define STC__DDSD_PITCH                 0x00000008
#define STC__DDSD_PIXELFORMAT           0x00001000
#define STC__DDSD_MIPMAPCOUNT           0x00020000
#define STC__DDSD_LINEARSIZE            0x00080000
#define STC__DDSD_DEPTH                 0x00800000

#define STC__DDPF_ALPHAPIXELS           0x00000001
#define STC__DDPF_ALPHA                 0x00000002
#define STC__DDPF_FOURCC                0x00000004
#define STC__DDPF_INDEXED               0x00000020
#define STC__DDPF_RGB                   0x00000040
#define STC__DDPF_YUV                   0x00000200
#define STC__DDPF_LUMINANCE             0x00020000
#define STC__DDPF_BUMPDUDV              0x00080000

#define STC__DDSCAPS_COMPLEX            0x00000008
#define STC__DDSCAPS_TEXTURE            0x00001000
#define STC__DDSCAPS_MIPMAP             0x00400000

#define STC__DDSCAPS2_VOLUME            0x00200000
#define STC__DDSCAPS2_CUBEMAP           0x00000200
#define STC__DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define STC__DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define STC__DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define STC__DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define STC__DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define STC__DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000

#define STC__DDSCAPS2_CUBEMAP_ALLSIDES (0      \
			| STC__DDSCAPS2_CUBEMAP_POSITIVEX \
			| STC__DDSCAPS2_CUBEMAP_NEGATIVEX \
			| STC__DDSCAPS2_CUBEMAP_POSITIVEY \
			| STC__DDSCAPS2_CUBEMAP_NEGATIVEY \
			| STC__DDSCAPS2_CUBEMAP_POSITIVEZ \
			| STC__DDSCAPS2_CUBEMAP_NEGATIVEZ )

#pragma pack(push, 1)
typedef struct stc__dds_pixel_format
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t rgb_bit_count;
    uint32_t bit_mask[4];
} stc__dds_pixel_format;

// https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dds-header
typedef struct stc__dds_header
{
    uint32_t                size;
    uint32_t                flags;
    uint32_t                height;
    uint32_t                width;
    uint32_t                pitch_lin_size;
    uint32_t                depth;
    uint32_t                mip_count;
    uint32_t                reserved1[11];
    stc__dds_pixel_format   pixel_format;
    uint32_t                caps1;
    uint32_t                caps2;
    uint32_t                caps3;
    uint32_t                caps4;
    uint32_t                reserved2;
} stc__dds_header;

// https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dds-header-dxt10
typedef struct stc__dds_header_dxgi
{
    uint32_t dxgi_format;
    uint32_t dimension;
    uint32_t misc_flags;
    uint32_t array_size;
    uint32_t misc_flags2;
} stc__dds_header_dxgi;

#pragma pack(pop)

typedef struct stc__dds_translate_fourcc_format
{
    uint32_t            dds_format;
    stc_texture_format  format;
    bool                srgb;
} stc__dds_translate_fourcc_format;

typedef struct stc__dds_translate_pixel_format
{
    uint32_t            bit_count;
    uint32_t            flags;
    uint32_t            bit_mask[4];
    stc_texture_format  format;
} stc__dds_translate_pixel_format;

typedef struct stc__mem_reader
{
    const uint8_t* buff;
    int            total;
    int            offset;
} stc__mem_reader;

typedef struct stc__block_info
{
    uint8_t bpp;
    uint8_t block_width;
    uint8_t block_height;
    uint8_t block_size;
    uint8_t min_block_x;
    uint8_t min_block_y;
    uint8_t depth_bits;
    uint8_t stencil_bits;
    uint8_t r_bits;
    uint8_t g_bits;
    uint8_t b_bits;
    uint8_t a_bits;
    uint8_t encoding;    
} stc__block_info;

#ifndef stc_memcpy
#   include <string.h>
#   define stc_memcpy(_dst, _src, _size)    memcpy((_dst), (_src), (_size))
#endif

#ifndef stc_memset
#   include <string.h>
#   define stc_memset(_dst, _v, _size)      memset((_dst), (_v), (_size))
#endif

#ifndef stc_assert
#   include <assert.h>
#   define stc_assert(_a)       assert(_a)
#endif

#ifndef stc_strcpy
#   include <string.h>
#   ifdef _MSC_VER
#       define stc_strcpy(_dst, _src)   strcpy_s((_dst), sizeof(_dst), (_src))
#   else
#       define stc_strcpy(_dst, _src)   strcpy((_dst), (_src))
#   endif
#endif

#define stc__max(a, b)              ((a) > (b) ? (a) : (b))
#define stc__min(a, b)              ((a) < (b) ? (a) : (b))
#define stc__err(_err, _msg)    if (_err)  stc_strcpy(_err->msg, _msg);   return false

static const stc__dds_translate_fourcc_format k__translate_dds_fourcc[] = {
    { STC__DDS_DXT1,                  STC_FORMAT_BC1,     false },
    { STC__DDS_DXT2,                  STC_FORMAT_BC2,     false },
    { STC__DDS_DXT3,                  STC_FORMAT_BC2,     false },
    { STC__DDS_DXT4,                  STC_FORMAT_BC3,     false },
    { STC__DDS_DXT5,                  STC_FORMAT_BC3,     false },
    { STC__DDS_ATI1,                  STC_FORMAT_BC4,     false },
    { STC__DDS_BC4U,                  STC_FORMAT_BC4,     false },
    { STC__DDS_ATI2,                  STC_FORMAT_BC5,     false },
    { STC__DDS_BC5U,                  STC_FORMAT_BC5,     false },
    { STC__DDS_A16B16G16R16,          STC_FORMAT_RGBA16,  false },
    { STC__DDS_A16B16G16R16F,         STC_FORMAT_RGBA16F, false },
    { STC__DDPF_RGB|STC__DDPF_ALPHAPIXELS, STC_FORMAT_BGRA8,   false },
    { STC__DDPF_INDEXED,              STC_FORMAT_R8,      false },
    { STC__DDPF_LUMINANCE,            STC_FORMAT_R8,      false },
    { STC__DDPF_ALPHA,                STC_FORMAT_R8,      false },
    { STC__DDS_R16F,                  STC_FORMAT_R16F,    false },
    { STC__DDS_R32F,                  STC_FORMAT_R32F,    false },
    { STC__DDS_A8L8,                  STC_FORMAT_RG8,     false },
    { STC__DDS_G16R16,                STC_FORMAT_RG16,    false },
    { STC__DDS_G16R16F,               STC_FORMAT_RG16F,   false },
    { STC__DDS_R8G8B8,                STC_FORMAT_RGB8,    false },
    { STC__DDS_A8R8G8B8,              STC_FORMAT_BGRA8,   false },
    { STC__DDS_A16B16G16R16,          STC_FORMAT_RGBA16,  false },
    { STC__DDS_A16B16G16R16F,         STC_FORMAT_RGBA16F, false },
    { STC__DDS_A2B10G10R10,           STC_FORMAT_RGB10A2, false },
};

static const stc__dds_translate_fourcc_format k__translate_dxgi[] = {
    { STC__DDS_FORMAT_BC1_UNORM,           STC_FORMAT_BC1,        false },
    { STC__DDS_FORMAT_BC1_UNORM_SRGB,      STC_FORMAT_BC1,        true  },
    { STC__DDS_FORMAT_BC2_UNORM,           STC_FORMAT_BC2,        false },
    { STC__DDS_FORMAT_BC2_UNORM_SRGB,      STC_FORMAT_BC2,        true  },
    { STC__DDS_FORMAT_BC3_UNORM,           STC_FORMAT_BC3,        false },
    { STC__DDS_FORMAT_BC3_UNORM_SRGB,      STC_FORMAT_BC3,        true  },
    { STC__DDS_FORMAT_BC4_UNORM,           STC_FORMAT_BC4,        false },
    { STC__DDS_FORMAT_BC5_UNORM,           STC_FORMAT_BC5,        false },
    { STC__DDS_FORMAT_BC6H_SF16,           STC_FORMAT_BC6H,       false },
    { STC__DDS_FORMAT_BC7_UNORM,           STC_FORMAT_BC7,        false },
    { STC__DDS_FORMAT_BC7_UNORM_SRGB,      STC_FORMAT_BC7,        true  },

    { STC__DDS_FORMAT_R8_UNORM,            STC_FORMAT_R8,         false },
    { STC__DDS_FORMAT_R16_UNORM,           STC_FORMAT_R16,        false },
    { STC__DDS_FORMAT_R16_FLOAT,           STC_FORMAT_R16F,       false },
    { STC__DDS_FORMAT_R32_FLOAT,           STC_FORMAT_R32F,       false },
    { STC__DDS_FORMAT_R8G8_UNORM,          STC_FORMAT_RG8,        false },
    { STC__DDS_FORMAT_R16G16_UNORM,        STC_FORMAT_RG16,       false },
    { STC__DDS_FORMAT_R16G16_FLOAT,        STC_FORMAT_RG16F,      false },
    { STC__DDS_FORMAT_B8G8R8A8_UNORM,      STC_FORMAT_BGRA8,      false },
    { STC__DDS_FORMAT_B8G8R8A8_UNORM_SRGB, STC_FORMAT_BGRA8,      true  },
    { STC__DDS_FORMAT_R8G8B8A8_UNORM,      STC_FORMAT_RGBA8,      false },
    { STC__DDS_FORMAT_R8G8B8A8_UNORM_SRGB, STC_FORMAT_RGBA8,      true  },
    { STC__DDS_FORMAT_R16G16B16A16_UNORM,  STC_FORMAT_RGBA16,     false },
    { STC__DDS_FORMAT_R16G16B16A16_FLOAT,  STC_FORMAT_RGBA16F,    false },
    { STC__DDS_FORMAT_R10G10B10A2_UNORM,   STC_FORMAT_RGB10A2,    false },
    { STC__DDS_FORMAT_R11G11B10_FLOAT,     STC_FORMAT_RG11B10F,   false },
};

static const stc__dds_translate_pixel_format k__translate_dds_pixel[] = {
    {  8, STC__DDPF_LUMINANCE,            { 0x000000ff, 0x00000000, 0x00000000, 0x00000000 }, STC_FORMAT_R8      },
    { 16, STC__DDPF_BUMPDUDV,             { 0x000000ff, 0x0000ff00, 0x00000000, 0x00000000 }, STC_FORMAT_RG8S    },
    { 24, STC__DDPF_RGB,                  { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, STC_FORMAT_RGB8    },
    { 24, STC__DDPF_RGB,                  { 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 }, STC_FORMAT_RGB8    },
    { 32, STC__DDPF_RGB,                  { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, STC_FORMAT_BGRA8   },
    { 32, STC__DDPF_RGB|STC__DDPF_ALPHAPIXELS, { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }, STC_FORMAT_RGBA8   },
    { 32, STC__DDPF_BUMPDUDV,             { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }, STC_FORMAT_RGBA8S  },
    { 32, STC__DDPF_RGB,                  { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, STC_FORMAT_BGRA8   },
    { 32, STC__DDPF_RGB|STC__DDPF_ALPHAPIXELS, { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, STC_FORMAT_BGRA8   }, // D3DFMT_A8R8G8B8
    { 32, STC__DDPF_RGB|STC__DDPF_ALPHAPIXELS, { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, STC_FORMAT_BGRA8   }, // D3DFMT_X8R8G8B8
    { 32, STC__DDPF_RGB|STC__DDPF_ALPHAPIXELS, { 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 }, STC_FORMAT_RGB10A2 },
    { 32, STC__DDPF_RGB,                  { 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 }, STC_FORMAT_RG16    },
    { 32, STC__DDPF_BUMPDUDV,             { 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 }, STC_FORMAT_RG16S   }
};

typedef enum stc__encode_type
{
    STC__ENCODE_UNORM,
    STC__ENCODE_SNORM,
    STC__ENCODE_FLOAT,
    STC__ENCODE_INT,
    STC__ENCODE_UINT,
    STC__ENCODE_COUNT
} stc__encode_type;

static const stc__block_info k__block_info[] =
{
    //  +-------------------------------------------- bits per pixel
    //  |  +----------------------------------------- block width
    //  |  |  +-------------------------------------- block height
    //  |  |  |   +---------------------------------- block size
    //  |  |  |   |  +------------------------------- min blocks x
    //  |  |  |   |  |  +---------------------------- min blocks y
    //  |  |  |   |  |  |   +------------------------ depth bits
    //  |  |  |   |  |  |   |  +--------------------- stencil bits
    //  |  |  |   |  |  |   |  |   +---+---+---+----- r, g, b, a bits
    //  |  |  |   |  |  |   |  |   r   g   b   a  +-- encoding type
    //  |  |  |   |  |  |   |  |   |   |   |   |  |
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC1
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC2
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC3
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC4
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC5
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_FLOAT) }, // BC6H
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // BC7
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ETC1
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ETC2
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ETC2A
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ETC2A1
    {   2, 8, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC12
    {   4, 4, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC14
    {   2, 8, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC12A
    {   4, 4, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC14A
    {   2, 8, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC22
    {   4, 4, 4,  8, 2, 2,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // PTC24
    {   0, 0, 0,  0, 0, 0,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_COUNT) }, // Unknown
    {   8, 1, 1,  1, 1, 1,  0, 0,  0,  0,  0,  8, (uint8_t)(STC__ENCODE_UNORM) }, // A8
    {   8, 1, 1,  1, 1, 1,  0, 0,  8,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // R8
    {  32, 1, 1,  4, 1, 1,  0, 0,  8,  8,  8,  8, (uint8_t)(STC__ENCODE_UNORM) }, // RGBA8
    {  32, 1, 1,  4, 1, 1,  0, 0,  8,  8,  8,  8, (uint8_t)(STC__ENCODE_SNORM) }, // RGBA8S
    {  32, 1, 1,  4, 1, 1,  0, 0, 16, 16,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // RG16
    {  24, 1, 1,  3, 1, 1,  0, 0,  8,  8,  8,  0, (uint8_t)(STC__ENCODE_UNORM) }, // RGB8
    {  16, 1, 1,  2, 1, 1,  0, 0, 16,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // R16
    {  32, 1, 1,  4, 1, 1,  0, 0, 32,  0,  0,  0, (uint8_t)(STC__ENCODE_FLOAT) }, // R32F
    {  16, 1, 1,  2, 1, 1,  0, 0, 16,  0,  0,  0, (uint8_t)(STC__ENCODE_FLOAT) }, // R16F
    {  32, 1, 1,  4, 1, 1,  0, 0, 16, 16,  0,  0, (uint8_t)(STC__ENCODE_FLOAT) }, // RG16F
    {  32, 1, 1,  4, 1, 1,  0, 0, 16, 16,  0,  0, (uint8_t)(STC__ENCODE_SNORM) }, // RG16S
    {  64, 1, 1,  8, 1, 1,  0, 0, 16, 16, 16, 16, (uint8_t)(STC__ENCODE_FLOAT) }, // RGBA16F
    {  64, 1, 1,  8, 1, 1,  0, 0, 16, 16, 16, 16, (uint8_t)(STC__ENCODE_UNORM) }, // RGBA16
    {  32, 1, 1,  4, 1, 1,  0, 0,  8,  8,  8,  8, (uint8_t)(STC__ENCODE_UNORM) }, // BGRA8
    {  32, 1, 1,  4, 1, 1,  0, 0, 10, 10, 10,  2, (uint8_t)(STC__ENCODE_UNORM) }, // RGB10A2
    {  32, 1, 1,  4, 1, 1,  0, 0, 11, 11, 10,  0, (uint8_t)(STC__ENCODE_UNORM) }, // RG11B10F
    {  16, 1, 1,  2, 1, 1,  0, 0,  8,  8,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // RG8
    {  16, 1, 1,  2, 1, 1,  0, 0,  8,  8,  0,  0, (uint8_t)(STC__ENCODE_SNORM) }  // RG8S
};


static inline int stc__read(stc__mem_reader* reader, void* buff, int size)
{
    int read_bytes = (reader->offset + size) <= reader->total ? size : (reader->total - reader->offset);
    stc_memcpy(buff, reader->buff + reader->offset, read_bytes);
    reader->offset += read_bytes;
    return read_bytes;
}

bool stc__parse_ktx(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
{
    stc_assert(0 && "To be implemented");
    return false;
}

bool stc__parse_dds(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
{

    stc__mem_reader r = {(const uint8_t*)file_data, size, sizeof(uint32_t)};
    stc__dds_header header;
    if (stc__read(&r, &header, sizeof(header)) < STC__DDS_HEADER_SIZE ||
        header.size != STC__DDS_HEADER_SIZE)
    {
        stc__err(err, "dds: header size does not match");
    }

    uint32_t required_flags = (STC__DDSD_CAPS|STC__DDSD_HEIGHT|STC__DDSD_WIDTH|STC__DDSD_PIXELFORMAT);
    if ((header.flags & required_flags) != required_flags) {
        stc__err(err, "dds: have invalid flags");
    }

    if (header.pixel_format.size != sizeof(stc__dds_pixel_format)) {
        stc__err(err, "dds: pixel format header is invalid");
    }

    uint32_t dxgi_format = 0;
    uint32_t array_size = 1;
    if (STC__DDPF_FOURCC == (header.flags & STC__DDPF_FOURCC) && 
        header.pixel_format.fourcc == STC__DDS_DX10)
    {
        stc__dds_header_dxgi dxgi_header;
        stc__read(&r, &dxgi_header, sizeof(dxgi_header));
        dxgi_format = dxgi_header.dxgi_format;
        array_size = dxgi_header.array_size;
    }

    if ((header.caps1 & STC__DDSCAPS_TEXTURE) == 0) {
        stc__err(err, "dds: unsupported caps");
    }

    bool cubemap = (header.caps2 & STC__DDSCAPS2_CUBEMAP) != 0;
    if (cubemap && (header.caps2 & STC__DDSCAPS2_CUBEMAP_ALLSIDES) != STC__DDSCAPS2_CUBEMAP_ALLSIDES) {
        stc__err(err, "dds: incomplete cubemap");
    }

    stc_texture_format format = _STC_FORMAT_COUNT;
    bool has_alpha = (header.pixel_format.flags & STC__DDPF_ALPHA) != 0;
    bool srgb = false;

    if (dxgi_format == 0) {
        if ((header.pixel_format.flags & STC__DDPF_FOURCC) == STC__DDPF_FOURCC) {
            int count = sizeof(k__translate_dds_fourcc)/sizeof(stc__dds_translate_fourcc_format);
            for (int i = 0; i < count; i++) {
                if (k__translate_dds_fourcc[i].dds_format == header.pixel_format.fourcc) {
                    format = k__translate_dds_fourcc[i].format;
                    break;
                }
            }
        } else {
            int count = sizeof(k__translate_dds_pixel)/sizeof(stc__dds_translate_pixel_format);
            for (int i = 0; i < count; i++) {
                const stc__dds_translate_pixel_format* f = &k__translate_dds_pixel[i];
                if (f->bit_count == header.pixel_format.rgb_bit_count &&
                    f->flags == header.pixel_format.flags &&
                    f->bit_mask[0] == header.pixel_format.bit_mask[0] &&
                    f->bit_mask[1] == header.pixel_format.bit_mask[1] &&
                    f->bit_mask[2] == header.pixel_format.bit_mask[2] &&
                    f->bit_mask[3] == header.pixel_format.bit_mask[3])
                {
                    format = f->format;
                    break;
                }
            }
        }
    } else {
        int count = sizeof(k__translate_dxgi)/sizeof(stc__dds_translate_fourcc_format);
        for (int i = 0; i < count; i++) {
            if (k__translate_dxgi[i].dds_format == dxgi_format) {
                format = k__translate_dxgi[i].format;
                srgb = k__translate_dxgi[i].srgb;
                break;
            }
        }
    }

    if (format == _STC_FORMAT_COUNT) {
        stc__err(err, "dds: unknown format");
    }

    stc_memset(tc, 0x0, sizeof(stc_texture_container));
    tc->data_offset = r.offset;
    tc->size_bytes = r.total - r.offset;
    tc->format = format;
    tc->width = (int)header.width;
    tc->height = (int)header.height;
    tc->depth = stc__max(1, (int)header.depth);
    tc->num_layers = (int)array_size;
    tc->num_mips = (header.caps1 & STC__DDSCAPS_MIPMAP) ? (int)header.mip_count : 1;
    tc->bpp = k__block_info[format].bpp;
    if (has_alpha)
        tc->flags |= STC_TEXTURE_FLAG_ALPHA;
    if (cubemap)
        tc->flags |= STC_TEXTURE_FLAG_CUBEMAP;
    if (srgb) 
        tc->flags |= STC_TEXTURE_FLAG_SRGB;
    tc->flags |= STC_TEXTURE_FLAG_DDS;

    return true;
}   


STC_API void stc_get_sub(const stc_texture_container* tc, stc_sub_data* sub_data, 
                            const void* file_data, int size,
                            int layer_idx, int mip_idx)
{
    stc_assert(tc);
    stc_assert(sub_data);
    stc_assert(file_data);
    stc_assert(size > 0);
    stc_assert(layer_idx < tc->num_layers);
    stc_assert(mip_idx < tc->num_mips);

    stc__mem_reader r = { (uint8_t*)file_data, size, tc->data_offset };
    stc_texture_format format = tc->format;
    bool has_alpha = (tc->flags & STC_TEXTURE_FLAG_ALPHA) ? true : false;

    stc_assert(format < _STC_FORMAT_COUNT && format != _STC_FORMAT_COMPRESSED);
    const stc__block_info* binfo = &k__block_info[format];
    const int bpp         = binfo->bpp;
    const int block_size   = binfo->block_size;
    const int block_width  = binfo->block_width;
    const int block_height = binfo->block_height;
    const int min_block_x  = binfo->min_block_x;
    const int min_block_y  = binfo->min_block_y;

    const int num_sides = tc->num_layers * ((tc->flags & STC_TEXTURE_FLAG_CUBEMAP) ? 6 : 1);
    const int min_width = min_block_x*block_width;
    const int min_height = min_block_y*block_height;

    if (tc->flags & STC_TEXTURE_FLAG_DDS) {
        for (int side = 0; side < num_sides; side++) {
            int width = tc->width;
            int height = tc->height;
            int depth = tc->depth;

            for (int mip = 0, c = tc->num_mips; mip < c; mip++) {
                width = ((width + block_width - 1)/block_width)*block_width;
                height = ((height + block_height - 1)/block_height)*block_height;
                width = stc__max(min_width, width);
                height = stc__max(min_height, height);
                depth = stc__max(1, depth);
                int size = width*height*depth*bpp/8;

                if (side == layer_idx && mip == mip_idx) {
                    sub_data->buff = r.buff + r.offset;
                    sub_data->width = width;
                    sub_data->height = height;
                    sub_data->depth = depth;
                    sub_data->size_bytes = size;
                    sub_data->row_pitch_bytes = width*bpp/8;
                    sub_data->slice_pitch_bytes = width*height*bpp/8;
                }

                r.offset += size;
                stc_assert(r.offset <= r.total && "texture buffer overflow");

                width >>= 1;
                height >>= 1;
                depth >>= 1;
            }
        }
    } else if (tc->flags & STC_TEXTURE_FLAG_KTX) {
        stc_assert(0 && "TODO");
    }
}

STC_API bool stc_parse(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
{
    stc_assert(tc);
    stc_assert(file_data);
    stc_assert(size > 0);

    stc__mem_reader r = {(const uint8_t*)file_data, size, 0};
    
    // Read file flag and determine the file type
    uint32_t file_flag = 0;
    if (stc__read(&r, &file_flag, sizeof(file_flag)) != sizeof(file_flag)) {
        stc__err(err, "invalid texture file");
    }

    switch (file_flag) {
    case STC__DDS_MAGIC:
        return stc__parse_dds(tc, file_data, size, err);
    default:
        stc__err(err, "unknown texture format");
    }
}

STC_API const char* stc_format_str(stc_texture_format format)
{
    switch (format) {
    case STC_FORMAT_BC1:         return "BC1";
    case STC_FORMAT_BC2:         return "BC2";
    case STC_FORMAT_BC3:         return "BC3";
    case STC_FORMAT_BC4:         return "BC4";
    case STC_FORMAT_BC5:         return "BC5";
    case STC_FORMAT_BC6H:        return "BC6H";
    case STC_FORMAT_BC7:         return "BC7";
    case STC_FORMAT_ETC1:        return "ETC1";
    case STC_FORMAT_ETC2:        return "ETC2";
    case STC_FORMAT_ETC2A:       return "ETC2A";
    case STC_FORMAT_ETC2A1:      return "ETC2A1";
    case STC_FORMAT_PTC12:       return "PTC12";
    case STC_FORMAT_PTC14:       return "PTC14";
    case STC_FORMAT_PTC12A:      return "PTC12A";
    case STC_FORMAT_PTC14A:      return "PTC14A";
    case STC_FORMAT_PTC22:       return "PTC22";
    case STC_FORMAT_PTC24:       return "PTC24";
    case STC_FORMAT_A8:          return "A8";
    case STC_FORMAT_R8:          return "R8";
    case STC_FORMAT_RGBA8:       return "RGBA8";
    case STC_FORMAT_RGBA8S:      return "RGBA8S";
    case STC_FORMAT_RG16:        return "RG16";
    case STC_FORMAT_RGB8:        return "RGB8";
    case STC_FORMAT_R16:         return "R16";
    case STC_FORMAT_R32F:        return "R32F";
    case STC_FORMAT_R16F:        return "R16F";
    case STC_FORMAT_RG16F:       return "RG16F";
    case STC_FORMAT_RG16S:       return "RG16S";
    case STC_FORMAT_RGBA16F:     return "RGBA16F";
    case STC_FORMAT_RGBA16:      return "RGBA16";
    case STC_FORMAT_BGRA8:       return "BGRA8";
    case STC_FORMAT_RGB10A2:     return "RGB10A2";
    case STC_FORMAT_RG11B10F:    return "R11B10F";
    case STC_FORMAT_RG8:         return "RG8";
    case STC_FORMAT_RG8S:        return "RG8S";
    default:                     return "Unknown";        
    }
}

STC_API bool stc_format_compressed(stc_texture_format format)
{
    stc_assert(format != _STC_FORMAT_COMPRESSED && format != _STC_FORMAT_COUNT);
    return format < _STC_FORMAT_COMPRESSED;
}

#endif  // STC_IMPLEMENT

