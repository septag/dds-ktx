//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/st-image#license-bsd-2-clause
//
// stc-parse.h - v1.0.0 - Parser for DDS/KTX formats
//      Parses DDS and KTX files from a memory blob, written in C99
//      
//      Supported formats:
//          For supported formats, see stc_texture_format enum. 
//          Both KTX/DDS parser supports all formats defined in stc_texture_format
//
//      Overriable macros:
//          STC_API     Define any function specifier for public functions (default: extern)
//          stc_memcpy  default: memcpy(dst, src, size)
//          stc_memset  default: memset(dst, v, size)
//          stc_assert  default: assert(a)
//          stc_strcpy  default: strcpy(dst, src)
//          
//      API:
//          bool stc_parse(stc_texture_container* tc, const void* file_data, int size, stc_error* err);
//              Parses texture file and fills the stc_texture_container struct
//              Returns true if successfully parsed, false if failed with an error message inside stc_error parameter (optional)
//              After format is parsed, you can read the contents of stc_texture_format and create your GPU texture
//              To get pointer to mips and slices see stc_get_sub function
//          
//          void stc_get_sub(const stc_texture_container* tex, stc_sub_data* buff, 
//                           const void* file_data, int size,
//                           int array_idx, int slice_face_idx, int mip_idx);
//              Gets sub-image data, form a parsed texture file
//              user must provided the container object and the original file data which was passed to stc_parse
//              array_idx: array index (0..num_layers)
//              slice_face_idx: depth-slice or cube-face index. 
//                              if 'flags' have STC_TEXTURE_FLAG_CUBEMAP bit, then this value represents cube-face-index (0..STC_CUBE_FACE_COUNT)
//                              else it represents depth slice index (0..depth)
//              mip_idx: mip index (0..num_mips-1 in stc_texture_container)
//          
//          const char* stc_format_str(stc_texture_format format);
//              Converts a format enumeration to string
//
//          bool stc_format_compressed(stc_texture_format format);
//              Returns true if format is compressed
//
//      Example (for 2D textures only): 
//          int size;
//          void* dds_data = load_file("test.dds", &size);
//          assert(dds_data);
//          stc_texture_container tc = {0};
//          if (stc_parse(&tc, dds_data, size, NULL)) {
//              assert(tc.depth == 1);
//              assert(!(tc.flags & STC_TEXTURE_FLAG_CUBEMAP));
//              assert(tc.num_layers == 1);
//              // Create GPU texture from tc data
//              for (int mip = 0; mip < tc->num_mips; mip++) {
//                  stc_sub_data sub_data;
//                  stc_get_sub(&tc, &sub_data, dds_data, size, 0, 0, mip);
//                  // Fill/Set texture sub resource data (mips in this case)
//              }
//          }
//          free(dds_data);     // memory must be valid during stc_ calls
//
// Version history:
//      0.9.0       Initial release, ktx is incomplete
//      1.0.0       Api change: stc_sub_data
//                  Added KTX support
// TODO
//      Read KTX metadata. currently it just stores the offset/size to the metadata block
//
// NOTES
//      Some portions of this code are taken from 'bimg' library: https://github.com/bkaradzic/bimg
//
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
    int         size_bytes;
    int         row_pitch_bytes;
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
    STC_FORMAT_ATC,         // ATC RGB 4BPP
    STC_FORMAT_ATCE,        // ATCE RGBA 8 BPP explicit alpha
    STC_FORMAT_ATCI,        // ATCI RGBA 8 BPP interpolated alpha
    STC_FORMAT_ASTC4x4,     // ASTC 4x4 8.0 BPP
    STC_FORMAT_ASTC5x5,     // ASTC 5x5 5.12 BPP
    STC_FORMAT_ASTC6x6,     // ASTC 6x6 3.56 BPP
    STC_FORMAT_ASTC8x5,     // ASTC 8x5 3.20 BPP
    STC_FORMAT_ASTC8x6,     // ASTC 8x6 2.67 BPP
    STC_FORMAT_ASTC10x5,    // ASTC 10x5 2.56 BPP
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
    int                 metadata_offset; // ktx only
    int                 metadata_size;   // ktx only
} stc_texture_container;

typedef enum stc_cube_face
{
    STC_CUBE_FACE_X_POSITIVE = 0,
    STC_CUBE_FACE_X_NEGATIVE,
    STC_CUBE_FACE_Y_POSITIVE,
    STC_CUBE_FACE_Y_NEGATIVE,
    STC_CUBE_FACE_Z_POSITIVE,
    STC_CUBE_FACE_Z_NEGATIVE,
    STC_CUBE_FACE_COUNT
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
                         int array_idx, int slice_face_idx, int mip_idx);
STC_API const char* stc_format_str(stc_texture_format format);
STC_API bool        stc_format_compressed(stc_texture_format format);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
#ifdef STC_IMPLEMENT

