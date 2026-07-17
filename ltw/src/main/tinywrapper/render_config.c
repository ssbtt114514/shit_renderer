#include "render_config.h"
#include "env.h"
#include "debug.h"
#include <GLES3/gl3.h>
#include <string.h>

render_config_t ltw_render_config = {0};

static inline void set_low_precision(render_config_t* cfg) {
    cfg->enable_anisotropic = false;
    cfg->enable_float_textures = false;
    cfg->enable_depth_float = false;
    cfg->enable_16bit_textures = false;
    cfg->max_texture_size = 1024;
    cfg->max_framebuffer_size = 512;
    cfg->compress_textures = true;
    cfg->use_packed_formats = true;
}

static inline void set_medium_precision(render_config_t* cfg) {
    cfg->enable_anisotropic = false;
    cfg->enable_float_textures = false;
    cfg->enable_depth_float = false;
    cfg->enable_16bit_textures = true;
    cfg->max_texture_size = 2048;
    cfg->max_framebuffer_size = 1024;
    cfg->compress_textures = true;
    cfg->use_packed_formats = true;
}

static inline void set_high_precision(render_config_t* cfg) {
    cfg->enable_anisotropic = true;
    cfg->enable_float_textures = true;
    cfg->enable_depth_float = true;
    cfg->enable_16bit_textures = true;
    cfg->max_texture_size = 4096;
    cfg->max_framebuffer_size = 2048;
    cfg->compress_textures = false;
    cfg->use_packed_formats = false;
}

static inline void set_ultra_precision(render_config_t* cfg) {
    cfg->enable_anisotropic = true;
    cfg->enable_float_textures = true;
    cfg->enable_depth_float = true;
    cfg->enable_16bit_textures = true;
    cfg->max_texture_size = 8192;
    cfg->max_framebuffer_size = 4096;
    cfg->compress_textures = false;
    cfg->use_packed_formats = false;
}

void render_config_init(void) {
    ltw_render_config.precision = PRECISION_HIGH;
    ltw_render_config.gl_version = LTW_GL_VERSION_3_3;
    ltw_render_config.enable_anisotropic = true;
    ltw_render_config.enable_float_textures = true;
    ltw_render_config.enable_depth_float = true;
    ltw_render_config.enable_16bit_textures = true;
    ltw_render_config.max_texture_size = 4096;
    ltw_render_config.max_framebuffer_size = 2048;
    ltw_render_config.max_vertex_count = 1048576;
    ltw_render_config.compress_textures = false;
    ltw_render_config.use_packed_formats = false;
}

void render_config_set_precision(render_precision_t precision) {
    ltw_render_config.precision = precision;
    
    switch (precision) {
        case PRECISION_LOW:
            set_low_precision(&ltw_render_config);
            break;
        case PRECISION_MEDIUM:
            set_medium_precision(&ltw_render_config);
            break;
        case PRECISION_HIGH:
            set_high_precision(&ltw_render_config);
            break;
        case PRECISION_ULTRA:
            set_ultra_precision(&ltw_render_config);
            break;
    }
    
    LTW_DEBUG_PRINTF("Render precision set to: %d (low=0, medium=1, high=2, ultra=3)", precision);
}

void render_config_set_gl_version(gl_version_t version) {
    ltw_render_config.gl_version = version;
    
    if (version <= LTW_GL_VERSION_3_0) {
        ltw_render_config.enable_float_textures = false;
        ltw_render_config.enable_depth_float = false;
        ltw_render_config.enable_16bit_textures = false;
        ltw_render_config.use_packed_formats = true;
    } else if (version == LTW_GL_VERSION_3_1) {
        ltw_render_config.enable_float_textures = false;
        ltw_render_config.enable_depth_float = false;
        ltw_render_config.enable_16bit_textures = true;
        ltw_render_config.use_packed_formats = true;
    } else if (version == LTW_GL_VERSION_3_2) {
        ltw_render_config.enable_float_textures = true;
        ltw_render_config.enable_depth_float = true;
        ltw_render_config.enable_16bit_textures = true;
        ltw_render_config.use_packed_formats = false;
    }
    
    LTW_DEBUG_PRINTF("GL version set to: %d.%d", version / 10, version % 10);
}

