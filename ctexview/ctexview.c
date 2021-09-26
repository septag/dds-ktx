
#if defined(_WIN32) || defined(_WIN64)
#   define SOKOL_D3D11
#   define _CRT_SECURE_NO_WARNINGS
#   include "quad_hlsl.vert.h"
#   include "quad_hlsl.frag.h"
#   include "quad_cubemap_hlsl.frag.h"
#   define SOKOL_LOG(s) OutputDebugStringA(s)
#elif defined(__linux__)
#   include "quad_glsl.vert.h"
#   include "quad_glsl.frag.h"
#   include "quad_cubemap_glsl.frag.h"
#   define SOKOL_GLCORE33
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#   include "quad_metal.vert.h"
#   include "quad_metal.frag.h"
#   include "quad_cubemap_metal.frag.h"
#   define SOKOL_METAL
#endif

#define SOKOL_IMPL
#define DDSKTX_IMPLEMENT

#define SOKOL_API_DECL static
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#define DDSKTX_API static
#include "../dds-ktx.h"

#ifndef __APPLE__
#   include <malloc.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"

#define FONT_SCALE 1.1f
#define CHECKER_SIZE 8

typedef struct uniforms_fs 
{
    float color[4];
    float args[4];
} uniforms_fs;

typedef struct uniforms_vs
{
    float proj_mat[16];
} uniforms_vs;

typedef struct vertex
{
    float x;
    float y;
    float u;
    float v;
    float w; // reserved for cubemapping
} vertex;


typedef struct ctexview_state
{
    sg_pass_action pass_action;
    void* file_data;
    int file_size;
    ddsktx_texture_info texinfo;
    sg_image tex;
    sg_shader shader;
    sg_shader shader_cubemap;
    sg_pipeline pip;
    sg_pipeline pip_cubemap;
    sg_pipeline pip_checker;
    sg_buffer vb;
    sg_buffer ib;
    sg_buffer vb_checker;
    sg_image checker;
    bool inv_text_color;
    uniforms_fs vars_fs;
    int cur_mip;
    int cur_slice;
    int cube_face;
} ctexview_state;

ctexview_state g_state;

static const vertex k_vertices[] = {
    { -1.0f, -1.0f, 0.0f, 1.0f, 0 },
    {  1.0f, -1.0f, 1.0f, 1.0f, 0 },
    {  1.0f,  1.0f, 1.0f, 0.0f, 0 },
    { -1.0f,  1.0f, 0.0f, 0.0f, 0 },
};

static const uint16_t k_indices[] = { 0, 2, 1, 2, 0, 3 };

static const char* k_cube_face_names[DDSKTX_CUBE_FACE_COUNT] = {
    "X+",
    "X-",
    "Y+",
    "Y-",
    "Z+",
    "Z-"
};

#if defined(_WIN32) || defined(_WIN64)
static desktop_size(int* width, int* height)
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    *width = desktop.right;
    *height = desktop.bottom;
}
#endif

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

// https://en.wikipedia.org/wiki/Cube_mapping
static void convert_cube_uv_to_xyz(int index, float u, float v, float* x, float* y, float* z)
{
    // convert range 0 to 1 to -1 to 1
    float uc = 2.0f * u - 1.0f;
    float vc = 2.0f * v - 1.0f;
    switch (index) {
    case 0: *x = 1.0f; *y = vc; *z = -uc; break;	// POSITIVE X
    case 1: *x = -1.0f; *y = vc; *z = uc; break;	// NEGATIVE X
    case 2: *x = uc; *y = 1.0f; *z = -vc; break;	// POSITIVE Y
    case 3: *x = uc; *y = -1.0f; *z = vc; break;	// NEGATIVE Y
    case 4: *x = uc; *y = vc; *z = 1.0f; break;	    // POSITIVE Z
    case 5: *x = -uc; *y = vc; *z = -1.0f; break;	// NEGATIVE Z
    }
}

static void set_cube_face(int index)
{
    if (g_state.texinfo.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP) {
        assert(index >= 0 && index < DDSKTX_CUBE_FACE_COUNT);

        vertex vertices[4];
        memcpy(vertices, k_vertices, sizeof(k_vertices));

        for (int i = 0; i < 4; i++) {
            float x, y, z;
            convert_cube_uv_to_xyz(index, vertices[i].u, vertices[i].v, &x, &y, &z);
            vertices[i].u = x;
            vertices[i].v = y;
            vertices[i].w = z;
        }

        sg_update_buffer(g_state.vb, vertices, sizeof(vertices));
    }
}