#define stc__makefourcc(_a, _b, _c, _d) ( ( (uint32_t)(_a) | ( (uint32_t)(_b) << 8) | \
                                        ( (uint32_t)(_c) << 16) | ( (uint32_t)(_d) << 24) ) )

// DDS: https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide
#define STC__DDS_HEADER_SIZE 124
#define STC__DDS_MAGIC       stc__makefourcc('D', 'D', 'S', ' ')
#define STC__DDS_DXT1        stc__makefourcc('D', 'X', 'T', '1')
#define STC__DDS_DXT2        stc__makefourcc('D', 'X', 'T', '2')
#define STC__DDS_DXT3        stc__makefourcc('D', 'X', 'T', '3')
#define STC__DDS_DXT4        stc__makefourcc('D', 'X', 'T', '4')
#define STC__DDS_DXT5        stc__makefourcc('D', 'X', 'T', '5')
#define STC__DDS_ATI1        stc__makefourcc('A', 'T', 'I', '1')
#define STC__DDS_BC4U        stc__makefourcc('B', 'C', '4', 'U')
#define STC__DDS_ATI2        stc__makefourcc('A', 'T', 'I', '2')
#define STC__DDS_BC5U        stc__makefourcc('B', 'C', '5', 'U')
#define STC__DDS_DX10        stc__makefourcc('D', 'X', '1', '0')

#define STC__DDS_ETC1        stc__makefourcc('E', 'T', 'C', '1')
#define STC__DDS_ETC2        stc__makefourcc('E', 'T', 'C', '2')
#define STC__DDS_ET2A        stc__makefourcc('E', 'T', '2', 'A')
#define STC__DDS_PTC2        stc__makefourcc('P', 'T', 'C', '2')
#define STC__DDS_PTC4        stc__makefourcc('P', 'T', 'C', '4')
#define STC__DDS_ATC         stc__makefourcc('A', 'T', 'C', ' ')
#define STC__DDS_ATCE        stc__makefourcc('A', 'T', 'C', 'E')
#define STC__DDS_ATCI        stc__makefourcc('A', 'T', 'C', 'I')
#define STC__DDS_ASTC4x4     stc__makefourcc('A', 'S', '4', '4')
#define STC__DDS_ASTC5x5     stc__makefourcc('A', 'S', '5', '5')
#define STC__DDS_ASTC6x6     stc__makefourcc('A', 'S', '6', '6')
#define STC__DDS_ASTC8x5     stc__makefourcc('A', 'S', '8', '5')
#define STC__DDS_ASTC8x6     stc__makefourcc('A', 'S', '8', '6')
#define STC__DDS_ASTC10x5    stc__makefourcc('A', 'S', ':', '5')

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

typedef struct stc__ktx_header
{
    uint8_t  id[8];
    uint32_t endianess;
    uint32_t type;
    uint32_t type_size;
    uint32_t format;
    uint32_t internal_format;
    uint32_t base_internal_format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_count;
    uint32_t face_count;
    uint32_t mip_count;
    uint32_t metadata_size;
} stc__ktx_header;
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

