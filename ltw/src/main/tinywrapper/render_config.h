#ifndef LTW_RENDER_CONFIG_H
#define LTW_RENDER_CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include <GLES3/gl3.h>

typedef enum {
    PRECISION_LOW = 0,
    PRECISION_MEDIUM = 1,
    PRECISION_HIGH = 2,
    PRECISION_ULTRA = 3
} render_precision_t;

typedef enum {
    LTW_GL_VERSION_2_1 = 21,
    LTW_GL_VERSION_3_0 = 30,
    LTW_GL_VERSION_3_1 = 31,
    LTW_GL_VERSION_3_2 = 32,
    LTW_GL_VERSION_3_3 = 33
} gl_version_t;

typedef struct {
    render_precision_t precision;
    gl_version_t gl_version;
    bool enable_anisotropic;
    bool enable_float_textures;
    bool enable_depth_float;
    bool enable_16bit_textures;
    size_t max_texture_size;
    size_t max_framebuffer_size;
    size_t max_vertex_count;
    bool compress_textures;
    bool use_packed_formats;
} render_config_t;

extern render_config_t ltw_render_config;

void render_config_init(void);
void render_config_set_precision(render_precision_t precision);
void render_config_set_gl_version(gl_version_t version);
void render_config_auto_detect(void);

GLenum render_config_pick_depth_format(GLenum requested);
GLenum render_config_pick_color_format(GLenum requested);
GLenum render_config_pick_texture_format(GLenum internalformat);

#endif // LTW_RENDER_CONFIG_H
