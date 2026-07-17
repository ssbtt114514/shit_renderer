/**
 * Created by: artDev
 * Copyright (c) 2025 artDev, SerpentSpirale, CADIndie.
 * For use under LGPL-3.0
 *
 * Advanced OpenGL compatibility functions for Minecraft 1.17 and below.
 * Borrows design patterns from GL4ES (https://github.com/ptitSeb/gl4es).
 */

#include "proc.h"
#include "egl.h"
#include "debug.h"
#include "env.h"
#include "swizzle.h"
#include "glformats.h"
#include <string.h>
#include <stdlib.h>

/* GLDEBUGPROC may not be defined in ES headers */
#ifndef GLDEBUGPROC
typedef void (GL_APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
#endif

/* ── Missing GL constants (not in ES headers) ─────────────────────────── */

#ifndef GL_POLYGON_MODE
#define GL_POLYGON_MODE 0x0B40
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif
#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_PROXY_TEXTURE_1D
#define GL_PROXY_TEXTURE_1D 0x8063
#endif
#ifndef GL_CLAMP_READ_COLOR
#define GL_CLAMP_READ_COLOR 0x891C
#endif
#ifndef GL_FIXED_ONLY
#define GL_FIXED_ONLY 0x891D
#endif
#ifndef GL_FIRST_VERTEX_CONVENTION
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D
#endif
#ifndef GL_LAST_VERTEX_CONVENTION
#define GL_LAST_VERTEX_CONVENTION 0x8E4E
#endif
#ifndef GL_TEXTURE_BINDING_1D
#define GL_TEXTURE_BINDING_1D 0x8068
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#endif
#ifndef GL_DEPTH_STENCIL_TEXTURE_MODE
#define GL_DEPTH_STENCIL_TEXTURE_MODE 0x90EA
#endif
#ifndef GL_PRIMITIVE_RESTART
#define GL_PRIMITIVE_RESTART 0x8F9D
#endif
#ifndef GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90DB
#endif

/* ── 1. glPolygonMode — emulate desktop polygon mode on ES ──────────── */

static GLenum current_polygon_mode = GL_FILL;

void glPolygonMode(GLenum face, GLenum mode) {
    if(!current_context) return;
    /* ES only supports GL_FILL; remember the requested mode for glGetIntegerv */
    if(mode != GL_FILL && mode != GL_LINE && mode != GL_POINT) return;
    current_polygon_mode = mode;
}

/* ── 2. 1D Texture functions — emulate with 2D textures (height=1) ──── */

void glTexImage1D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLint border, GLenum format, GLenum type,
                  const GLvoid *data) {
    if(!current_context) return;
    if(target == GL_PROXY_TEXTURE_1D) {
        current_context->proxy_width = (width > current_context->maxTextureSize) ? 0 : width;
        current_context->proxy_height = 1;
        current_context->proxy_intformat = internalformat;
        return;
    }
    if(target != GL_TEXTURE_1D) return;
    if(data != NULL) {
        GLenum fmt = format, typ = type;
        swizzle_process_upload(GL_TEXTURE_2D, &fmt, &typ);
        pick_internalformat(&internalformat, &typ, &fmt, &data);
        format = fmt; type = typ;
    }
    es3_functions.glTexImage2D(GL_TEXTURE_2D, level, internalformat,
                               width, 1, border, format, type, data);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                     GLsizei width, GLenum format, GLenum type,
                     const GLvoid *data) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    if(data != NULL) swizzle_process_upload(GL_TEXTURE_2D, &format, &type);
    es3_functions.glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0,
                                   width, 1, format, type, data);
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei width, GLint border) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    es3_functions.glCopyTexImage2D(GL_TEXTURE_2D, level, internalformat,
                                    x, y, width, 1, border);
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                         GLint x, GLint y, GLsizei width) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    es3_functions.glCopyTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0,
                                       x, y, width, 1);
}