static void adjust_checker_coords(int width, int height)
{
    int count_x = width/CHECKER_SIZE;
    int count_y = height/CHECKER_SIZE;

    float ratio = (float)width / (float)height;
    float u, v;
    if (width > height) {
        u = (float)count_x;
        v = (float)count_y * ratio;
    }
    else {
        v = (float)count_y;
        u = (float)count_x / ratio;
    }

    vertex vertices[4];
    memcpy(vertices, k_vertices, sizeof(k_vertices));

    for (int i = 0; i < 4; i++) {
        vertices[i].u = vertices[i].u != 0 ? u : 0;
        vertices[i].v = vertices[i].v != 0 ? v : 0;
    }
    sg_update_buffer(g_state.vb_checker, vertices, sizeof(vertices));
}

static void sx_mat4_ortho(float mat[16], float width, float height, float zn, float zf, float offset, bool ogl_ndc)
{
    const float d = zf - zn;
    const float cc = (ogl_ndc ? 2.0f : 1.0f) / d;
    const float ff = ogl_ndc ? -(zn + zf) / d : -zn / d;

    mat[0] = 2.0f / width;
    mat[1] = 0;
    mat[2] = 0;
    mat[3] = 0;

    mat[4] = 0;
    mat[5] = 2.0f / height;
    mat[6] = 0;
    mat[7] = 0;

    mat[8] = 0;
    mat[9] = 0;
    mat[10] = -cc;
    mat[11] = 0;

    mat[12] = offset;
    mat[13] = 0;
    mat[14] = ff;
    mat[15] = 1.0f;
}

static void sx_mat4_ident(float mat[16])
{
    mat[0] = 1.0f;
    mat[1] = 0;
    mat[2] = 0;
    mat[3] = 0;

    mat[4] = 0;
    mat[5] = 1.0f;
    mat[6] = 0;
    mat[7] = 0;

    mat[8] = 0;
    mat[9] = 0;
    mat[10] = 1.0f;
    mat[11] = 0;

    mat[12] = 0;
    mat[13] = 0;
    mat[14] = 0;
    mat[15] = 1.0f;
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

static sg_shader_desc get_shader_desc(const void* vs_data, uint32_t vs_size, const void* fs_data, 
                                      uint32_t fs_size, sg_image_type imgtype)
{
    return (sg_shader_desc){
        .attrs = {
            [0] = {.name = "a_pos", .sem_name = "POSITION" },
            [1] = {.name = "a_uv", .sem_name = "TEXCOORD" }
        },
            .vs = {
            #ifdef SOKOL_D3D11
                .byte_code = (const uint8_t*)vs_data,
                .byte_code_size = vs_size,
            #else
                .source = (const char*)vs_data,
            #endif
            #ifdef SOKOL_METAL
                .entry = "main0",
            #endif
                .uniform_blocks[0] = {
                    .size = sizeof(uniforms_vs),
                    .uniforms = {
                        [0] = {.name = "proj_mat", .type = SG_UNIFORMTYPE_MAT4 },
                    }
                }
        },
            .fs = {
            #ifdef SOKOL_D3D11
                .byte_code = (const uint8_t*)fs_data,
                .byte_code_size = fs_size,
            #else
                .source = (const char*)fs_data,
            #endif
            #ifdef SOKOL_METAL
                .entry = "main0",
            #endif
                .images[0] = {
                    .name = "tex_image",
                    .type = imgtype
                },
                .uniform_blocks[0] = {
                    .size = sizeof(uniforms_fs),
                    .uniforms = {
                        [0] = {.name = "color", .type = SG_UNIFORMTYPE_FLOAT4 },
                        [1] = {.name = "target_lod", SG_UNIFORMTYPE_FLOAT4 }
                    }
                }
        }
    };
}

static void init(void) 
{
    sg_setup(&(sg_desc) {
        .context = sapp_sgcontext()
    });

    sg_image_type imgtype = (g_state.texinfo.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP) ? SG_IMAGETYPE_CUBE : SG_IMAGETYPE_2D;

    g_state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = {0, 0, 0, 1.0f} }
    };

    g_state.vb = sg_make_buffer(&(sg_buffer_desc) {
        .usage = SG_USAGE_DYNAMIC,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .size = sizeof(k_vertices)
    });

    g_state.vb_checker = sg_make_buffer(&(sg_buffer_desc) {
        .usage = SG_USAGE_DYNAMIC,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .size = sizeof(k_vertices)
    });

    g_state.ib = sg_make_buffer(&(sg_buffer_desc) {
        .usage = SG_USAGE_IMMUTABLE,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(k_indices),
        .content = k_indices
    });


    {
        sg_shader_desc desc = get_shader_desc(quad_vs_data, quad_vs_size, quad_fs_data, quad_fs_size, 
                                              SG_IMAGETYPE_2D);
        g_state.shader = sg_make_shader(&desc);
    }

    {
        sg_shader_desc desc = get_shader_desc(quad_vs_data, quad_vs_size, quad_cubemap_fs_data, 
                                              quad_cubemap_fs_size, SG_IMAGETYPE_CUBE);
        g_state.shader_cubemap = sg_make_shader(&desc);
    }

    sg_pipeline_desc pip_desc = (sg_pipeline_desc) {
        .layout = {
            .buffers[0] = {
                .stride = sizeof(vertex),
            },
            .attrs = {
                [0] = {.offset = 0, .format = SG_VERTEXFORMAT_FLOAT2 },
                [1] = {.offset = 8, .format = SG_VERTEXFORMAT_FLOAT3 }
            }
        },
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
    };

    {
        pip_desc.shader = g_state.shader;
        g_state.pip = sg_make_pipeline(&pip_desc);
    }

    {
        pip_desc.shader = g_state.shader_cubemap;
        g_state.pip_cubemap = sg_make_pipeline(&pip_desc);
    }

    // main texture (dds-ktx)
    if (imgtype == SG_IMAGETYPE_CUBE) {
        set_cube_face(0);
    } else {
        sg_update_buffer(g_state.vb, k_vertices, sizeof(k_vertices));
    }

    adjust_checker_coords(sapp_width(), sapp_height());

    sg_image_desc desc = {
        .type = imgtype,
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

    int num_faces = imgtype == SG_IMAGETYPE_CUBE ? 6 : 1;
    for (int face = 0; face < num_faces; face++) {
        for (int mip = 0; mip < g_state.texinfo.num_mips; mip++) {
            ddsktx_sub_data subdata;
            ddsktx_get_sub(&g_state.texinfo, &subdata, g_state.file_data, g_state.file_size, 0, face, mip);
            desc.content.subimage[face][mip].ptr = subdata.buff;
            desc.content.subimage[face][mip].size = subdata.size_bytes;
        }
    }

    g_state.tex = sg_make_image(&desc);

    sdtx_setup(&(sdtx_desc_t) {
        .fonts = {
            [0] = sdtx_font_c64(),
        },
    });
    sdtx_set_context(SDTX_DEFAULT_CONTEXT);
    sdtx_canvas((float)sapp_width() * (1.0f/FONT_SCALE), (float)sapp_height() * (1.0f/FONT_SCALE));

    uint32_t checker_colors[] = { 0xff999999, 0xff666666 };

    g_state.checker = create_checker_texture(8, 16, checker_colors);

    g_state.vars_fs.color[0] = g_state.vars_fs.color[1] = g_state.vars_fs.color[2] = g_state.vars_fs.color[3] = 1.0f;
}

