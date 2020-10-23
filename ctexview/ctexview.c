
#if defined(_WIN32) || defined(_WIN64)
#   define SOKOL_D3D11
#   define _CRT_SECURE_NO_WARNINGS
#   include "quad_hlsl.vert.h"
#   include "quad_hlsl.frag.h"
#elif defined(__linux__)
#   include "quad_glsl.vert.h"
#   include "quad_glsl.frag.h"
#   define SOKOL_GLCORE33
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#   include "quad_metal.vert.h"
#   include "quad_metal.frag.h"
#   define SOKOL_METAL
#endif

#define SOKOL_IMPL
#define DDSKTX_IMPLEMENT

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#include "../dds-ktx.h"

#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"

#define FONT_SCALE 1.1f

typedef struct uniforms 
{
    float color[4];
    float args[4];
} uniforms;


typedef struct ctexview_state
{
    sg_pass_action pass_action;
    void* file_data;
    int file_size;
    ddsktx_texture_info texinfo;
    sg_image tex;
    sg_shader shader;
    sg_pipeline pip;
    sg_buffer vb;
    sg_buffer ib;
    sg_image checker;
    bool inv_text_color;
    uniforms vars;
    int cur_mip;
} ctexview_state;

ctexview_state g_state;

typedef struct vertex
{
    float x;
    float y;
    float u;
    float v;
} vertex;