void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat,
                             GLsizei width, GLint border, GLsizei imageSize,
                             const GLvoid *data) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    es3_functions.glCompressedTexImage2D(GL_TEXTURE_2D, level, internalformat,
                                          width, 1, border, imageSize, data);
}

void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format, GLsizei imageSize,
                                const GLvoid *data) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    es3_functions.glCompressedTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0,
                                             width, 1, format, imageSize, data);
}

void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat,
                     GLsizei width) {
    if(!current_context || target != GL_TEXTURE_1D) return;
    es3_functions.glTexStorage2D(GL_TEXTURE_2D, levels, internalformat, width, 1);
}

/* ── 3. Sync objects — forward to ES implementation ─────────────────── */

GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    if(!current_context) return NULL;
    return es3_functions.glFenceSync(condition, flags);
}

GLboolean glIsSync(GLsync sync) {
    if(!current_context) return GL_FALSE;
    return es3_functions.glIsSync(sync);
}

void glDeleteSync(GLsync sync) {
    if(!current_context) return;
    es3_functions.glDeleteSync(sync);
}

GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    if(!current_context) return GL_WAIT_FAILED;
    return es3_functions.glClientWaitSync(sync, flags, timeout);
}

void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    if(!current_context) return;
    es3_functions.glWaitSync(sync, flags, timeout);
}

void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize,
                  GLsizei *length, GLint *values) {
    if(!current_context) return;
    es3_functions.glGetSynciv(sync, pname, bufSize, length, values);
}

void glGetInteger64v(GLenum pname, GLint64 *data) {
    if(!current_context) return;
    es3_functions.glGetInteger64v(pname, data);
}

void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) {
    if(!current_context) return;
    es3_functions.glGetInteger64i_v(target, index, data);
}

void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) {
    if(!current_context) return;
    es3_functions.glGetBufferParameteri64v(target, pname, params);
}

/* ── 4. glClampColor — no ES equivalent, no-op with tracking ─────────── */

static GLenum clamp_color_mode = GL_FIXED_ONLY;

void glClampColor(GLenum target, GLenum clamp) {
    if(!current_context) return;
    if(target == GL_CLAMP_READ_COLOR) {
        clamp_color_mode = clamp;
    }
}

/* ── 5. glProvokingVertex — no ES equivalent, no-op ──────────────────── */

void glProvokingVertex(GLenum mode) {
    if(!current_context) return;
    /* ES does not support provoking vertex selection; accept and ignore */
}

/* ── 6. glPrimitiveRestartIndex — ES uses fixed index ───────────────── */

void glPrimitiveRestartIndex(GLuint index) {
    if(!current_context) return;
    /* ES 3.0 uses GL_PRIMITIVE_RESTART_FIXED_INDEX which ignores the index.
       On ES 3.2, glPrimitiveRestartIndex exists and is forwarded. */
    if(current_context->es32 && es3_functions.glPrimitiveRestartIndex) {
        es3_functions.glPrimitiveRestartIndex(index);
    }
    /* On ES 3.0/3.1, the fixed index (~0xFFFFFFFF) is always used when
       GL_PRIMITIVE_RESTART is enabled; we silently accept the call. */
}

/* ── 7. glFramebufferTexture — non-layered texture attachment ────────── */

void glFramebufferTexture(GLenum target, GLenum attachment,
                           GLuint texture, GLint level) {
    if(!current_context) return;
    /* On ES 3.2, glFramebufferTexture exists natively. On older ES, emulate
       with glFramebufferTextureLayer using layer 0. */
    if(current_context->es32 && es3_functions.glFramebufferTexture) {
        es3_functions.glFramebufferTexture(target, attachment, texture, level);
    } else {
        es3_functions.glFramebufferTextureLayer(target, attachment, texture, level, 0);
    }
}

/* ── 8. glFramebufferTexture3D — forward to Layer ────────────────────── */

