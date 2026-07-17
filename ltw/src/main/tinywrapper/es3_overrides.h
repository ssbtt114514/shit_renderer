/**
 * Created by: artDev
 * Copyright (c) 2025 artDev, SerpentSpirale, CADIndie.
 * For use under LGPL-3.0
 */
void glClearDepth(double depth);
void* glMapBuffer(GLenum target, GLenum access);
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
void glDebugMessageControl( 	GLenum source,
                               GLenum type,
                               GLenum severity,
                               GLsizei count,
                               const GLuint *ids,
                               GLboolean enabled);
void glMultiDrawElementsBaseVertex( 	GLenum mode,
                                       const GLsizei *count,
                                       GLenum type,
                                       const void * const *indices,
                                       GLsizei drawcount,
                                       const GLint *basevertex);
void glBindFragDataLocation(GLuint program,
                            GLuint colorNumber,
                            const char * name);
void glGetTexImage( 	GLenum target,
                       GLint level,
                       GLenum format,
                       GLenum type,
                       void * pixels);

void glGetQueryObjectiv( 	GLuint id,
                            GLenum pname,
                            GLint * params);

void glDepthRange(GLdouble nearVal,
                  GLdouble farVal);

// ARB matrix uniform aliases (defined in gl_compat.c)
void glUniformMatrix2x3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix3x2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix2x4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix4x2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix3x4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix4x3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

// EXT tex storage aliases (defined in gl_compat.c)
void glTexStorage1DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
void glTexStorage2DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
void glTexStorage3DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

// ARB tex buffer range alias (defined in gl_compat.c)
void glTexBufferRangeARB(GLenum target, GLenum internalFormat, GLuint buffer, GLintptr offset, GLsizeiptr size);