static int nearest_pow2(int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

static sg_image create_checker_texture(int checker_size, int size, uint32_t colors[2])
{
    assert(size % 4 == 0 && "size must be multiple of four");
    assert(size % checker_size == 0 && "checker_size must be dividable by size");

    int size_bytes = size * size * sizeof(uint32_t);
    uint32_t* pixels = malloc(size_bytes);
    assert(pixels);

    // split into tiles and color them
    int tiles_x = size / checker_size;
    int tiles_y = size / checker_size;
    int num_tiles = tiles_x * tiles_y;

    int* poss = malloc(sizeof(int) * 2 * num_tiles);
    assert(poss);
    int _x = 0, _y = 0;
    for (int i = 0; i < num_tiles; i++) {
        poss[i*2] = _x;
        poss[i*2 + 1] = _y;
        _x += checker_size;
        if (_x >= size) {
            _x = 0;
            _y += checker_size;
        }
    }

    int color_idx = 0;
    for (int i = 0; i < num_tiles; i++) {
        int* p = poss + i*2;
        uint32_t c = colors[color_idx];
        if (i == 0 || ((i + 1) % tiles_x) != 0)
            color_idx = !color_idx;
        int end_x = p[0] + checker_size;
        int end_y = p[1] + checker_size;
        for (int y = p[1]; y < end_y; y++) {
            for (int x = p[0]; x < end_x; x++) {
                int pixel = x + y * size;
                pixels[pixel] = c;
            }
        }
    }

    sg_image tex = sg_make_image(&(sg_image_desc) {
        .width = size,
        .height = size,
        .num_mipmaps = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .content = (sg_image_content){.subimage[0][0].ptr = pixels,
                                      .subimage[0][0].size = size_bytes }
    });

    free(poss);
    free(pixels);

    return tex;
}


static void print_msg(const char* fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    #if defined(_WIN32) || defined(_WIN64)
        MessageBoxA(NULL, msg, "DDS/KTX viewer", MB_OK);
    #else
        puts(msg);
    #endif
}

static void init(void) 
{
    sg_setup(&(sg_desc) {
        .context = sapp_sgcontext()
    });

    g_state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = {0, 0, 0, 1.0f} }
    };

    static vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },
        { 1.0f,  -1.0f, 1.0f, 1.0f },
        { 1.0f,   1.0f, 1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f, 0.0f },
    };

    static uint16_t indices[] = { 0, 2, 1, 2, 0, 3 };

    g_state.vb = sg_make_buffer(&(sg_buffer_desc) {
        .usage = SG_USAGE_IMMUTABLE,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .size = sizeof(vertices),
        .content = vertices
    });

    g_state.ib = sg_make_buffer(&(sg_buffer_desc) {
        .usage = SG_USAGE_IMMUTABLE,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .content = indices
    });

    g_state.shader = sg_make_shader(&(sg_shader_desc) {
        .attrs = {
            [0] = { .name = "a_pos", .sem_name = "POSITION" },
            [1] = { .name = "a_uv", .sem_name = "TEXCOORD" }
        },
        .vs = { 
        #ifdef SOKOL_D3D11
            .byte_code = (const uint8_t*)quad_vs_data, 
            .byte_code_size = quad_vs_size,
        #else
            .source = (const char*)quad_vs_data,
        #endif
        #ifdef SOKOL_METAL
            .entry = "main0"
        #endif
        },
        .fs = { 
        #ifdef SOKOL_D3D11
            .byte_code = (const uint8_t*)quad_fs_data, 
            .byte_code_size = quad_fs_size,
        #else
            .source = (const char*)quad_fs_data,
        #endif
        #ifdef SOKOL_METAL
            .entry = "main0",
        #endif
            .images[0] = {
                .name = "tex_image",
                .type = SG_IMAGETYPE_2D
            },
            .uniform_blocks[0] = {
                .size = sizeof(uniforms),
                .uniforms =  {
                    [0] = { .name = "color", .type = SG_UNIFORMTYPE_FLOAT4 },
                    [1] = { .name = "target_lod", SG_UNIFORMTYPE_FLOAT4 }
                }
            }
        }
    });

    g_state.pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .layout = {
            .buffers[0] = {
                .stride = sizeof(vertex),
            },
            .attrs = {
                [0] = { .offset = 0, .format = SG_VERTEXFORMAT_FLOAT2 },
                [1] = { .offset = 8, .format = SG_VERTEXFORMAT_FLOAT2 }
            }
        },
        .shader = g_state.shader,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .rasterizer = {
            .cull_mode = SG_CULLMODE_BACK
        },
        .blend = { 
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA 
        }
    });

    // main texture (dds-ktx)
    sg_image_desc desc = {
        .type = SG_IMAGETYPE_2D,
        .width = g_state.texinfo.width,
        .height = g_state.texinfo.height,
        .depth = 1,
        .num_mipmaps = g_state.texinfo.num_mips,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST
    };

    switch (g_state.texinfo.format) {
    case DDSKTX_FORMAT_BC1:     desc.pixel_format = SG_PIXELFORMAT_BC1_RGBA; break;
    case DDSKTX_FORMAT_BC2:     desc.pixel_format = SG_PIXELFORMAT_BC2_RGBA; break;
    case DDSKTX_FORMAT_BC3:     desc.pixel_format = SG_PIXELFORMAT_BC3_RGBA; break;
    case DDSKTX_FORMAT_BC4:     desc.pixel_format = SG_PIXELFORMAT_BC4_R; break;
    case DDSKTX_FORMAT_BC5:     desc.pixel_format = SG_PIXELFORMAT_BC5_RG; break;
    case DDSKTX_FORMAT_BC6H:    desc.pixel_format = SG_PIXELFORMAT_BC6H_RGBF; break;
    case DDSKTX_FORMAT_BC7:     desc.pixel_format = SG_PIXELFORMAT_BC7_RGBA; break;
    case DDSKTX_FORMAT_A8:      
    case DDSKTX_FORMAT_R8:      desc.pixel_format = SG_PIXELFORMAT_R8; break;
    case DDSKTX_FORMAT_RGBA8:
    case DDSKTX_FORMAT_RGBA8S:   desc.pixel_format = SG_PIXELFORMAT_RGBA8; break;
    case DDSKTX_FORMAT_RG16:     desc.pixel_format = SG_PIXELFORMAT_RG16; break;
    case DDSKTX_FORMAT_RGB8:     desc.pixel_format = SG_PIXELFORMAT_RGBA8; break;
    case DDSKTX_FORMAT_R16:      desc.pixel_format = SG_PIXELFORMAT_R16; break;
    case DDSKTX_FORMAT_R32F:     desc.pixel_format = SG_PIXELFORMAT_R32F; break;
    case DDSKTX_FORMAT_R16F:     desc.pixel_format = SG_PIXELFORMAT_R16F; break;
    case DDSKTX_FORMAT_RG16F:    desc.pixel_format = SG_PIXELFORMAT_RG16F; break;
    case DDSKTX_FORMAT_RG16S:    desc.pixel_format = SG_PIXELFORMAT_RG16; break;
    case DDSKTX_FORMAT_RGBA16F:  desc.pixel_format = SG_PIXELFORMAT_RGBA16F; break;
    case DDSKTX_FORMAT_RGBA16:   desc.pixel_format = SG_PIXELFORMAT_RGBA16; break;
    case DDSKTX_FORMAT_BGRA8:    desc.pixel_format = SG_PIXELFORMAT_BGRA8; break;
    case DDSKTX_FORMAT_RGB10A2:  desc.pixel_format = SG_PIXELFORMAT_RGB10A2; break;
    case DDSKTX_FORMAT_RG11B10F: desc.pixel_format = SG_PIXELFORMAT_RG11B10F; break;
    case DDSKTX_FORMAT_RG8:      desc.pixel_format = SG_PIXELFORMAT_RG8; break;
    case DDSKTX_FORMAT_RG8S:     desc.pixel_format = SG_PIXELFORMAT_RG8; break;
    default:    assert(0); exit(-1);
    }    

    for (int mip = 0; mip < g_state.texinfo.num_mips; mip++) {
        ddsktx_sub_data subdata;
        ddsktx_get_sub(&g_state.texinfo, &subdata, g_state.file_data, g_state.file_size, 0, 0, mip);
        desc.content.subimage[0][mip].ptr = subdata.buff;
        desc.content.subimage[0][mip].size = subdata.size_bytes;
    }

    g_state.tex = sg_make_image(&desc);

    sdtx_setup(&(sdtx_desc_t) {
        .fonts = {
            [0] = sdtx_font_c64(),
        },
    });
    sdtx_set_context(SDTX_DEFAULT_CONTEXT);
    sdtx_canvas((float)sapp_width() * (1.0f/FONT_SCALE), (float)sapp_height() * (1.0f/FONT_SCALE));

    uint32_t checker_colors[] = {
        0xff999999,
        0xff666666
    };

    if (g_state.texinfo.flags & DDSKTX_TEXTURE_FLAG_ALPHA) {
        g_state.checker = create_checker_texture(8,
                                                 g_state.texinfo.width > g_state.texinfo.height ?
                                                 nearest_pow2(g_state.texinfo.width) :
                                                 nearest_pow2(g_state.texinfo.height),
                                                 checker_colors);
    }

    g_state.vars.color[0] = g_state.vars.color[1] = g_state.vars.color[2] = g_state.vars.color[3] = 1.0f;
}