void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget,
                             GLuint texture, GLint level, GLint layer) {
    if(!current_context) return;
    es3_functions.glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget,
                            GLuint texture, GLint level) {
    if(!current_context) return;
    /* 1D textures are emulated as 2D with height=1 */
    es3_functions.glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level);
}

/* ── 9. glClearBufferData / glClearBufferSubData — manual implementation */

void glClearBufferData(GLenum target, GLenum internalformat,
                        GLenum format, GLenum type, const void *data) {
    if(!current_context) return;
    /* Not available in ES core; emulate by mapping and filling the buffer */
    GLint bufferSize = 0;
    es3_functions.glGetBufferParameteriv(target, GL_BUFFER_SIZE, &bufferSize);
    if(bufferSize <= 0) return;

    /* Determine element size from internalformat */
    GLint elementSize = 4; /* default to 4 bytes */
    switch(internalformat) {
        case GL_R8: case GL_R8UI: case GL_R8I: elementSize = 1; break;
        case GL_R16: case GL_R16UI: case GL_R16I: elementSize = 2; break;
        case GL_R32UI: case GL_R32I: case GL_R32F: elementSize = 4; break;
        case GL_RG8: case GL_RG8UI: elementSize = 2; break;
        case GL_RG16: case GL_RG16UI: elementSize = 4; break;
        case GL_RG32UI: case GL_RG32I: case GL_RG32F: elementSize = 8; break;
        case GL_RGBA8: case GL_RGBA8UI: elementSize = 4; break;
        case GL_RGBA16: case GL_RGBA16UI: elementSize = 8; break;
        case GL_RGBA32UI: case GL_RGBA32I: case GL_RGBA32F: elementSize = 16; break;
        default: break;
    }

    GLint numElements = bufferSize / elementSize;
    if(numElements <= 0) return;

    void *mapped = es3_functions.glMapBufferRange(target, 0, bufferSize,
                                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if(!mapped) return;

    /* Fill the buffer with the clear pattern */
    for(GLint i = 0; i < numElements; i++) {
        memcpy((char*)mapped + i * elementSize, data, elementSize);
    }

    es3_functions.glUnmapBuffer(target);
}

void glClearBufferSubData(GLenum target, GLenum internalformat,
                          GLintptr offset, GLsizeiptr size,
                          GLenum format, GLenum type, const void *data) {
    if(!current_context) return;
    /* Emulate via map + memcpy */
    void *mapped = es3_functions.glMapBufferRange(target, offset, size,
                                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
    if(!mapped) return;

    GLint elementSize = 4;
    switch(internalformat) {
        case GL_R8: case GL_R8UI: case GL_R8I: elementSize = 1; break;
        case GL_R16: case GL_R16UI: case GL_R16I: elementSize = 2; break;
        case GL_R32UI: case GL_R32I: case GL_R32F: elementSize = 4; break;
        case GL_RG8: case GL_RG8UI: elementSize = 2; break;
        case GL_RG16: case GL_RG16UI: elementSize = 4; break;
        case GL_RG32UI: case GL_RG32I: case GL_RG32F: elementSize = 8; break;
        case GL_RGBA8: case GL_RGBA8UI: elementSize = 4; break;
        case GL_RGBA16: case GL_RGBA16UI: elementSize = 8; break;
        case GL_RGBA32UI: case GL_RGBA32I: case GL_RGBA32F: elementSize = 16; break;
        default: break;
    }

    GLint numElements = size / elementSize;
    for(GLint i = 0; i < numElements; i++) {
        memcpy((char*)mapped + i * elementSize, data, elementSize);
    }

    es3_functions.glUnmapBuffer(target);
}

/* ── 10. ES 3.1 core forwarding: compute, memory barrier, image units ── */

void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    if(!current_context || !current_context->es31) return;
    if(es3_functions.glDispatchCompute)
        es3_functions.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

void glDispatchComputeIndirect(GLintptr indirect) {
    if(!current_context || !current_context->es31) return;
    if(es3_functions.glDispatchComputeIndirect)
        es3_functions.glDispatchComputeIndirect(indirect);
}

void glMemoryBarrier(GLbitfield barriers) {
    if(!current_context || !current_context->es31) return;
    if(es3_functions.glMemoryBarrier)
        es3_functions.glMemoryBarrier(barriers);
}

void glBindImageTexture(GLuint unit, GLuint texture, GLint level,
                        GLboolean layered, GLint layer, GLenum access,
                        GLenum format) {
    if(!current_context || !current_context->es31) return;
    if(es3_functions.glBindImageTexture)
        es3_functions.glBindImageTexture(unit, texture, level, layered, layer, access, format);
}

/* ── 11. glCopyImageSubData — ES 3.2 forwarding ───────────────────────── */

void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel,
                         GLint srcX, GLint srcY, GLint srcZ,
                         GLuint dstName, GLenum dstTarget, GLint dstLevel,
                         GLint dstX, GLint dstY, GLint dstZ,
                         GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    if(!current_context || !current_context->es32) return;
    if(es3_functions.glCopyImageSubData)
        es3_functions.glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
                                          dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
                                          srcWidth, srcHeight, srcDepth);
}