#define stc__max(a, b)                  ((a) > (b) ? (a) : (b))
#define stc__min(a, b)                  ((a) < (b) ? (a) : (b))
#define stc__align_mask(_value, _mask)  (((_value)+(_mask)) & ((~0)&(~(_mask))))
#define stc__err(_err, _msg)            if (_err)  stc_strcpy(_err->msg, _msg);   return false

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
    { STC__DDS_ETC1,                  STC_FORMAT_ETC1,     false },
    { STC__DDS_ETC2,                  STC_FORMAT_ETC2,     false },
    { STC__DDS_ET2A,                  STC_FORMAT_ETC2A,    false },
    { STC__DDS_PTC2,                  STC_FORMAT_PTC12A,   false },
    { STC__DDS_PTC4,                  STC_FORMAT_PTC14A,   false },
    { STC__DDS_ATC ,                  STC_FORMAT_ATC,      false },
    { STC__DDS_ATCE,                  STC_FORMAT_ATCE,     false },
    { STC__DDS_ATCI,                  STC_FORMAT_ATCI,     false },
    { STC__DDS_ASTC4x4,               STC_FORMAT_ASTC4x4,  false },
    { STC__DDS_ASTC5x5,               STC_FORMAT_ASTC5x5,  false },
    { STC__DDS_ASTC6x6,               STC_FORMAT_ASTC6x6,  false },
    { STC__DDS_ASTC8x5,               STC_FORMAT_ASTC8x5,  false },
    { STC__DDS_ASTC8x6,               STC_FORMAT_ASTC8x6,  false },
    { STC__DDS_ASTC10x5,              STC_FORMAT_ASTC10x5, false },
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
    {   4, 4, 4,  8, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ATC
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ATCE
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ATCI
    {   8, 4, 4, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC4x4
    {   6, 5, 5, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC5x5
    {   4, 6, 6, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC6x6
    {   4, 8, 5, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC8x5
    {   3, 8, 6, 16, 1, 1,  0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC8x6
    {   3, 10, 5, 16, 1, 1, 0, 0,  0,  0,  0,  0, (uint8_t)(STC__ENCODE_UNORM) }, // ASTC10x5
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

// KTX: https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
#define STC__KTX_MAGIC       stc__makefourcc(0xAB, 'K', 'T', 'X')
#define STC__KTX_HEADER_SIZE 60     // actual header size is 64, but we read 4 bytes for the 'magic'

#define STC__KTX_ETC1_RGB8_OES                             0x8D64
#define STC__KTX_COMPRESSED_R11_EAC                        0x9270
#define STC__KTX_COMPRESSED_SIGNED_R11_EAC                 0x9271
#define STC__KTX_COMPRESSED_RG11_EAC                       0x9272
#define STC__KTX_COMPRESSED_SIGNED_RG11_EAC                0x9273
#define STC__KTX_COMPRESSED_RGB8_ETC2                      0x9274
#define STC__KTX_COMPRESSED_SRGB8_ETC2                     0x9275
#define STC__KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2  0x9276
#define STC__KTX_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define STC__KTX_COMPRESSED_RGBA8_ETC2_EAC                 0x9278
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC          0x9279
#define STC__KTX_COMPRESSED_RGB_PVRTC_4BPPV1_IMG           0x8C00
#define STC__KTX_COMPRESSED_RGB_PVRTC_2BPPV1_IMG           0x8C01
#define STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG          0x8C02
#define STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG          0x8C03
#define STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG          0x9137
#define STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG          0x9138
#define STC__KTX_COMPRESSED_RGB_S3TC_DXT1_EXT              0x83F0
#define STC__KTX_COMPRESSED_RGBA_S3TC_DXT1_EXT             0x83F1
#define STC__KTX_COMPRESSED_RGBA_S3TC_DXT3_EXT             0x83F2
#define STC__KTX_COMPRESSED_RGBA_S3TC_DXT5_EXT             0x83F3
#define STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT       0x8C4D
#define STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT       0x8C4E
#define STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT       0x8C4F
#define STC__KTX_COMPRESSED_LUMINANCE_LATC1_EXT            0x8C70
#define STC__KTX_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT      0x8C72
#define STC__KTX_COMPRESSED_RGBA_BPTC_UNORM_ARB            0x8E8C
#define STC__KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB      0x8E8D
#define STC__KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB      0x8E8E
#define STC__KTX_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB    0x8E8F
#define STC__KTX_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT          0x8A54
#define STC__KTX_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT          0x8A55
#define STC__KTX_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT    0x8A56
#define STC__KTX_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT    0x8A57
#define STC__KTX_ATC_RGB_AMD                               0x8C92
#define STC__KTX_ATC_RGBA_EXPLICIT_ALPHA_AMD               0x8C93
#define STC__KTX_ATC_RGBA_INTERPOLATED_ALPHA_AMD           0x87EE
#define STC__KTX_COMPRESSED_RGBA_ASTC_4x4_KHR              0x93B0
#define STC__KTX_COMPRESSED_RGBA_ASTC_5x5_KHR              0x93B2
#define STC__KTX_COMPRESSED_RGBA_ASTC_6x6_KHR              0x93B4
#define STC__KTX_COMPRESSED_RGBA_ASTC_8x5_KHR              0x93B5
#define STC__KTX_COMPRESSED_RGBA_ASTC_8x6_KHR              0x93B6
#define STC__KTX_COMPRESSED_RGBA_ASTC_10x5_KHR             0x93B8
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR      0x93D0
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR      0x93D2
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR      0x93D4
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR      0x93D5
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR      0x93D6
#define STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR     0x93D8

#define STC__KTX_A8                                        0x803C
#define STC__KTX_R8                                        0x8229
#define STC__KTX_R16                                       0x822A
#define STC__KTX_RG8                                       0x822B
#define STC__KTX_RG16                                      0x822C
#define STC__KTX_R16F                                      0x822D
#define STC__KTX_R32F                                      0x822E
#define STC__KTX_RG16F                                     0x822F
#define STC__KTX_RG32F                                     0x8230
#define STC__KTX_RGBA8                                     0x8058
#define STC__KTX_RGBA16                                    0x805B
#define STC__KTX_RGBA16F                                   0x881A
#define STC__KTX_R32UI                                     0x8236
#define STC__KTX_RG32UI                                    0x823C
#define STC__KTX_RGBA32UI                                  0x8D70
#define STC__KTX_RGBA32F                                   0x8814
#define STC__KTX_RGB565                                    0x8D62
#define STC__KTX_RGBA4                                     0x8056
#define STC__KTX_RGB5_A1                                   0x8057
#define STC__KTX_RGB10_A2                                  0x8059
#define STC__KTX_R8I                                       0x8231
#define STC__KTX_R8UI                                      0x8232
#define STC__KTX_R16I                                      0x8233
#define STC__KTX_R16UI                                     0x8234
#define STC__KTX_R32I                                      0x8235
#define STC__KTX_R32UI                                     0x8236
#define STC__KTX_RG8I                                      0x8237
#define STC__KTX_RG8UI                                     0x8238
#define STC__KTX_RG16I                                     0x8239
#define STC__KTX_RG16UI                                    0x823A
#define STC__KTX_RG32I                                     0x823B
#define STC__KTX_RG32UI                                    0x823C
#define STC__KTX_R8_SNORM                                  0x8F94
#define STC__KTX_RG8_SNORM                                 0x8F95
#define STC__KTX_RGB8_SNORM                                0x8F96
#define STC__KTX_RGBA8_SNORM                               0x8F97
#define STC__KTX_R16_SNORM                                 0x8F98
#define STC__KTX_RG16_SNORM                                0x8F99
#define STC__KTX_RGB16_SNORM                               0x8F9A
#define STC__KTX_RGBA16_SNORM                              0x8F9B
#define STC__KTX_SRGB8                                     0x8C41
#define STC__KTX_SRGB8_ALPHA8                              0x8C43
#define STC__KTX_RGBA32UI                                  0x8D70
#define STC__KTX_RGB32UI                                   0x8D71
#define STC__KTX_RGBA16UI                                  0x8D76
#define STC__KTX_RGB16UI                                   0x8D77
#define STC__KTX_RGBA8UI                                   0x8D7C
#define STC__KTX_RGB8UI                                    0x8D7D
#define STC__KTX_RGBA32I                                   0x8D82
#define STC__KTX_RGB32I                                    0x8D83
#define STC__KTX_RGBA16I                                   0x8D88
#define STC__KTX_RGB16I                                    0x8D89
#define STC__KTX_RGBA8I                                    0x8D8E
#define STC__KTX_RGB8                                      0x8051
#define STC__KTX_RGB8I                                     0x8D8F
#define STC__KTX_RGB9_E5                                   0x8C3D
#define STC__KTX_R11F_G11F_B10F                            0x8C3A

#define STC__KTX_ZERO                                      0
#define STC__KTX_RED                                       0x1903
#define STC__KTX_ALPHA                                     0x1906
#define STC__KTX_RGB                                       0x1907
#define STC__KTX_RGBA                                      0x1908
#define STC__KTX_BGRA                                      0x80E1
#define STC__KTX_RG                                        0x8227

#define STC__KTX_BYTE                                      0x1400
#define STC__KTX_UNSIGNED_BYTE                             0x1401
#define STC__KTX_SHORT                                     0x1402
#define STC__KTX_UNSIGNED_SHORT                            0x1403
#define STC__KTX_INT                                       0x1404
#define STC__KTX_UNSIGNED_INT                              0x1405
#define STC__KTX_FLOAT                                     0x1406
#define STC__KTX_HALF_FLOAT                                0x140B
#define STC__KTX_UNSIGNED_INT_5_9_9_9_REV                  0x8C3E
#define STC__KTX_UNSIGNED_SHORT_5_6_5                      0x8363
#define STC__KTX_UNSIGNED_SHORT_4_4_4_4                    0x8033
#define STC__KTX_UNSIGNED_SHORT_5_5_5_1                    0x8034
#define STC__KTX_UNSIGNED_INT_2_10_10_10_REV               0x8368
#define STC__KTX_UNSIGNED_INT_10F_11F_11F_REV              0x8C3B

typedef struct stc__ktx_format_info
{
    uint32_t internal_fmt;
    uint32_t internal_fmt_srgb;
    uint32_t fmt;
    uint32_t type;    
} stc__ktx_format_info;

typedef struct stc__ktx_format_info2
{
	uint32_t            internal_fmt;
	stc_texture_format  format;    
} stc__ktx_format_info2;

static const stc__ktx_format_info k__translate_ktx_fmt[] = {
		{ STC__KTX_COMPRESSED_RGBA_S3TC_DXT1_EXT,            STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,        STC__KTX_COMPRESSED_RGBA_S3TC_DXT1_EXT,            STC__KTX_ZERO,                         }, // BC1
		{ STC__KTX_COMPRESSED_RGBA_S3TC_DXT3_EXT,            STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,        STC__KTX_COMPRESSED_RGBA_S3TC_DXT3_EXT,            STC__KTX_ZERO,                         }, // BC2
		{ STC__KTX_COMPRESSED_RGBA_S3TC_DXT5_EXT,            STC__KTX_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,        STC__KTX_COMPRESSED_RGBA_S3TC_DXT5_EXT,            STC__KTX_ZERO,                         }, // BC3
		{ STC__KTX_COMPRESSED_LUMINANCE_LATC1_EXT,           STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_LUMINANCE_LATC1_EXT,           STC__KTX_ZERO,                         }, // BC4
		{ STC__KTX_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,     STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,     STC__KTX_ZERO,                         }, // BC5
		{ STC__KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB,     STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB,     STC__KTX_ZERO,                         }, // BC6H
		{ STC__KTX_COMPRESSED_RGBA_BPTC_UNORM_ARB,           STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_RGBA_BPTC_UNORM_ARB,           STC__KTX_ZERO,                         }, // BC7
		{ STC__KTX_ETC1_RGB8_OES,                            STC__KTX_ZERO,                                       STC__KTX_ETC1_RGB8_OES,                            STC__KTX_ZERO,                         }, // ETC1
		{ STC__KTX_COMPRESSED_RGB8_ETC2,                     STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_RGB8_ETC2,                     STC__KTX_ZERO,                         }, // ETC2
		{ STC__KTX_COMPRESSED_RGBA8_ETC2_EAC,                STC__KTX_COMPRESSED_SRGB8_ETC2,                      STC__KTX_COMPRESSED_RGBA8_ETC2_EAC,                STC__KTX_ZERO,                         }, // ETC2A
		{ STC__KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, STC__KTX_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,  STC__KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, STC__KTX_ZERO,                         }, // ETC2A1
		{ STC__KTX_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,          STC__KTX_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT,           STC__KTX_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,          STC__KTX_ZERO,                         }, // PTC12
		{ STC__KTX_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,          STC__KTX_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT,           STC__KTX_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,          STC__KTX_ZERO,                         }, // PTC14
		{ STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,         STC__KTX_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT,     STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,         STC__KTX_ZERO,                         }, // PTC12A
		{ STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,         STC__KTX_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT,     STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,         STC__KTX_ZERO,                         }, // PTC14A
		{ STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,         STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG,         STC__KTX_ZERO,                         }, // PTC22
		{ STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,         STC__KTX_ZERO,                                       STC__KTX_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG,         STC__KTX_ZERO,                         }, // PTC24
		{ STC__KTX_ATC_RGB_AMD,                              STC__KTX_ZERO,                                       STC__KTX_ATC_RGB_AMD,                              STC__KTX_ZERO,                         }, // ATC
		{ STC__KTX_ATC_RGBA_EXPLICIT_ALPHA_AMD,              STC__KTX_ZERO,                                       STC__KTX_ATC_RGBA_EXPLICIT_ALPHA_AMD,              STC__KTX_ZERO,                         }, // ATCE
		{ STC__KTX_ATC_RGBA_INTERPOLATED_ALPHA_AMD,          STC__KTX_ZERO,                                       STC__KTX_ATC_RGBA_INTERPOLATED_ALPHA_AMD,          STC__KTX_ZERO,                         }, // ATCI
		{ STC__KTX_COMPRESSED_RGBA_ASTC_4x4_KHR,             STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,       STC__KTX_COMPRESSED_RGBA_ASTC_4x4_KHR,             STC__KTX_ZERO,                         }, // ASTC4x4
		{ STC__KTX_COMPRESSED_RGBA_ASTC_5x5_KHR,             STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,       STC__KTX_COMPRESSED_RGBA_ASTC_5x5_KHR,             STC__KTX_ZERO,                         }, // ASTC5x5
		{ STC__KTX_COMPRESSED_RGBA_ASTC_6x6_KHR,             STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,       STC__KTX_COMPRESSED_RGBA_ASTC_6x6_KHR,             STC__KTX_ZERO,                         }, // ASTC6x6
		{ STC__KTX_COMPRESSED_RGBA_ASTC_8x5_KHR,             STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,       STC__KTX_COMPRESSED_RGBA_ASTC_8x5_KHR,             STC__KTX_ZERO,                         }, // ASTC8x5
		{ STC__KTX_COMPRESSED_RGBA_ASTC_8x6_KHR,             STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,       STC__KTX_COMPRESSED_RGBA_ASTC_8x6_KHR,             STC__KTX_ZERO,                         }, // ASTC8x6
		{ STC__KTX_COMPRESSED_RGBA_ASTC_10x5_KHR,            STC__KTX_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,      STC__KTX_COMPRESSED_RGBA_ASTC_10x5_KHR,            STC__KTX_ZERO,                         }, // ASTC10x5
		{ STC__KTX_ZERO,                                     STC__KTX_ZERO,                                       STC__KTX_ZERO,                                     STC__KTX_ZERO,                         }, // Unknown
		{ STC__KTX_ALPHA,                                    STC__KTX_ZERO,                                       STC__KTX_ALPHA,                                    STC__KTX_UNSIGNED_BYTE,                }, // A8
		{ STC__KTX_R8,                                       STC__KTX_ZERO,                                       STC__KTX_RED,                                      STC__KTX_UNSIGNED_BYTE,                }, // R8
		{ STC__KTX_RGBA8,                                    STC__KTX_SRGB8_ALPHA8,                               STC__KTX_RGBA,                                     STC__KTX_UNSIGNED_BYTE,                }, // RGBA8
		{ STC__KTX_RGBA8_SNORM,                              STC__KTX_ZERO,                                       STC__KTX_RGBA,                                     STC__KTX_BYTE,                         }, // RGBA8S
		{ STC__KTX_RG16,                                     STC__KTX_ZERO,                                       STC__KTX_RG,                                       STC__KTX_UNSIGNED_SHORT,               }, // RG16
		{ STC__KTX_RGB8,                                     STC__KTX_SRGB8,                                      STC__KTX_RGB,                                      STC__KTX_UNSIGNED_BYTE,                }, // RGB8
		{ STC__KTX_R16,                                      STC__KTX_ZERO,                                       STC__KTX_RED,                                      STC__KTX_UNSIGNED_SHORT,               }, // R16
		{ STC__KTX_R32F,                                     STC__KTX_ZERO,                                       STC__KTX_RED,                                      STC__KTX_FLOAT,                        }, // R32F
		{ STC__KTX_R16F,                                     STC__KTX_ZERO,                                       STC__KTX_RED,                                      STC__KTX_HALF_FLOAT,                   }, // R16F
		{ STC__KTX_RG16F,                                    STC__KTX_ZERO,                                       STC__KTX_RG,                                       STC__KTX_FLOAT,                        }, // RG16F
		{ STC__KTX_RG16_SNORM,                               STC__KTX_ZERO,                                       STC__KTX_RG,                                       STC__KTX_SHORT,                        }, // RG16S
		{ STC__KTX_RGBA16F,                                  STC__KTX_ZERO,                                       STC__KTX_RGBA,                                     STC__KTX_HALF_FLOAT,                   }, // RGBA16F
		{ STC__KTX_RGBA16,                                   STC__KTX_ZERO,                                       STC__KTX_RGBA,                                     STC__KTX_UNSIGNED_SHORT,               }, // RGBA16
		{ STC__KTX_BGRA,                                     STC__KTX_SRGB8_ALPHA8,                               STC__KTX_BGRA,                                     STC__KTX_UNSIGNED_BYTE,                }, // BGRA8
		{ STC__KTX_RGB10_A2,                                 STC__KTX_ZERO,                                       STC__KTX_RGBA,                                     STC__KTX_UNSIGNED_INT_2_10_10_10_REV,  }, // RGB10A2
		{ STC__KTX_R11F_G11F_B10F,                           STC__KTX_ZERO,                                       STC__KTX_RGB,                                      STC__KTX_UNSIGNED_INT_10F_11F_11F_REV, }, // RG11B10F
		{ STC__KTX_RG8,                                      STC__KTX_ZERO,                                       STC__KTX_RG,                                       STC__KTX_UNSIGNED_BYTE,                }, // RG8
		{ STC__KTX_RG8_SNORM,                                STC__KTX_ZERO,                                       STC__KTX_RG,                                       STC__KTX_BYTE,                         }  // RG8S
};

static const stc__ktx_format_info2 k__translate_ktx_fmt2[] =
{
    { STC__KTX_A8,                           STC_FORMAT_A8    },
    { STC__KTX_RED,                          STC_FORMAT_R8    },
    { STC__KTX_RGB,                          STC_FORMAT_RGB8  },
    { STC__KTX_RGBA,                         STC_FORMAT_RGBA8 },
    { STC__KTX_COMPRESSED_RGB_S3TC_DXT1_EXT, STC_FORMAT_BC1   },
};

typedef struct stc__format_info
{
    const char* name;
    bool        has_alpha;
} stc__format_info;

static const stc__format_info k__formats_info[] = {
    {"BC1", false},
    {"BC2", true},
    {"BC3", true},
    {"BC4", false},
    {"BC5", false},
    {"BC6H", false},
    {"BC7", true},
    {"ETC1", false},
    {"ETC2", false},
    {"ETC2A", true},
    {"ETC2A1", true},
    {"PTC12", false},
    {"PTC14", false},
    {"PTC12A", true},
    {"PTC14A", true},
    {"PTC22", true},
    {"PTC24", true},
    {"ATC", false},
    {"ATCE", false},
    {"ATCI", false},
    {"ASTC4x4", true},
    {"ASTC5x5", true},
    {"ASTC6x6", false},
    {"ASTC8x5", true},
    {"ASTC8x6", false},
    {"ASTC10x5", false},
    {"<unknown>", false},
    {"A8", true},
    {"R8", false},
    {"RGBA8", true},
    {"RGBA8S", true},
    {"RG16", false},
    {"RGB8", false},
    {"R16", false},
    {"R32F", false},
    {"R16F", false},
    {"RG16F", false},
    {"RG16S", false},
    {"RGBA16F", true},
    {"RGBA16",true},
    {"BGRA8", true},
    {"RGB10A2", true},
    {"RG11B10F", false},
    {"RG8", false},
    {"RG8S", false}
};


static inline int stc__read(stc__mem_reader* reader, void* buff, int size)
{
    int read_bytes = (reader->offset + size) <= reader->total ? size : (reader->total - reader->offset);
    stc_memcpy(buff, reader->buff + reader->offset, read_bytes);
    reader->offset += read_bytes;
    return read_bytes;
}

static bool stc__parse_ktx(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
{
    stc_memset(tc, 0x0, sizeof(stc_texture_container));

    stc__mem_reader r = {(const uint8_t*)file_data, size, sizeof(uint32_t)};
    stc__ktx_header header;
    if (stc__read(&r, &header, sizeof(header)) != STC__KTX_HEADER_SIZE) {
        stc__err(err, "ktx; header size does not match");
    }

    if (header.id[1] != '1' && header.id[2] != '1')    {
        stc__err(err, "ktx: invalid file header");
    }

    // TODO: support little endian
    if (header.endianess == 0x04030201) {
        stc__err(err, "ktx: little-endian format is not supported");
    }

    tc->metadata_offset = r.offset;
    tc->metadata_size = (int)header.metadata_size;
    r.offset += (int)header.metadata_size;

    stc_texture_format format = _STC_FORMAT_COUNT;

    int count = sizeof(k__translate_ktx_fmt)/sizeof(stc__ktx_format_info);
    for (int i = 0; i < count; i++) {
        if (k__translate_ktx_fmt[i].internal_fmt == header.internal_format) {
            format = (stc_texture_format)i;
            break;
        }
    }

    if (format == _STC_FORMAT_COUNT) {
        count = sizeof(k__translate_ktx_fmt2)/sizeof(stc__ktx_format_info2);
        for (int i = 0; i < count; i++) {
            if (k__translate_ktx_fmt2[i].internal_fmt == header.internal_format) {
                format = (stc_texture_format)k__translate_ktx_fmt2[i].format;
                break;
            }
        }
    }

    if (format == _STC_FORMAT_COUNT) {
        stc__err(err, "ktx: unsupported format");
    } 

    if (header.face_count > 1 && header.face_count != STC_CUBE_FACE_COUNT) {
        stc__err(err, "ktx: incomplete cubemap");
    }

    tc->data_offset = r.offset;
    tc->size_bytes = r.total - r.offset;
    tc->format = format;
    tc->width = (int)header.width;
    tc->height = (int)header.height;
    tc->depth = stc__max((int)header.depth, 1);
    tc->num_layers = stc__max((int)header.array_count, 1);
    tc->num_mips = stc__max((int)header.mip_count, 1);
    tc->bpp = k__block_info[format].bpp;

    if (header.face_count > 1)
        tc->flags |= STC_TEXTURE_FLAG_CUBEMAP;
    tc->flags |= k__formats_info[format].has_alpha ? STC_TEXTURE_FLAG_ALPHA : 0;
    tc->flags |= STC_TEXTURE_FLAG_KTX;

    return false;
}

static bool stc__parse_dds(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
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
    tc->num_layers = stc__max(1, (int)array_size);
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

void stc_get_sub(const stc_texture_container* tc, stc_sub_data* sub_data, 
                 const void* file_data, int size,
                 int array_idx, int slice_face_idx, int mip_idx)
{
    stc_assert(tc);
    stc_assert(sub_data);
    stc_assert(file_data);
    stc_assert(size > 0);
    stc_assert(array_idx < tc->num_layers);
    stc_assert(!((tc->flags&STC_TEXTURE_FLAG_CUBEMAP) && (slice_face_idx >= STC_CUBE_FACE_COUNT)) && "invalid cube-face index");
    stc_assert(!(!(tc->flags&STC_TEXTURE_FLAG_CUBEMAP) && (slice_face_idx >= tc->depth)) && "invalid depth-slice index");
    stc_assert(mip_idx < tc->num_mips);

    stc__mem_reader r = { (uint8_t*)file_data, size, tc->data_offset };
    stc_texture_format format = tc->format;
    bool has_alpha = (tc->flags & STC_TEXTURE_FLAG_ALPHA) ? true : false;

    stc_assert(format < _STC_FORMAT_COUNT && format != _STC_FORMAT_COMPRESSED);
    const stc__block_info* binfo = &k__block_info[format];
    const int bpp          = binfo->bpp;
    const int block_size   = binfo->block_size;
    const int block_width  = binfo->block_width;
    const int block_height = binfo->block_height;
    const int min_block_x  = binfo->min_block_x;
    const int min_block_y  = binfo->min_block_y;

    int num_faces;
    const int min_width = min_block_x*block_width;
    const int min_height = min_block_y*block_height;

    stc_assert(!((tc->flags & STC_TEXTURE_FLAG_CUBEMAP) && tc->depth > 1) && "textures must be either Cube or 3D");
    int slice_idx, face_idx, num_slices;
    if (tc->flags & STC_TEXTURE_FLAG_CUBEMAP) {
        slice_idx = 0;
        face_idx = slice_face_idx;
        num_faces = STC_CUBE_FACE_COUNT;
        num_slices = 1;
    } else {
        slice_idx = slice_face_idx;
        face_idx = 0;
        num_faces = 1;
        num_slices = tc->depth;
    }    

    if (tc->flags & STC_TEXTURE_FLAG_DDS) {
        for (int layer = 0, num_layers = tc->num_layers; layer < num_layers; layer++) {
            for (int face = 0; face < num_faces; face++) {
                int width = tc->width;
                int height = tc->height;

                for (int mip = 0, mip_count = tc->num_mips; mip < mip_count; mip++) {
                    width = ((width + block_width - 1)/block_width)*block_width;
                    height = ((height + block_height - 1)/block_height)*block_height;
                    width = stc__max(min_width, width);
                    height = stc__max(min_height, height);
                    int mip_size = width/block_width * height/block_height * block_size;
                    stc_assert(width*height*bpp/8 == mip_size);

                    for (int slice = 0; slice < num_slices; slice++) {
                        if (layer == array_idx && mip == mip_idx && 
                            slice == slice_idx && face_idx == face) 
                        {
                            sub_data->buff = r.buff + r.offset;
                            sub_data->width = width;
                            sub_data->height = height;
                            sub_data->size_bytes = mip_size;
                            sub_data->row_pitch_bytes = width*bpp/8;
                            return;
                        }

                        r.offset += mip_size;
                        stc_assert(r.offset <= r.total && "texture buffer overflow");
                    } // foreach slice

                    width >>= 1;
                    height >>= 1;
                }   // foreach mip
            }   // foreach face
        } // foreach array-item
    } else if (tc->flags & STC_TEXTURE_FLAG_KTX) {
        int width = tc->width;
        int height = tc->height;

        for (int mip = 0, c = tc->num_mips; mip < c; mip++) {
            width = ((width  + block_width  - 1) / block_width)*block_width;
            height = ((height + block_height - 1) / block_height)*block_height;
            width  = stc__max(min_width, width);
            height = stc__max(min_height, height);
            int mip_size = width/block_width * height/block_height * block_size;
            stc_assert(width*height*bpp/8 == mip_size);

            int image_size;
            stc__read(&r, &image_size, sizeof(image_size)); 
            stc_assert(image_size == (mip_size*num_faces*num_slices) && "image size mismatch");

            for (int layer = 0, num_layers = tc->num_layers; layer < num_layers; layer++) {
                for (int face = 0; face < num_faces; face++) {
                    for (int slice = 0; slice < num_slices; slice++) {
                        if (layer == array_idx && mip == mip_idx &&
                            slice == slice_idx && face_idx == face) 
                        {
                            sub_data->buff = r.buff + r.offset;
                            sub_data->width = width;
                            sub_data->height = height;
                            sub_data->size_bytes = mip_size;
                            sub_data->row_pitch_bytes = width*bpp/8;
                            return;
                        }

                        r.offset += mip_size;
                        stc_assert(r.offset <= r.total && "texture buffer overflow");
                    }   // foreach slice

                    
                    r.offset = stc__align_mask(r.offset, 3); // cube-padding
                }   // foreach face
            }   // foreach array-item

            width >>= 1;
            height >>= 1;
            
            r.offset = stc__align_mask(r.offset, 3); // mip-padding
        }   // foreach mip     
    } else {
        sx_assert(0 && "invalid file format");
    }
}

bool stc_parse(stc_texture_container* tc, const void* file_data, int size, stc_error* err)
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

const char* stc_format_str(stc_texture_format format)
{
    return k__formats_info[format].name;
}

bool stc_format_compressed(stc_texture_format format)
{
    stc_assert(format != _STC_FORMAT_COMPRESSED && format != _STC_FORMAT_COUNT);
    return format < _STC_FORMAT_COMPRESSED;
}

#endif  // STC_IMPLEMENT