static const char* texture_type_info()
{
    static char info[128];

    const char* type = "2D";
    if (g_state.texinfo.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP) {
        snprintf(info, sizeof(info), "Cube (%s)", k_cube_face_names[g_state.cube_face]);
    } else if (g_state.texinfo.depth > 1) {
        snprintf(info, sizeof(info), "3D (%d/%d)", g_state.cur_slice, g_state.texinfo.depth);
    } else {
        strcpy(info, "2D");
    }

    return info;

}

static void frame(void) 
{
    sdtx_home();
    sdtx_origin(1, 1);
    sdtx_pos(0, 0);
    sdtx_color3b(!g_state.inv_text_color ? 255 : 0, !g_state.inv_text_color ? 255 : 0, 0);

    sdtx_printf("%s\t%dx%d (mip %d/%d)", 
                ddsktx_format_str(g_state.texinfo.format), g_state.texinfo.width, 
                g_state.texinfo.height, g_state.cur_mip + 1, g_state.texinfo.num_mips);
    sdtx_crlf();
    sdtx_printf("%s\tmask: %c%c%c%c\t", 
                texture_type_info(),
                g_state.vars_fs.color[0] == 1.0f ? 'R' : 'X',
                g_state.vars_fs.color[1] == 1.0f ? 'G' : 'X',
                g_state.vars_fs.color[2] == 1.0f ? 'B' : 'X',
                g_state.vars_fs.color[3] == 1.0f ? 'A' : 'X');
    sdtx_crlf();

    g_state.vars_fs.args[0] = (float)g_state.cur_mip;

    sg_begin_default_pass(&g_state.pass_action, sapp_width(), sapp_height());
    if (g_state.tex.id) {
        sg_bindings bindings = {
            .index_buffer = g_state.ib,
        };

        if (g_state.checker.id) {
            bindings.fs_images[0] = g_state.checker;
            bindings.vertex_buffers[0] = g_state.vb_checker;
            uniforms_fs ufs = {
                {1.0f, 1.0f, 1.0f, 1.0f},
                {0, 0, 0, 0}
            };

            uniforms_vs uvs;

            float ratio = (float)sapp_width() / (float)sapp_height();
            float w = 1.0f, h = 1.0f;
            if (sapp_width() > sapp_height()) {
                h = w/ratio;
            } else {
                w = h*ratio;
            }
            sx_mat4_ortho(uvs.proj_mat, w, h, -1.0f, 1.0f, 0, false);

            sg_apply_pipeline(g_state.pip);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uvs, sizeof(uvs));
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &ufs, sizeof(ufs));
            sg_apply_bindings(&bindings);
            sg_draw(0, 6, 1);
        }


        uniforms_vs uvs;
        sx_mat4_ident(uvs.proj_mat);
        bindings.fs_images[0] = g_state.tex;
        bindings.vertex_buffers[0] = g_state.vb;

        // for the image to the window and keep the ratio
        int w = g_state.texinfo.width;
        int h = g_state.texinfo.height;
        {
            float ratio_outer = (float)sapp_width() / (float)sapp_height();
            float ratio_inner = (float)w / (float)h;
            float scale = (ratio_inner >= ratio_outer) ? 
                ((float)sapp_width()/(float)w) : 
                ((float)sapp_height()/(float)h);
            w = (int)((float)w * scale);
            h = (int)((float)h * scale);
        }

        sg_apply_viewport((sapp_width() - w)/2, (sapp_height() - h)/2, w, h, true);

        sg_apply_pipeline((g_state.texinfo.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP) ? g_state.pip_cubemap : g_state.pip);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uvs, sizeof(uvs));
        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &g_state.vars_fs, sizeof(g_state.vars_fs));
        sg_apply_bindings(&bindings);
        sg_draw(0, 6, 1);
    }

    sg_apply_viewport(0, 0, sapp_width(), sapp_height(), true);
    sdtx_draw();

    sg_end_pass();
    sg_commit();
}