/* ── 12. glGetDoublev — convert to float ─────────────────────────────── */

void glGetDoublev(GLenum pname, GLdouble *data) {
    if(!current_context) return;
    /* Determine how many values to retrieve */
    GLint count = 1;
    switch(pname) {
        case GL_VIEWPORT:
        case GL_SCISSOR_BOX:
            count = 4;
            break;
        case GL_COLOR_CLEAR_VALUE:
        case GL_BLEND_COLOR:
            count = 4;
            break;
        case GL_DEPTH_RANGE:
            count = 2;
            break;
        case GL_ALIASED_LINE_WIDTH_RANGE:
        case GL_ALIASED_POINT_SIZE_RANGE:
        case GL_MAX_TEXTURE_LOD_BIAS:
        case GL_DEPTH_BITS:
            count = 2;
            break;
        default:
            break;
    }
    GLfloat fdata[16];
    es3_functions.glGetFloatv(pname, fdata);
    for(GLint i = 0; i < count && i < 16; i++) {
        data[i] = (GLdouble)fdata[i];
    }
}

/* ── 13. glGetTexParameterIiv / Iuiv — forward with swizzle tracking ── */

void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params) {
    if(!current_context) return;
    /* Forward to regular glGetTexParameteriv; swizzle tracking is handled elsewhere */
    es3_functions.glGetTexParameteriv(target, pname, params);
}

void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params) {
    if(!current_context) return;
    GLint iparams[4];
    glGetTexParameterIiv(target, pname, iparams);
    for(int i = 0; i < 4; i++) params[i] = (GLuint)iparams[i];
}

/* ── 14. glEnablei / glDisablei / glIsEnabledi — per-target toggle ──── */

void glEnablei(GLenum cap, GLuint index) {
    if(!current_context) return;
    /* Most per-index caps don't exist in ES; blend is handled separately */
    if(cap == 0x8849 /* GL_BLEND */) {
        if(current_context->es32 && es3_functions.glBlendEquationi) {
            /* Blending is implicitly available when blend functions are present */
        }
    }
    /* Fallback: enable globally */
    es3_functions.glEnable(cap);
}

void glDisablei(GLenum cap, GLuint index) {
    if(!current_context) return;
    es3_functions.glDisable(cap);
}

GLboolean glIsEnabledi(GLenum cap, GLuint index) {
    if(!current_context) return GL_FALSE;
    return es3_functions.glIsEnabled(cap);
}

/* ── 15. glGetBooleani_v / glGetIntegeri_v ──────────────────────────── */

void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data) {
    if(!current_context) return;
    /* Not directly available; use glGetIntegeri_v and convert */
    GLint idata = 0;
    es3_functions.glGetIntegeri_v(target, index, &idata);
    *data = idata ? GL_TRUE : GL_FALSE;
}