GLESOVERRIDE(glClearDepth)
GLESOVERRIDE(glMapBuffer)
GLESOVERRIDE(glGetTexLevelParameteriv)
GLESOVERRIDE(glGetTexLevelParameterfv)
GLESOVERRIDE(glCreateShader)
GLESOVERRIDE(glDeleteShader)
GLESOVERRIDE(glCreateProgram)
GLESOVERRIDE(glDeleteProgram)
GLESOVERRIDE(glLinkProgram)
GLESOVERRIDE(glAttachShader)
GLESOVERRIDE(glGetShaderiv)
GLESOVERRIDE(glShaderSource)
GLESOVERRIDE(glTexImage2D)
GLESOVERRIDE(glDebugMessageControl)
GLESOVERRIDE(glGetString)
GLESOVERRIDE(glEnable)
GLESOVERRIDE(glMultiDrawArrays)
GLESOVERRIDE(glMultiDrawElements)
GLESOVERRIDE(glMultiDrawElementsBaseVertex)
GLESOVERRIDE(glDrawElementsBaseVertex)
GLESOVERRIDE(glBindBufferBase)
GLESOVERRIDE(glBindBufferRange)
GLESOVERRIDE(glBindBuffer)
GLESOVERRIDE(glUseProgram)
GLESOVERRIDE(glGetIntegerv)
GLESOVERRIDE(glBindFramebuffer)
GLESOVERRIDE(glGenFramebuffers)
GLESOVERRIDE(glDeleteFramebuffers)
GLESOVERRIDE(glFramebufferTexture2D)
GLESOVERRIDE(glFramebufferTextureLayer)
GLESOVERRIDE(glFramebufferRenderbuffer)
GLESOVERRIDE(glGetFramebufferAttachmentParameteriv)
GLESOVERRIDE(glDrawBuffers)
GLESOVERRIDE(glDrawBuffer)
GLESOVERRIDE(glClearBufferiv)
GLESOVERRIDE(glClearBufferuiv)
GLESOVERRIDE(glClearBufferfv)
GLESOVERRIDE(glCheckFramebufferStatus)
GLESOVERRIDE(glReadPixels)
GLESOVERRIDE(glTexSubImage2D)
GLESOVERRIDE(glCopyTexSubImage2D)
GLESOVERRIDE(glTexParameteri)
GLESOVERRIDE(glBindFragDataLocation)
GLESOVERRIDE(glGetTexImage)
GLESOVERRIDE(glGetQueryObjectiv)
GLESOVERRIDE(glGetQueryObjecti64v)
GLESOVERRIDE(glGetQueryObjectui64v)
GLESOVERRIDE(glDepthRange)
GLESOVERRIDE(glVertexAttrib1d)
GLESOVERRIDE(glVertexAttrib1dv)
GLESOVERRIDE(glVertexAttrib1s)
GLESOVERRIDE(glVertexAttrib1sv)
GLESOVERRIDE(glVertexAttrib2d)
GLESOVERRIDE(glVertexAttrib2dv)
GLESOVERRIDE(glVertexAttrib2s)
GLESOVERRIDE(glVertexAttrib2sv)
GLESOVERRIDE(glVertexAttrib3d)
GLESOVERRIDE(glVertexAttrib3dv)
GLESOVERRIDE(glVertexAttrib3s)
GLESOVERRIDE(glVertexAttrib3sv)
GLESOVERRIDE(glVertexAttrib4d)
GLESOVERRIDE(glVertexAttrib4dv)
GLESOVERRIDE(glVertexAttrib4s)
GLESOVERRIDE(glVertexAttrib4sv)
GLESOVERRIDE(glVertexAttrib4Nbv)
GLESOVERRIDE(glVertexAttrib4Niv)
GLESOVERRIDE(glVertexAttrib4Nsv)
GLESOVERRIDE(glVertexAttrib4Nub)
GLESOVERRIDE(glVertexAttrib4Nubv)
GLESOVERRIDE(glVertexAttrib4Nuiv)
GLESOVERRIDE(glVertexAttrib4Nusv)
GLESOVERRIDE(glVertexAttribI1i)
GLESOVERRIDE(glVertexAttribI1iv)
GLESOVERRIDE(glVertexAttribI1ui)
GLESOVERRIDE(glVertexAttribI1uiv)
GLESOVERRIDE(glVertexAttribI2i)
GLESOVERRIDE(glVertexAttribI2iv)
GLESOVERRIDE(glVertexAttribI2ui)
GLESOVERRIDE(glVertexAttribI2uiv)
GLESOVERRIDE(glVertexAttribI3i)
GLESOVERRIDE(glVertexAttribI3iv)
GLESOVERRIDE(glVertexAttribI3ui)
GLESOVERRIDE(glVertexAttribI3uiv)
GLESOVERRIDE(glVertexAttribI4bv)
GLESOVERRIDE(glVertexAttribI4ubv)
GLESOVERRIDE(glVertexAttribI4sv)
GLESOVERRIDE(glVertexAttribI4usv)
GLESOVERRIDE(glBufferStorage)
GLESOVERRIDE(glGetStringi)
GLESOVERRIDE(glTexParameterf)
GLESOVERRIDE(glTexParameteri)
GLESOVERRIDE(glTexParameterfv)
GLESOVERRIDE(glTexParameteriv)
GLESOVERRIDE(glTexParameterIiv)
GLESOVERRIDE(glTexParameterIuiv)
GLESOVERRIDE(glRenderbufferStorage)
GLESOVERRIDE(glGetError)
GLESOVERRIDE(glTexBuffer)
GLESOVERRIDE(glTexBufferRange)
GLESOVERRIDE(glMapBufferRange)
GLESOVERRIDE(glFlushMappedBufferRange)