static void frame(void) 
{
    sdtx_home();
    sdtx_origin(1, 1);
    sdtx_pos(0, 0);
    sdtx_color3b(!g_state.inv_text_color ? 255 : 0, !g_state.inv_text_color ? 255 : 0, 0);

    sdtx_printf("%s\t%dx%d (mip %d/%d)", 
                ddsktx_format_str(g_state.texinfo.format), g_state.texinfo.width, 
                g_state.texinfo.height, g_state.cur_mip, g_state.texinfo.num_mips);
    sdtx_crlf();
    sdtx_printf("mask: %c%c%c%c\t", 
                g_state.vars.color[0] == 1.0f ? 'R' : 'X',
                g_state.vars.color[1] == 1.0f ? 'G' : 'X',
                g_state.vars.color[2] == 1.0f ? 'B' : 'X',
                g_state.vars.color[3] == 1.0f ? 'A' : 'X');
    sdtx_crlf();

    g_state.vars.args[0] = (float)g_state.cur_mip;

    sg_begin_default_pass(&g_state.pass_action, sapp_width(), sapp_height());
    if (g_state.tex.id) {
        sg_bindings bindings = {
            .vertex_buffers[0] = g_state.vb,
            .index_buffer = g_state.ib,
        };

        if (g_state.checker.id) {
            bindings.fs_images[0] = g_state.checker;
            uniforms u = {
                {1.0f, 1.0f, 1.0f, 1.0f},
                {0, 0, 0, 0}
            };

            sg_apply_pipeline(g_state.pip);
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &u, sizeof(u));
            sg_apply_bindings(&bindings);
            sg_draw(0, 6, 1);
        }


        bindings.fs_images[0] = g_state.tex;
        sg_apply_pipeline(g_state.pip);
        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &g_state.vars, sizeof(g_state.vars));
        sg_apply_bindings(&bindings);
        sg_draw(0, 6, 1);
    }

    sdtx_draw();

    sg_end_pass();
    sg_commit();
}