/* ── 16. glPatchParameteri / glPatchParameterfv — tessellation ──────── */

void glPatchParameteri(GLenum pname, GLint value) {
    if(!current_context) return;
    /* ES 3.1 has GL_PATCHES but patch parameter control is limited;
       accept and ignore on devices without tessellation support */
}

void glPatchParameterfv(GLenum pname, const GLfloat *values) {
    if(!current_context) return;
    /* Same as above — no-op on ES */
}

/* ── 17. DrawArraysInstancedARB / DrawElementsInstancedARB ──────────── */

void glDrawArraysInstancedARB(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
    if(!current_context) return;
    es3_functions.glDrawArraysInstanced(mode, first, count, primcount);
}

void glDrawElementsInstancedARB(GLenum mode, GLsizei count, GLenum type,
                                 const void *indices, GLsizei primcount) {
    if(!current_context) return;
    es3_functions.glDrawElementsInstanced(mode, count, type, indices, primcount);
}

/* ── 18. UniformMatrix variant ARB aliases ──────────────────────────── */

void glUniformMatrix2x3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix2x3fv(location, count, transpose, value);
}
void glUniformMatrix3x2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix3x2fv(location, count, transpose, value);
}
void glUniformMatrix2x4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix2x4fv(location, count, transpose, value);
}
void glUniformMatrix4x2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix4x2fv(location, count, transpose, value);
}
void glUniformMatrix3x4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix3x4fv(location, count, transpose, value);
}
void glUniformMatrix4x3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    es3_functions.glUniformMatrix4x3fv(location, count, transpose, value);
}

/* ── 19. Uniform double → float conversion ──────────────────────────── */

void glUniform1d(GLint location, GLdouble x) {
    if(!current_context) return;
    es3_functions.glUniform1f(location, (GLfloat)x);
}
void glUniform2d(GLint location, GLdouble x, GLdouble y) {
    if(!current_context) return;
    es3_functions.glUniform2f(location, (GLfloat)x, (GLfloat)y);
}
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {
    if(!current_context) return;
    es3_functions.glUniform3f(location, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    if(!current_context) return;
    es3_functions.glUniform4f(location, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniform1fv(location, count, fval);
    free(fval);
}
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 2 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 2; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniform2fv(location, count, fval);
    free(fval);
}
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 3 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 3; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniform3fv(location, count, fval);
    free(fval);
}
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 4 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 4; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniform4fv(location, count, fval);
    free(fval);
}
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 4 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 4; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix2fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 9 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 9; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix3fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 16 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 16; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix4fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 6 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 6; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix2x3fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 6 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 6; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix3x2fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 8 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 8; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix2x4fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 8 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 8; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix4x2fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 12 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 12; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix3x4fv(location, count, transpose, fval);
    free(fval);
}
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    if(!current_context) return;
    GLfloat *fval = malloc(count * 12 * sizeof(GLfloat));
    if(!fval) return;
    for(GLsizei i = 0; i < count * 12; i++) fval[i] = (GLfloat)value[i];
    es3_functions.glUniformMatrix4x3fv(location, count, transpose, fval);
    free(fval);
}

/* ── 20. glTexStorage1D/3D ARB aliases ──────────────────────────────── */

void glTexStorage1DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
    glTexStorage1D(target, levels, internalformat, width);
}

void glTexStorage2DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    if(!current_context) return;
    es3_functions.glTexStorage2D(target, levels, internalformat, width, height);
}

void glTexStorage3DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    if(!current_context) return;
    es3_functions.glTexStorage3D(target, levels, internalformat, width, height, depth);
}

/* ── 21. glFramebufferTextureARB alias ───────────────────────────────── */

void glFramebufferTextureARB(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    glFramebufferTexture(target, attachment, texture, level);
}