static void release(void)
{
    free(g_state.file_data);
    sg_destroy_pipeline(g_state.pip);
    sg_destroy_pipeline(g_state.pip_checker);
    sg_destroy_pipeline(g_state.pip_cubemap);
    sg_destroy_shader(g_state.shader);
    sg_destroy_shader(g_state.shader_cubemap);
    sg_destroy_buffer(g_state.vb);
    sg_destroy_buffer(g_state.vb_checker);
    sg_destroy_buffer(g_state.ib);
    sg_destroy_image(g_state.tex);
    sg_destroy_image(g_state.checker);
    sdtx_shutdown();
    sg_shutdown();
}

static void on_events(const sapp_event* e)
{
    switch (e->type) {
    case SAPP_EVENTTYPE_RESIZED:
        sdtx_canvas((float)sapp_width() * (1.0f/FONT_SCALE), (float)sapp_height() * (1.0f/FONT_SCALE));
        adjust_checker_coords(e->window_width, e->window_height);
        break;

    case SAPP_EVENTTYPE_KEY_DOWN:
        if (e->key_code == SAPP_KEYCODE_GRAVE_ACCENT) {
            g_state.inv_text_color = !g_state.inv_text_color;
        }
        if (e->key_code == SAPP_KEYCODE_A) {
            g_state.vars_fs.color[3] = g_state.vars_fs.color[3] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_R) {
            g_state.vars_fs.color[0] = g_state.vars_fs.color[0] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_G) {
            g_state.vars_fs.color[1] = g_state.vars_fs.color[1] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_B) {
            g_state.vars_fs.color[2] = g_state.vars_fs.color[2] == 1.0f ? 0 : 1.0f;
        }
        if (e->key_code == SAPP_KEYCODE_UP) {
            g_state.cur_mip = (g_state.cur_mip + 1) >= g_state.texinfo.num_mips ? (g_state.texinfo.num_mips - 1) : g_state.cur_mip + 1;
        }
        if (e->key_code == SAPP_KEYCODE_DOWN) {
            g_state.cur_mip = (g_state.cur_mip > 0) ? g_state.cur_mip - 1 : 0;
        }

        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }

        if (e->key_code == SAPP_KEYCODE_F) {
            g_state.cube_face = (g_state.cube_face + 1) % DDSKTX_CUBE_FACE_COUNT;
            set_cube_face(g_state.cube_face);
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

    int window_w = tc.width;
    int window_h = tc.height;
    #if defined(_WIN32) || defined(_WIN64)
        int desktop_w, desktop_h;
        desktop_size(&desktop_w, &desktop_h);
        float ratio = (float)tc.width / (float)tc.height;
        if (window_w > (desktop_w - 50)) {
            window_w = desktop_w - 50;
            window_h = (int)((float)window_w / ratio);
        }
        if (window_h > (desktop_h - 50)) {
            window_h = desktop_h - 50;
            window_w = (int)((float)window_h * ratio);
        }
    #endif

    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = release,
        .event_cb = on_events,
        .width = window_w,
        .height = window_h,
        .window_title = "DDS/KTX viewer",
        .swap_interval = 2,
        .sample_count = 1
    };
}
