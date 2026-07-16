/**
 * Created by: artDev
 * Copyright (c) 2025 artDev, SerpentSpirale, CADIndie.
 * For use under LGPL-3.0
 */
#include "egl.h"
#include "unordered_map/int_hash.h"
#include "string_utils.h"
#include "env.h"
#include "debug.h"
#include "mempool.h"
#include <string.h>

thread_local context_t *current_context;
unordered_map* context_map;

EGLContext (*host_eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLBoolean (*host_eglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
EGLBoolean (*host_eglMakeCurrent) (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);

void init_egl() {
    context_map = alloc_intmap();
    host_eglCreateContext = (EGLContext (*)(EGLDisplay, EGLConfig, EGLContext,
                                            const EGLint *)) host_eglGetProcAddress("eglCreateContext");
    host_eglDestroyContext = (EGLBoolean (*)(EGLDisplay, EGLContext)) host_eglGetProcAddress(
            "eglDestroyContext");
    host_eglMakeCurrent = (EGLBoolean (*)(EGLDisplay, EGLSurface, EGLSurface,
                                          EGLContext)) host_eglGetProcAddress("eglMakeCurrent");
}

static bool init_context(context_t* tw_context) {
    tw_context->shader_map = alloc_intmap_safe();
    if(!tw_context->shader_map) goto fail;
    tw_context->framebuffer_map = alloc_intmap_safe();
    if(!tw_context->framebuffer_map) goto fail_dealloc;
    tw_context->program_map = alloc_intmap_safe();
    if(!tw_context->program_map) goto fail_dealloc;
    tw_context->texture_swztrack_map = alloc_intmap_safe();
    if(!tw_context->texture_swztrack_map) goto fail_dealloc;
    for(int i = 0; i < MAX_BOUND_BASEBUFFERS; i++) {
        unordered_map *map = alloc_intmap_safe();
        if(!map) goto fail_dealloc;
        tw_context->bound_basebuffers[i] = map;
    }

    tw_context->shader_info_pool = mempool_create(sizeof(struct shader_info_t), 64);
    if(!tw_context->shader_info_pool) goto fail_dealloc;
    tw_context->program_info_pool = mempool_create(sizeof(struct program_info_t), 32);
    if(!tw_context->program_info_pool) goto fail_dealloc;
    tw_context->framebuffer_pool = mempool_create(sizeof(framebuffer_t), 32);
    if(!tw_context->framebuffer_pool) goto fail_dealloc;
    tw_context->swizzle_track_pool = mempool_create(sizeof(texture_swizzle_track_t), 128);
    if(!tw_context->swizzle_track_pool) goto fail_dealloc;

    return true;

    fail_dealloc:
    if(tw_context->swizzle_track_pool) mempool_destroy(tw_context->swizzle_track_pool);
    if(tw_context->framebuffer_pool) mempool_destroy(tw_context->framebuffer_pool);
    if(tw_context->program_info_pool) mempool_destroy(tw_context->program_info_pool);
    if(tw_context->shader_info_pool) mempool_destroy(tw_context->shader_info_pool);
    for(int i = 0; i < MAX_BOUND_BASEBUFFERS; i++) {
        unordered_map *map = tw_context->bound_basebuffers[i];
        if(map) unordered_map_free(map);
    }
    if(tw_context->shader_map)
        unordered_map_free(tw_context->shader_map);
    if(tw_context->framebuffer_map)
        unordered_map_free(tw_context->framebuffer_map);
    if(tw_context->program_map)
        unordered_map_free(tw_context->program_map);
    if(tw_context->texture_swztrack_map)
        unordered_map_free(tw_context->texture_swztrack_map);
    fail:
    return false;
}

static void free_context(context_t* tw_context) {
    unordered_map_free(tw_context->shader_map);
    unordered_map_free(tw_context->program_map);
    unordered_map_free(tw_context->framebuffer_map);
    unordered_map_free(tw_context->texture_swztrack_map);
    if(tw_context->extensions_string != NULL) free(tw_context->extensions_string);
    if(tw_context->nextras != 0 && tw_context->extra_extensions_array != NULL) {
        for(int i = 0; i < tw_context->nextras; i++) {
            free((tw_context->extra_extensions_array[i]));
        }
        free(tw_context->extra_extensions_array);
    }

    if(tw_context->shader_info_pool) mempool_destroy(tw_context->shader_info_pool);
    if(tw_context->program_info_pool) mempool_destroy(tw_context->program_info_pool);
    if(tw_context->framebuffer_pool) mempool_destroy(tw_context->framebuffer_pool);
    if(tw_context->swizzle_track_pool) mempool_destroy(tw_context->swizzle_track_pool);
}

void init_extra_extensions(context_t* context, int* length) {
    const char* es_extensions = (const char*)es3_functions.glGetString(GL_EXTENSIONS);
    *length = (int)strlen(es_extensions);
    size_t capacity = *length + 512 + 1;
    context->extensions_string = malloc(capacity);
    if(!context->extensions_string) {
        LTW_ERROR_PRINTF("LTW: Failed to allocate memory for extensions string");
        *length = 0;
        return;
    }
    context->extensions_capacity = capacity;
    memcpy(context->extensions_string, es_extensions, *length+1);
}

void add_extra_extension(context_t* context, int* length, const char* extension)  {
    size_t extension_len = strlen(extension);

    if(extension_len > 1024) {
        LTW_ERROR_PRINTF("LTW: Extension name too long: %zu bytes", extension_len);
        return;
    }

    char* str_append_extension = malloc(extension_len + 2);
    if(!str_append_extension) {
        LTW_ERROR_PRINTF("LTW: Failed to allocate memory for extension string");
        return;
    }
    memcpy(str_append_extension, extension, extension_len);
    str_append_extension[extension_len] = ' ';
    str_append_extension[extension_len + 1] = 0;

    size_t current_capacity = context->extensions_capacity;
    size_t new_length = *length + extension_len + 1;
    if(new_length >= current_capacity) {
        size_t new_capacity = new_length + 256;
        char* new_ptr = realloc(context->extensions_string, new_capacity);
        if(!new_ptr) {
            LTW_ERROR_PRINTF("LTW: Failed to reallocate extensions string");
            free(str_append_extension);
            return;
        }
        context->extensions_string = new_ptr;
        context->extensions_capacity = new_capacity;
    }

    int extension_idx = context->nextras;
    char** new_array = realloc(context->extra_extensions_array, sizeof(char*)*(context->nextras + 1));
    if(!new_array) {
        LTW_ERROR_PRINTF("LTW: Failed to reallocate extra extensions array");
        free(str_append_extension);
        return;
    }
    context->extra_extensions_array = new_array;
    char* extra_extension = malloc(extension_len + 1);
    if(!extra_extension) {
        LTW_ERROR_PRINTF("LTW: Failed to allocate memory for extra extension");
        free(str_append_extension);
        return;
    }
    strncpy(extra_extension, extension, extension_len + 1);
    context->extra_extensions_array[extension_idx] = extra_extension;

    memcpy(context->extensions_string + *length, str_append_extension, extension_len + 2);
    *length += extension_len + 1;
    context->nextras++;
    free(str_append_extension);
}

void fin_extra_extensions(context_t* context, int length) {
    if(context->extensions_string[length-2] != ' ') return;
    char* orig_string = context->extensions_string;
    context->extensions_string = realloc(context->extensions_string, length - 1);
    if(context->extensions_string == NULL) {
        free(orig_string);
        return;
    }
    context->extensions_string[length-2] = 0;
}

void build_extension_string(context_t* context) {
    int length;
    init_extra_extensions(context, &length);

    // Compatibility mode for older Minecraft releases (1.17 and below). When
    // LTW_GL_VERSION=3.2 is set we omit extensions that only exist on ES 3.1/3.2
    // so the application does not take render paths that require compute shaders,
    // SSBOs, indirect drawing, etc.
    const char* compat_version = getenv("LTW_GL_VERSION");
    bool compat_3_2 = compat_version && strcmp(compat_version, "3.2") == 0;
    if(context->buffer_storage) {
        if(!env_istrue("LTW_HIDE_BUFFER_STORAGE"))
            add_extra_extension(context, &length, "GL_ARB_buffer_storage");
        else printf("LTW: The buffer storage extension is hidden.\n");
    }
    if(context->buffer_texture_ext || context->es32) {
        add_extra_extension(context, &length, "GL_ARB_texture_buffer_object");
    }
    add_extra_extension(context, &length, "GL_ARB_draw_elements_base_vertex");
    // Required by Iris. Indexed variants are available since ES3.2 or with OES/EXT_draw_buffers_indexed extensions
    if(context->blending.available)
        add_extra_extension(context, &length, "GL_ARB_draw_buffers_blend");
    // Used by Minecraft for the GPU usage counter (see Blaze3D TimerQuery)
    add_extra_extension(context, &length, "GL_ARB_timer_query");

    // Immutable texture storage is core since OpenGL ES 3.0; glTexStorage2D/3D pass through directly.
    add_extra_extension(context, &length, "GL_ARB_texture_storage");

    // ESSL 3.00 (ES 3.0) provides the pack/unpack built-in functions that
    // GL_ARB_shading_language_packing advertises on desktop GL.
    add_extra_extension(context, &length, "GL_ARB_shading_language_packing");

    // The ARB extensions below map 1:1 to OpenGL ES 3.1 core entry points. Their functions resolve
    // to the driver's own (same-named) ES implementations through eglGetProcAddress, and the desktop
    // GLSL is lowered to ESSL by the optimizer (see shader_wrapper.c / glsl_optimizer). Exposed only
    // on devices that actually report ES 3.1, so the application never takes an unsupported path.
    if(context->es31 && !compat_3_2) {
        add_extra_extension(context, &length, "GL_ARB_draw_indirect");
        add_extra_extension(context, &length, "GL_ARB_texture_multisample");
        add_extra_extension(context, &length, "GL_ARB_texture_storage_multisample");
        add_extra_extension(context, &length, "GL_ARB_stencil_texturing");
        add_extra_extension(context, &length, "GL_ARB_shader_storage_buffer_object");
        add_extra_extension(context, &length, "GL_ARB_compute_shader");
        add_extra_extension(context, &length, "GL_ARB_shader_image_load_store");
        add_extra_extension(context, &length, "GL_ARB_shader_image_size");
        add_extra_extension(context, &length, "GL_ARB_shader_atomic_counters");
        add_extra_extension(context, &length, "GL_ARB_program_interface_query");
        // ESSL 3.10 provides precise, fma, bitfield manipulation, etc.
        add_extra_extension(context, &length, "GL_ARB_gpu_shader5");
    }

    // Extensions whose backing ES 3.1 core entry points were individually probed
    // in find_esversion().  Advertised only when the function pointers actually
    // resolved, so there is no risk of a silent no-op stub.
    // (GL_ARB_multi_draw_indirect is intentionally NOT advertised here: the
    // unsuffixed desktop entry points have no ES core equivalent, only the
    // GL_EXT_multi_draw_indirect EXT-suffixed ones, and there is no override
    // forwarding the desktop names — advertising would route the app into a
    // silent no-op stub. Use the existing multidraw_indirect flag for that path.)
    if(context->framebuffer_no_attachments) {
        add_extra_extension(context, &length, "GL_ARB_framebuffer_no_attachments");
    }
    if(context->vertex_attrib_binding) {
        add_extra_extension(context, &length, "GL_ARB_vertex_attrib_binding");
    }

    // glCopyImageSubData is core since OpenGL ES 3.2.
    if(context->es32 && !compat_3_2) {
        add_extra_extension(context, &length, "GL_ARB_copy_image");
    }

    // NOTE: GL_ARB_texture_view / GL_ARB_sample_shading are NOT advertised here
    // because the NDK gl32.h does not define the unsuffixed PFN typedefs for
    // glTextureView / glMinSampleShading (only OES-suffixed variants exist).
    // When support is needed, add forwarding overrides in es3_overrides.h.
    fin_extra_extensions(context, length);
}

static void find_esversion(context_t* context) {
    const char* version = (const char*) es3_functions.glGetString(GL_VERSION);
    const char* shader_version = (const char*) es3_functions.glGetString(GL_SHADING_LANGUAGE_VERSION);

    int esmajor = 0, esminor = 0, shadermajor = 3, shaderminor = 0;
    sscanf(version, " OpenGL ES %i.%i", &esmajor, &esminor);
    sscanf(shader_version, " OpenGL ES GLSL ES %i.%i", &shadermajor, &shaderminor);
    context->shader_version = shadermajor * 100 + shaderminor;
    printf("LTW: Running on OpenGL ES %i.%i with ESSL %i\n", esmajor, esminor, context->shader_version);
    if(esmajor == 0 && esminor == 0) goto fail;
    if(esmajor < 3 || context->shader_version < 300) {
        printf("Unsupported OpenGL ES version. This will cause you problems down the line.\n");
        return;
    }
    if(esmajor == 3) {
        context->es31 = esminor >= 1;
        context->es32 = esminor >= 2;
    }else if(esmajor > 3) {
        context->es32 = context->es31 = true;
    }

    const char* extensions = (const char*) es3_functions.glGetString(GL_EXTENSIONS);
    if(strstr(extensions, "GL_EXT_buffer_storage")) context->buffer_storage = true;
    if(strstr(extensions, "GL_EXT_texture_buffer")) context->buffer_texture_ext = true;
    if(strstr(extensions, "GL_EXT_multi_draw_indirect")) context->multidraw_indirect = true;

    // Probe ES 3.1/3.2 core entry points (same names as desktop GL, resolved via
    // eglGetProcAddress).  Each ARB extension below is advertised only when its
    // backing function pointers actually resolved, so the application never takes
    // a code path that silently no-ops.
    if(context->es31) {
        context->framebuffer_no_attachments =
            es3_functions.glFramebufferParameteri != NULL &&
            es3_functions.glGetFramebufferParameteriv != NULL;
        context->vertex_attrib_binding =
            es3_functions.glBindVertexBuffer != NULL &&
            es3_functions.glVertexAttribBinding != NULL &&
            es3_functions.glVertexAttribFormat != NULL &&
            es3_functions.glVertexBindingDivisor != NULL;
    }
    // EXT_disjoint_timer_query provides accurate int64 timer queries.
    // On Core Profile it's ARB_timer_query instead.
    // This enables real time queries via mentioned extension, otherwise faked ones are used (see query.c)
    if(strstr(extensions, "GL_EXT_disjoint_timer_query") || env_istrue_d("LTW_ENABLE_TIMER_QUERY", false)) context->timer_query = true;

    bool basevertex_oes = strstr(extensions, "GL_OES_draw_elements_base_vertex");
    bool basevertex_ext = strstr(extensions, "GL_EXT_draw_elements_base_vertex");
    if(context->es32) context->drawelementsbasevertex = es3_functions.glDrawElementsBaseVertex;
    else if(basevertex_oes) context->drawelementsbasevertex = es3_functions.glDrawElementsBaseVertexOES;
    else if(basevertex_ext) context->drawelementsbasevertex = es3_functions.glDrawElementsBaseVertexEXT;
    else context->drawelementsbasevertex = NULL;

    bool drawbuffersi_oes = strstr(extensions, "GL_OES_draw_buffers_indexed");
    bool drawbuffersi_ext = strstr(extensions, "GL_EXT_draw_buffers_indexed");
    blending_functions_t* blend = &context->blending;
    blend->available = true;
#define SET_FUNC(type) \
    blend->blendequationi = es3_functions.glBlendEquationi ## type; \
    blend->blendequationseparatei = es3_functions.glBlendEquationSeparatei ## type; \
    blend->blendfunci = es3_functions.glBlendFunci ## type; \
    blend->blendfuncseparatei = es3_functions.glBlendFuncSeparatei ## type; \
    blend->colormaski = es3_functions.glColorMaski ## type; \

    if(context->es32){
        SET_FUNC()
    }
    else if(drawbuffersi_oes){
        SET_FUNC(OES)
    }
    else if(drawbuffersi_ext){
        SET_FUNC(EXT)
    }
    else {
        blend->available = false;
    }
#undef SET_FUNC
    build_extension_string(context);

    return;
    fail:
    printf("LTW: Failed to detect OpenGL ES version");
}

void basevertex_init(context_t* context);
void buffer_copier_init(context_t* context);
static void init_incontext(context_t* tw_context) {
    es3_functions.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tw_context->maxTextureSize);
    es3_functions.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &tw_context->max_drawbuffers);
    es3_functions.glGetIntegerv(GL_NUM_EXTENSIONS, &tw_context->nextensions_es);
    if(tw_context->max_drawbuffers > MAX_DRAWBUFFERS) {
        tw_context->max_drawbuffers = MAX_DRAWBUFFERS;
    }

    find_esversion(tw_context);

    basevertex_init(tw_context);
    buffer_copier_init(tw_context);
    es3_functions.glGenBuffers(1, &tw_context->multidraw_element_buffer);

    memset(tw_context->format_cache, 0, sizeof(tw_context->format_cache));
    tw_context->format_cache_index = 0;

    size_t device_memory_mb = detect_device_memory_mb();
    if(device_memory_mb >= 6144) {
        tw_context->multidraw_buffer_size = 512 * 1024;
        LTW_ERROR_PRINTF("LTW: Using large multidraw buffer (512KB) for high-memory device");
    } else if(device_memory_mb >= 4096) {
        tw_context->multidraw_buffer_size = 384 * 1024;
        LTW_ERROR_PRINTF("LTW: Using medium multidraw buffer (384KB) for mid-range device");
    } else {
        tw_context->multidraw_buffer_size = 256 * 1024;
        LTW_ERROR_PRINTF("LTW: Using small multidraw buffer (256KB) for low-memory device");
    }

    es3_functions.glBindBuffer(GL_COPY_WRITE_BUFFER, tw_context->multidraw_element_buffer);
    es3_functions.glBufferData(GL_COPY_WRITE_BUFFER, tw_context->multidraw_buffer_size, NULL, GL_STREAM_DRAW);
    es3_functions.glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    tw_context->pending_swizzle_count = 0;
    tw_context->swizzle_batch_mode = false;
    memset(tw_context->pending_swizzle_textures, 0, sizeof(tw_context->pending_swizzle_textures));

    tw_context->fast_gl.glDrawArrays = es3_functions.glDrawArrays;
    tw_context->fast_gl.glDrawElements = es3_functions.glDrawElements;
    tw_context->fast_gl.glBindBuffer = es3_functions.glBindBuffer;
    tw_context->fast_gl.glBindTexture = es3_functions.glBindTexture;
    tw_context->fast_gl.glTexParameteri = es3_functions.glTexParameteri;
    tw_context->fast_gl.glGetTexParameteriv = es3_functions.glGetTexParameteriv;
    tw_context->fast_gl.glGetIntegerv = es3_functions.glGetIntegerv;
    tw_context->fast_gl.glBufferData = es3_functions.glBufferData;
    tw_context->fast_gl.glBufferSubData = es3_functions.glBufferSubData;
    tw_context->fast_gl.glCopyBufferSubData = es3_functions.glCopyBufferSubData;
    tw_context->fast_gl.glMapBufferRange = es3_functions.glMapBufferRange;
    tw_context->fast_gl.glUnmapBuffer = es3_functions.glUnmapBuffer;
    tw_context->fast_gl.glFlushMappedBufferRange = es3_functions.glFlushMappedBufferRange;

    tw_context->multidraw_ring_head = 0;
    tw_context->multidraw_ring_tail = 0;
    tw_context->multidraw_ring_wrapped = false;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
    EGLContext phys_context = host_eglCreateContext(dpy, config, share_context, attrib_list);
    if(phys_context == EGL_NO_CONTEXT) return phys_context;
    context_t* tw_context = calloc(1, sizeof(context_t));
    if(tw_context == NULL || !init_context(tw_context)) {
        if(tw_context) free(tw_context);
        host_eglDestroyContext(dpy, phys_context);
        return EGL_NO_CONTEXT;
    }
    unordered_map_put(context_map, phys_context, tw_context);
    return phys_context;
}

EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx) {
    if(!host_eglDestroyContext(dpy, ctx)) return EGL_FALSE;
    context_t* old_ctx = unordered_map_remove(context_map, ctx);
    free_context(old_ctx);
    free(old_ctx);
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    if(!host_eglMakeCurrent(dpy, draw, read, ctx)) return EGL_FALSE;
    if(ctx == EGL_NO_CONTEXT) {
        current_context = NULL;
        return EGL_TRUE;
    }
    context_t* tw_context = unordered_map_get(context_map, ctx);
    if(tw_context == NULL) {
        printf("TinywrapperEGL: Failed to find context %p\n", ctx);
        abort();
    }
    if(!tw_context->context_rdy) {
        init_incontext(tw_context);
        tw_context->context_rdy = true;
    }
    current_context = tw_context;
    return EGL_TRUE;
}