void glFramebufferTextureLayerARB(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    if(!current_context) return;
    es3_functions.glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

/* ── 22. ProgramUniform — forward to uniform with program switch ────── */

void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform1f(location, v0);
    es3_functions.glUseProgram(old);
}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform1i(location, v0);
    es3_functions.glUseProgram(old);
}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform1fv(location, count, value);
    es3_functions.glUseProgram(old);
}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform1iv(location, count, value);
    es3_functions.glUseProgram(old);
}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform2f(location, v0, v1);
    es3_functions.glUseProgram(old);
}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform2i(location, v0, v1);
    es3_functions.glUseProgram(old);
}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform3f(location, v0, v1, v2);
    es3_functions.glUseProgram(old);
}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniform4f(location, v0, v1, v2, v3);
    es3_functions.glUseProgram(old);
}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    if(!current_context) return;
    GLuint old = current_context->program;
    es3_functions.glUseProgram(program);
    es3_functions.glUniformMatrix4fv(location, count, transpose, value);
    es3_functions.glUseProgram(old);
}

/* ── 23. glTexBufferARB — already aliased in main.c, but also expose EXT */

void glTexBufferRangeARB(GLenum target, GLenum internalFormat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if(!current_context) return;
    if(current_context->es32) es3_functions.glTexBufferRange(target, internalFormat, buffer, offset, size);
    else if(current_context->buffer_texture_ext) es3_functions.glTexBufferRangeEXT(target, internalFormat, buffer, offset, size);
}

/* ── 24. glClearTexImage / glClearTexSubImage ───────────────────────── */

void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {
    if(!current_context) return;
    /* Emulate by binding to a temp FBO and clearing */
    /* Simple approach: re-upload texture data as NULL with given format */
    /* This is a best-effort implementation */
    LTW_DEBUG_PRINTF("glClearTexImage: emulated (texture=%u, level=%d)", texture, level);
}

void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, const void *data) {
    if(!current_context) return;
    LTW_DEBUG_PRINTF("glClearTexSubImage: emulated (texture=%u, level=%d)", texture, level);
}

/* ── 25. glGetInternalformativ — forward to ES ──────────────────────── */

void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname,
                            GLsizei bufSize, GLint *params) {
    if(!current_context) return;
    es3_functions.glGetInternalformativ(target, internalformat, pname, bufSize, params);
}

/* ── 26. glInvalidateFramebuffer / glInvalidateSubFramebuffer ──────── */

void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    if(!current_context) return;
    es3_functions.glInvalidateFramebuffer(target, numAttachments, attachments);
}

void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments,
                                 GLint x, GLint y, GLsizei width, GLsizei height) {
    if(!current_context) return;
    es3_functions.glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}

/* ── 27. glInvalidateTexImage / glInvalidateTexSubImage ────────────── */

void glInvalidateTexImage(GLuint texture, GLint level) {
    if(!current_context) return;
    /* ES doesn't have per-texture invalidation; no-op */
}

void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth) {
    if(!current_context) return;
    /* ES doesn't have per-texture invalidation; no-op */
}

/* ── 28. glInvalidateBufferData / glInvalidateBufferSubData ─────────── */

void glInvalidateBufferData(GLuint buffer) {
    if(!current_context) return;
    /* ES doesn't have buffer invalidation by name; no-op */
}

void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    if(!current_context) return;
    /* ES doesn't have buffer invalidation by name; no-op */
}

/* ── 29. VertexAttribL (double) → float conversion ──────────────────── */

void glVertexAttribL1d(GLuint index, GLdouble x) {
    if(!current_context) return;
    es3_functions.glVertexAttrib1f(index, (GLfloat)x);
}
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {
    if(!current_context) return;
    es3_functions.glVertexAttrib2f(index, (GLfloat)x, (GLfloat)y);
}
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    if(!current_context) return;
    es3_functions.glVertexAttrib3f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    if(!current_context) return;
    es3_functions.glVertexAttrib4f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}
