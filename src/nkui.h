#ifndef RAYMEOWER_NKUI_H
#define RAYMEOWER_NKUI_H
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_INCLUDE_COMMAND_USERDATA
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT


#ifndef NK_INCLUDE_FIXED_TYPES
    #define NK_INT8              Sint8
    #define NK_UINT8             Uint8
    #define NK_INT16             Sint16
    #define NK_UINT16            Uint16
    #define NK_INT32             Sint32
    #define NK_UINT32            Uint32
    #define NK_SIZE_TYPE         uintptr_t
    #define NK_POINTER_TYPE      uintptr_t
#endif

#ifndef NK_INCLUDE_STANDARD_BOOL
    #define NK_BOOL               bool
#endif

#define NK_ASSERT(condition)      SDL_assert(condition)
#define NK_STATIC_ASSERT(exp)     SDL_COMPILE_TIME_ASSERT(, exp)
#define NK_MEMSET(dst, c, len)    SDL_memset(dst, c, len)
#define NK_MEMCPY(dst, src, len)  SDL_memcpy(dst, src, len)
#define NK_VSNPRINTF(s, n, f, a)  SDL_vsnprintf(s, n, f, a)
#define NK_STRTOD(str, endptr)    SDL_strtod(str, endptr)
#define NK_INV_SQRT(f)            (1.0f / SDL_sqrtf(f))
#define NK_SIN(f)                 SDL_sinf(f)
#define NK_COS(f)                 SDL_cosf(f)

#define STBTT_ifloor(x)       ((int)SDL_floor(x))
#define STBTT_iceil(x)        ((int)SDL_ceil(x))
#define STBTT_sqrt(x)         SDL_sqrt(x)
#define STBTT_pow(x,y)        SDL_pow(x,y)
#define STBTT_fmod(x,y)       SDL_fmod(x,y)
#define STBTT_cos(x)          SDL_cosf(x)
#define STBTT_acos(x)         SDL_acos(x)
#define STBTT_fabs(x)         SDL_fabs(x)
#define STBTT_assert(x)       SDL_assert(x)
#define STBTT_strlen(x)       SDL_strlen(x)
#define STBTT_memcpy          SDL_memcpy
#define STBTT_memset          SDL_memset
#define stbtt_uint8           Uint8
#define stbtt_int8            Sint8
#define stbtt_uint16          Uint16
#define stbtt_int16           Sint16
#define stbtt_uint32          Uint32
#define stbtt_int32           Sint32
#define STBRP_SORT            SDL_qsort
#define STBRP_ASSERT          SDL_assert

#include <SDL3/SDL.h>
#define NK_IMPLEMENTATION
#include "nuklear.h"
#define NK_SDL3_RENDERER_IMPLEMENTATION
#include "nuklear_sdl3_renderer.h"

struct Settings {
    int samples;
    int depth;
    int height;
    int width;

    int renderSamples;
    int renderDepth;
    int renderHeight;
    int renderWidth;

    bool renderMode;

    union {
        struct Vec3 skyColor;
        struct nk_colorf skyColorNk;
    };
};

struct nk_context* ctx;
enum nk_anti_aliasing AA;

void NkUiInit(SDL_Window *window, SDL_Renderer *renderer) {
    float font_scale = 1.0f;
    ctx = nk_sdl_init(window, renderer, nk_sdl_allocator());
    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    struct nk_font *font;

    /* set up the font atlas and add desired font; note that font sizes are
     * multiplied by font_scale to produce better results at higher DPIs */
    atlas = nk_sdl_font_stash_begin(ctx);
    font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13 * font_scale, &config);*/
    nk_sdl_font_stash_end(ctx);

    /* this hack makes the font appear to be scaled down to the desired
     * size and is only necessary when font_scale > 1 */
    font->handle.height /= font_scale;
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    nk_style_set_font(ctx, &font->handle);
    AA = NK_ANTI_ALIASING_ON;

    nk_input_begin(ctx);
}

void NkUiDraw(struct Settings *settings) {
    nk_input_end(ctx);

    /* GUI */
    if (nk_begin(ctx, "Settings", nk_rect(50, 50, 250, 460),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
        NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
        nk_layout_row_dynamic(ctx, 25, 1);

        nk_label(ctx, "Viewport settings", NK_TEXT_CENTERED);
        nk_property_int(ctx, "Resolution X:", 32, &settings->width, 640, 1, 1);
        nk_property_int(ctx, "Resolution Y:", 32, &settings->height, 480, 1, 1);
        nk_property_int(ctx, "Samples:", 1, &settings->samples, 16, 1, 1);
        nk_property_int(ctx, "Depth:", 1, &settings->depth, 4, 1, 1);

        nk_label(ctx, "Render settings", NK_TEXT_CENTERED);
        if (nk_button_label(ctx, "Render Image")) {
            settings->renderMode = true;
        }
        nk_property_int(ctx, "Resolution X: ", 64, &settings->renderWidth, 3840, 1, 1);
        nk_property_int(ctx, "Resolution Y: ",  64, &settings->renderHeight, 2160, 1, 1);
        nk_property_int(ctx, "Samples: ", 1, &settings->renderSamples, 1024, 1, 1);
        nk_property_int(ctx, "Depth: ", 1, &settings->renderDepth, 8, 1, 1);


        nk_label(ctx, "Scene settings", NK_TEXT_CENTERED);
        nk_label(ctx, "Sky Color:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(settings->skyColorNk), nk_vec2(nk_widget_width(ctx),400))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            settings->skyColorNk = nk_color_picker(ctx, settings->skyColorNk, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            settings->skyColor.x = nk_propertyf(ctx, "#R:", 0, settings->skyColor.x, 1.0f, 0.01f,0.005f);
            settings->skyColor.y = nk_propertyf(ctx, "#G:", 0, settings->skyColor.y, 1.0f, 0.01f,0.005f);
            settings->skyColor.z = nk_propertyf(ctx, "#B:", 0, settings->skyColor.z, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }
    }
    nk_end(ctx);
}

#endif //RAYMEOWER_NKUI_H