void render_config_auto_detect(void) {
    size_t mem_mb = detect_device_memory_mb();
    
    if (mem_mb < 2048) {
        render_config_set_precision(PRECISION_LOW);
    } else if (mem_mb < 4096) {
        render_config_set_precision(PRECISION_MEDIUM);
    } else if (mem_mb < 8192) {
        render_config_set_precision(PRECISION_HIGH);
    } else {
        render_config_set_precision(PRECISION_ULTRA);
    }
    
    const char* ltw_version = getenv("LTW_GL_VERSION");
    if (ltw_version) {
        if (strcmp(ltw_version, "2.1") == 0) {
            render_config_set_gl_version(LTW_GL_VERSION_2_1);
        } else if (strcmp(ltw_version, "3.0") == 0) {
            render_config_set_gl_version(LTW_GL_VERSION_3_0);
        } else if (strcmp(ltw_version, "3.1") == 0) {
            render_config_set_gl_version(LTW_GL_VERSION_3_1);
        } else if (strcmp(ltw_version, "3.2") == 0) {
            render_config_set_gl_version(LTW_GL_VERSION_3_2);
        }
    }
    
    LTW_DEBUG_PRINTF("Auto-detected memory: %zu MB, precision: %d", mem_mb, ltw_render_config.precision);
}

GLenum render_config_pick_depth_format(GLenum requested) {
    render_config_t* cfg = &ltw_render_config;
    
    switch (requested) {
        case GL_DEPTH_COMPONENT32F:
            if (!cfg->enable_depth_float) {
                return GL_DEPTH_COMPONENT24;
            }
            return GL_DEPTH_COMPONENT32F;
        case GL_DEPTH_COMPONENT24:
            return GL_DEPTH_COMPONENT24;
        case GL_DEPTH32F_STENCIL8:
            if (!cfg->enable_depth_float) {
                return GL_DEPTH24_STENCIL8;
            }
            return GL_DEPTH32F_STENCIL8;
        case GL_DEPTH_COMPONENT:
        default:
            if (cfg->enable_depth_float) {
                return GL_DEPTH_COMPONENT32F;
            }
            return GL_DEPTH_COMPONENT16;
    }
}

GLenum render_config_pick_color_format(GLenum requested) {
    render_config_t* cfg = &ltw_render_config;
    
    switch (requested) {
        case GL_RGBA32F:
            if (!cfg->enable_float_textures) {
                return cfg->enable_16bit_textures ? GL_RGBA16F : GL_RGBA8;
            }
            return GL_RGBA32F;
        case GL_RGBA16F:
            if (!cfg->enable_16bit_textures && !cfg->enable_float_textures) {
                return GL_RGBA8;
            }
            return GL_RGBA16F;
        case GL_RGB32F:
            if (!cfg->enable_float_textures) {
                return GL_RGB8;
            }
            return GL_RGB32F;
        case GL_RGB16F:
            if (!cfg->enable_16bit_textures && !cfg->enable_float_textures) {
                return GL_RGB8;
            }
            return GL_RGB16F;
        case GL_R32F:
        case GL_G32F:
        case GL_B32F:
            if (!cfg->enable_float_textures) {
                return GL_R8;
            }
            return GL_R32F;
        case GL_RG32F:
            if (!cfg->enable_float_textures) {
                return GL_RG8;
            }
            return GL_RG32F;
        case GL_R16F:
        case GL_G16F:
            if (!cfg->enable_16bit_textures && !cfg->enable_float_textures) {
                return GL_R8;
            }
            return GL_R16F;
        case GL_RGB10_A2:
            if (cfg->use_packed_formats) {
                return GL_RGB10_A2;
            }
            return GL_RGBA8;
        case GL_R11F_G11F_B10F:
            if (!cfg->enable_float_textures) {
                return GL_RGB8;
            }
            return GL_R11F_G11F_B10F;
        case GL_RGB9_E5:
            if (!cfg->enable_float_textures) {
                return GL_RGB8;
            }
            return GL_RGB9_E5;
        default:
            return requested;
    }
}

GLenum render_config_pick_texture_format(GLenum internalformat) {
    return render_config_pick_color_format(internalformat);
}