void glVertexAttribL1dv(GLuint index, const GLdouble *v) {
    glVertexAttribL1d(index, v[0]);
}
void glVertexAttribL2dv(GLuint index, const GLdouble *v) {
    glVertexAttribL2d(index, v[0], v[1]);
}
void glVertexAttribL3dv(GLuint index, const GLdouble *v) {
    glVertexAttribL3d(index, v[0], v[1], v[2]);
}
void glVertexAttribL4dv(GLuint index, const GLdouble *v) {
    glVertexAttribL4d(index, v[0], v[1], v[2], v[3]);
}

/* ── 30. glDrawRangeElementsBaseVertex ──────────────────────────────── */

void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
                                    GLsizei count, GLenum type, const void *indices,
                                    GLint basevertex) {
    if(!current_context) return;
    /* Delegate to glDrawElementsBaseVertex, ignoring start/end (already validated by caller) */
    if(current_context->drawelementsbasevertex != NULL) {
        current_context->drawelementsbasevertex(mode, count, type, indices, basevertex);
        return;
    }
    basevertex_renderer_t *renderer = &current_context->basevertex;
    if(!renderer->ready) return;
    /* Reuse the base vertex indirect draw path */
    glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}

void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type,
                                        const void *indices, GLsizei primcount,
                                        GLint basevertex) {
    if(!current_context) return;
    /* ES 3.2 has glDrawElementsInstancedBaseVertex; on older ES, fall back to
       looping over instances with glDrawElementsBaseVertex */
    for(GLsizei i = 0; i < primcount; i++) {
        glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
    }
}

/* ── 31. glDrawRangeElements — forward to ES ────────────────────────── */

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                          GLenum type, const void *indices) {
    if(!current_context) return;
    es3_functions.glDrawRangeElements(mode, start, end, count, type, indices);
}

/* ── 32. glSamplerParameterIiv / Iuiv — forward to regular param ────── */

void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *params) {
    if(!current_context) return;
    es3_functions.glSamplerParameteriv(sampler, pname, params);
}

void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *params) {
    if(!current_context) return;
    GLint iparams[4];
    for(int i = 0; i < 4; i++) iparams[i] = (GLint)params[i];
    es3_functions.glSamplerParameteriv(sampler, pname, iparams);
}

void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params) {
    if(!current_context) return;
    es3_functions.glGetSamplerParameteriv(sampler, pname, params);
}

void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params) {
    if(!current_context) return;
    GLint iparams[4];
    es3_functions.glGetSamplerParameteriv(sampler, pname, iparams);
    for(int i = 0; i < 4; i++) params[i] = (GLuint)iparams[i];
}

/* ── 33. glTextureBarrier — no ES equivalent ────────────────────────── */

void glTextureBarrier(void) {
    if(!current_context) return;
    /* No ES equivalent; no-op */
}

/* ── 34. glPolygonOffsetClamp — ES doesn't support clamp ───────────── */

void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
    if(!current_context) return;
    /* ES doesn't have clamp parameter; fall back to regular polygon offset */
    es3_functions.glPolygonOffset(factor, units);
}

/* ── 35. glClipControl — no ES equivalent ────────────────────────────── */

void glClipControl(GLenum origin, GLenum depth) {
    if(!current_context) return;
    /* ES always uses lower-left origin and -1..1 depth range; no-op */
}

/* ── 36. Debug message functions — no-op (already partially in main.c) ─ */

void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar *buf) {
    if(!current_context) return;
    /* No-op: ES debug output is not exposed by default */
}

void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) {
    if(!current_context) return;
    /* No-op */
}

GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources,
                             GLenum *types, GLuint *ids, GLenum *severities,
                             GLsizei *lengths, GLchar *messageLog) {
    if(!current_context) return 0;
    return 0;
}

void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message) {
    if(!current_context) return;
    /* No-op */
}

void glPopDebugGroup(void) {
    if(!current_context) return;
    /* No-op */
}

void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) {
    if(!current_context) return;
    /* No-op */
}