static void release(void)
{
    free(g_state.file_data);
    sdtx_shutdown();
    sg_shutdown();
}

static void on_events(const sapp_event* e)
{
    switch (e->type) {
    case SAPP_EVENTTYPE_RESIZED:
        sdtx_canvas((float)sapp_width() * (1.0f/FONT_SCALE), (float)sapp_height() * (1.0f/FONT_SCALE));
        break;

    case SAPP_EVENTTYPE_KEY_DOWN:
        if (e->key_code == SAPP_KEYCODE_GRAVE_ACCENT) {
            g_state.inv_text_color = !g_state.inv_text_color;
        }
        if (e->key_code == SAPP_KEYCODE_A) {
            g_state.vars.color[3] = g_state.vars.color[3] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_R) {
            g_state.vars.color[0] = g_state.vars.color[0] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_G) {
            g_state.vars.color[1] = g_state.vars.color[1] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_B) {
            g_state.vars.color[2] = g_state.vars.color[2] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_UP) {
            g_state.cur_mip = (g_state.cur_mip + 1) >= g_state.texinfo.num_mips ? (g_state.texinfo.num_mips - 1) : g_state.cur_mip + 1;
        }
        if (e->key_code == SAPP_KEYCODE_DOWN) {
            g_state.cur_mip = (g_state.cur_mip > 0) ? g_state.cur_mip - 1 : 0;
        }

        break;
    }
}

sapp_desc sokol_main(int argc, char* argv[]) 
{
    if (argc <= 1) {
        print_msg("Provide a file to load as argument");
        exit(-1);
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        print_msg("Error: could not open file: %s\n", argv[1]);
        exit(-1);
    }

    fseek(f, 0, SEEK_END);
    int size = (int)ftell(f);
    if (size == 0) {
        print_msg("Error: file '%s' is empty\n", argv[1]);
        exit(-1);
    }
    fseek(f, 0, SEEK_SET);

    void* data = malloc(size);
    if (!data) {
        print_msg("out of memory: requested size: %d\n", (int)size);
        exit(-1);
    }
    
    if (fread(data, 1, size, f) != size) {
        print_msg("could not read file data : %s\n", argv[1]);
        exit(-1);
    }

    g_state.file_data = data;
    g_state.file_size = size;

    fclose(f);

    ddsktx_texture_info tc = {0};
    ddsktx_error img_err;
    if (!ddsktx_parse(&tc, data, size, &img_err)) {
        print_msg("Loading image '%s' failed: %s", argv[1], img_err.msg);
        exit(-1);
    } 

    g_state.texinfo = tc;

    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = release,
        .event_cb = on_events,
        .width = tc.width,
        .height = tc.height,
        .window_title = "DDS/KTX viewer",
        .swap_interval = 1,
    };
}