// ── gl_compat.c: Advanced OpenGL compatibility for MC 1.17- ────────────
// Polygon mode (no ES equivalent; tracking only)
GLESOVERRIDE(glPolygonMode)
// 1D textures (emulated as 2D with height=1)
GLESOVERRIDE(glTexImage1D)
GLESOVERRIDE(glTexSubImage1D)
GLESOVERRIDE(glCopyTexImage1D)
GLESOVERRIDE(glCopyTexSubImage1D)
GLESOVERRIDE(glCompressedTexImage1D)
GLESOVERRIDE(glCompressedTexSubImage1D)
GLESOVERRIDE(glTexStorage1D)
// Sync objects (forward to ES 3.0)
GLESOVERRIDE(glFenceSync)
GLESOVERRIDE(glIsSync)
GLESOVERRIDE(glDeleteSync)
GLESOVERRIDE(glClientWaitSync)
GLESOVERRIDE(glWaitSync)
GLESOVERRIDE(glGetSynciv)
GLESOVERRIDE(glGetInteger64v)
GLESOVERRIDE(glGetInteger64i_v)
GLESOVERRIDE(glGetBufferParameteri64v)
// No-op stubs (no ES equivalent)
GLESOVERRIDE(glClampColor)
GLESOVERRIDE(glProvokingVertex)
GLESOVERRIDE(glPrimitiveRestartIndex)
// Framebuffer texture (non-layered)
GLESOVERRIDE(glFramebufferTexture)
GLESOVERRIDE(glFramebufferTexture3D)
GLESOVERRIDE(glFramebufferTexture1D)
// Buffer clearing (manual implementation)
GLESOVERRIDE(glClearBufferData)
GLESOVERRIDE(glClearBufferSubData)
// ES 3.1 compute/memory/image forwarding
GLESOVERRIDE(glDispatchCompute)
GLESOVERRIDE(glDispatchComputeIndirect)
GLESOVERRIDE(glMemoryBarrier)
GLESOVERRIDE(glBindImageTexture)
// ES 3.2 copy image
GLESOVERRIDE(glCopyImageSubData)
// Double → float query conversion
GLESOVERRIDE(glGetDoublev)
GLESOVERRIDE(glGetTexParameterIiv)
GLESOVERRIDE(glGetTexParameterIuiv)
// Per-index enable/disable
GLESOVERRIDE(glEnablei)
GLESOVERRIDE(glDisablei)
GLESOVERRIDE(glIsEnabledi)
GLESOVERRIDE(glGetBooleani_v)
// Tessellation patch params (no-op)
GLESOVERRIDE(glPatchParameteri)
GLESOVERRIDE(glPatchParameterfv)
// ARB instanced draw aliases
GLESOVERRIDE(glDrawArraysInstancedARB)
GLESOVERRIDE(glDrawElementsInstancedARB)
// ARB matrix uniform aliases
GLESOVERRIDE(glUniformMatrix2x3fvARB)
GLESOVERRIDE(glUniformMatrix3x2fvARB)
GLESOVERRIDE(glUniformMatrix2x4fvARB)
GLESOVERRIDE(glUniformMatrix4x2fvARB)
GLESOVERRIDE(glUniformMatrix3x4fvARB)
GLESOVERRIDE(glUniformMatrix4x3fvARB)
// Double uniforms → float conversion
GLESOVERRIDE(glUniform1d)
GLESOVERRIDE(glUniform2d)
GLESOVERRIDE(glUniform3d)
GLESOVERRIDE(glUniform4d)
GLESOVERRIDE(glUniform1dv)
GLESOVERRIDE(glUniform2dv)
GLESOVERRIDE(glUniform3dv)
GLESOVERRIDE(glUniform4dv)
GLESOVERRIDE(glUniformMatrix2dv)
GLESOVERRIDE(glUniformMatrix3dv)
GLESOVERRIDE(glUniformMatrix4dv)
GLESOVERRIDE(glUniformMatrix2x3dv)
GLESOVERRIDE(glUniformMatrix3x2dv)
GLESOVERRIDE(glUniformMatrix2x4dv)
GLESOVERRIDE(glUniformMatrix4x2dv)
GLESOVERRIDE(glUniformMatrix3x4dv)
GLESOVERRIDE(glUniformMatrix4x3dv)
// EXT tex storage aliases
GLESOVERRIDE(glTexStorage1DEXT)
GLESOVERRIDE(glTexStorage2DEXT)
GLESOVERRIDE(glTexStorage3DEXT)
// ARB framebuffer texture aliases
GLESOVERRIDE(glFramebufferTextureARB)
GLESOVERRIDE(glFramebufferTextureLayerARB)
// Program uniform (program switch + uniform)
GLESOVERRIDE(glProgramUniform1f)
GLESOVERRIDE(glProgramUniform1i)
GLESOVERRIDE(glProgramUniform1fv)
GLESOVERRIDE(glProgramUniform1iv)
GLESOVERRIDE(glProgramUniform2f)
GLESOVERRIDE(glProgramUniform2i)
GLESOVERRIDE(glProgramUniform3f)
GLESOVERRIDE(glProgramUniform4f)
GLESOVERRIDE(glProgramUniformMatrix4fv)
// ARB tex buffer range
GLESOVERRIDE(glTexBufferRangeARB)
// Texture clearing
GLESOVERRIDE(glClearTexImage)
GLESOVERRIDE(glClearTexSubImage)
// Internal format query
GLESOVERRIDE(glGetInternalformativ)
// Framebuffer invalidation
GLESOVERRIDE(glInvalidateFramebuffer)
GLESOVERRIDE(glInvalidateSubFramebuffer)
GLESOVERRIDE(glInvalidateTexImage)
GLESOVERRIDE(glInvalidateTexSubImage)
GLESOVERRIDE(glInvalidateBufferData)
GLESOVERRIDE(glInvalidateBufferSubData)
// VertexAttribL (double → float)
GLESOVERRIDE(glVertexAttribL1d)
GLESOVERRIDE(glVertexAttribL2d)
GLESOVERRIDE(glVertexAttribL3d)
GLESOVERRIDE(glVertexAttribL4d)
GLESOVERRIDE(glVertexAttribL1dv)
GLESOVERRIDE(glVertexAttribL2dv)
GLESOVERRIDE(glVertexAttribL3dv)
GLESOVERRIDE(glVertexAttribL4dv)
// Base vertex draw variants
GLESOVERRIDE(glDrawRangeElementsBaseVertex)
GLESOVERRIDE(glDrawElementsInstancedBaseVertex)
GLESOVERRIDE(glDrawRangeElements)
// Sampler parameter I variants
GLESOVERRIDE(glSamplerParameterIiv)
GLESOVERRIDE(glSamplerParameterIuiv)
GLESOVERRIDE(glGetSamplerParameterIiv)
GLESOVERRIDE(glGetSamplerParameterIuiv)
// No-op stubs
GLESOVERRIDE(glTextureBarrier)
GLESOVERRIDE(glPolygonOffsetClamp)
GLESOVERRIDE(glClipControl)
// Debug messages (no-op)
GLESOVERRIDE(glDebugMessageInsert)
GLESOVERRIDE(glDebugMessageCallback)
GLESOVERRIDE(glGetDebugMessageLog)
GLESOVERRIDE(glPushDebugGroup)
GLESOVERRIDE(glPopDebugGroup)
GLESOVERRIDE(glObjectLabel)
GLESOVERRIDE(glGetObjectLabel)
// Packed vertex attribs
GLESOVERRIDE(glVertexAttribP1ui)
GLESOVERRIDE(glVertexAttribP2ui)
GLESOVERRIDE(glVertexAttribP3ui)
GLESOVERRIDE(glVertexAttribP4ui)
GLESOVERRIDE(glVertexAttribP1uiv)
GLESOVERRIDE(glVertexAttribP2uiv)
GLESOVERRIDE(glVertexAttribP3uiv)
GLESOVERRIDE(glVertexAttribP4uiv)
// Conditional render (no-op)
GLESOVERRIDE(glBeginConditionalRender)
GLESOVERRIDE(glEndConditionalRender)
// Transform feedback
GLESOVERRIDE(glBeginTransformFeedback)
GLESOVERRIDE(glEndTransformFeedback)
GLESOVERRIDE(glTransformFeedbackVaryings)
// Query objects
GLESOVERRIDE(glGenQueries)
GLESOVERRIDE(glDeleteQueries)
GLESOVERRIDE(glIsQuery)
GLESOVERRIDE(glBeginQuery)
GLESOVERRIDE(glEndQuery)
GLESOVERRIDE(glGetQueryiv)
GLESOVERRIDE(glGetQueryObjectuiv)