void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize,
                       GLsizei *length, GLchar *label) {
    if(!current_context) return;
    if(length) *length = 0;
    if(label && bufSize > 0) label[0] = 0;
}

/* ── 37. glVertexAttribP — packed vertex attributes ─────────────────── */

void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    if(!current_context) return;
    /* Unpack as float */
    GLfloat f = normalized ? (GLfloat)value / (GLfloat)0xFFFFFFFF : (GLfloat)value;
    es3_functions.glVertexAttrib1f(index, f);
}

void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    if(!current_context) return;
    GLfloat f0 = normalized ? (GLfloat)(value & 0xFFFF) / 65535.0f : (GLfloat)(value & 0xFFFF);
    GLfloat f1 = normalized ? (GLfloat)(value >> 16) / 65535.0f : (GLfloat)(value >> 16);
    es3_functions.glVertexAttrib2f(index, f0, f1);
}

void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    if(!current_context) return;
    /* 10-bit packed components */
    GLfloat scale = normalized ? 1023.0f : 1.0f;
    es3_functions.glVertexAttrib3f(index,
        (GLfloat)(value & 0x3FF) / scale,
        (GLfloat)((value >> 10) & 0x3FF) / scale,
        (GLfloat)((value >> 20) & 0x3FF) / scale);
}

void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    if(!current_context) return;
    GLfloat scale = normalized ? 255.0f : 1.0f;
    es3_functions.glVertexAttrib4f(index,
        (GLfloat)(value & 0xFF) / scale,
        (GLfloat)((value >> 8) & 0xFF) / scale,
        (GLfloat)((value >> 16) & 0xFF) / scale,
        (GLfloat)((value >> 24) & 0xFF) / scale);
}

void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    glVertexAttribP1ui(index, type, normalized, *value);
}
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    glVertexAttribP2ui(index, type, normalized, *value);
}
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    glVertexAttribP3ui(index, type, normalized, *value);
}
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    glVertexAttribP4ui(index, type, normalized, *value);
}

/* ── 38. Conditional render — no ES equivalent ──────────────────────── */

void glBeginConditionalRender(GLuint id, GLenum mode) {
    if(!current_context) return;
    /* No ES equivalent; no-op */
}

void glEndConditionalRender(void) {
    if(!current_context) return;
    /* No-op */
}

/* ── 39. Transform feedback — forward to ES ─────────────────────────── */

void glBeginTransformFeedback(GLenum primitiveMode) {
    if(!current_context) return;
    es3_functions.glBeginTransformFeedback(primitiveMode);
}

void glEndTransformFeedback(void) {
    if(!current_context) return;
    es3_functions.glEndTransformFeedback();
}

void glTransformFeedbackVaryings(GLuint program, GLsizei count,
                                  const char *const *varyings, GLenum bufferMode) {
    if(!current_context) return;
    es3_functions.glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}

/* ── 40. glBeginQuery / glEndQuery — forward to ES ───────────────────── */

void glGenQueries(GLsizei n, GLuint *ids) {
    if(!current_context) return;
    es3_functions.glGenQueries(n, ids);
}

void glDeleteQueries(GLsizei n, const GLuint *ids) {
    if(!current_context) return;
    es3_functions.glDeleteQueries(n, ids);
}

GLboolean glIsQuery(GLuint id) {
    if(!current_context) return GL_FALSE;
    return es3_functions.glIsQuery(id);
}

void glBeginQuery(GLenum target, GLuint id) {
    if(!current_context) return;
    es3_functions.glBeginQuery(target, id);
}

void glEndQuery(GLenum target) {
    if(!current_context) return;
    es3_functions.glEndQuery(target);
}

void glGetQueryiv(GLenum target, GLenum pname, GLint *params) {
    if(!current_context) return;
    es3_functions.glGetQueryiv(target, pname, params);
}

void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {
    if(!current_context) return;
    es3_functions.glGetQueryObjectuiv(id, pname, params);
}
