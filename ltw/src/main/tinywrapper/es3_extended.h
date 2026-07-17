/**
 * Created by: artDev
 * Copyright (c) 2025 artDev, SerpentSpirale, CADIndie.
 * For use under LGPL-3.0
 */

// Added manually as needed
GLESFUNC(glDrawElementsIndirect,PFNGLDRAWELEMENTSINDIRECTPROC)
GLESFUNC(glMultiDrawArraysEXT,PFNGLMULTIDRAWARRAYSEXTPROC)
GLESFUNC(glMultiDrawElementsEXT,PFNGLMULTIDRAWELEMENTSEXTPROC)
GLESFUNC(glGetTexLevelParameteriv,PFNGLGETTEXLEVELPARAMETERIVPROC)
GLESFUNC(glGetTexLevelParameterfv,PFNGLGETTEXLEVELPARAMETERFVPROC)
GLESFUNC(glDrawElementsBaseVertex, PFNGLDRAWELEMENTSBASEVERTEXPROC)
GLESFUNC(glDrawElementsBaseVertexOES, PFNGLDRAWELEMENTSBASEVERTEXOESPROC)
GLESFUNC(glDrawElementsBaseVertexEXT, PFNGLDRAWELEMENTSBASEVERTEXEXTPROC)
GLESFUNC(glBlendEquationi, PFNGLBLENDEQUATIONIPROC)
GLESFUNC(glBlendEquationiOES, PFNGLBLENDEQUATIONIOESPROC)
GLESFUNC(glBlendEquationiEXT, PFNGLBLENDEQUATIONIEXTPROC)
GLESFUNC(glBlendEquationSeparatei, PFNGLBLENDEQUATIONSEPARATEIPROC)
GLESFUNC(glBlendEquationSeparateiOES, PFNGLBLENDEQUATIONSEPARATEIOESPROC)
GLESFUNC(glBlendEquationSeparateiEXT, PFNGLBLENDEQUATIONSEPARATEIEXTPROC)
GLESFUNC(glBlendFunci, PFNGLBLENDFUNCIPROC)
GLESFUNC(glBlendFunciOES, PFNGLBLENDFUNCIOESPROC)
GLESFUNC(glBlendFunciEXT, PFNGLBLENDFUNCIEXTPROC)
GLESFUNC(glBlendFuncSeparatei, PFNGLBLENDFUNCSEPARATEIPROC)
GLESFUNC(glBlendFuncSeparateiOES, PFNGLBLENDFUNCSEPARATEIOESPROC)
GLESFUNC(glBlendFuncSeparateiEXT, PFNGLBLENDFUNCSEPARATEIEXTPROC)
GLESFUNC(glColorMaski, PFNGLCOLORMASKIPROC)
GLESFUNC(glColorMaskiOES, PFNGLCOLORMASKIOESPROC)
GLESFUNC(glColorMaskiEXT, PFNGLCOLORMASKIEXTPROC)
GLESFUNC(glBufferStorageEXT, PFNGLBUFFERSTORAGEEXTPROC)
GLESFUNC(glTexBuffer, PFNGLTEXBUFFERPROC);
GLESFUNC(glTexBufferRange, PFNGLTEXBUFFERRANGEPROC);
GLESFUNC(glTexBufferEXT, PFNGLTEXBUFFEREXTPROC)
GLESFUNC(glTexBufferRangeEXT, PFNGLTEXBUFFERRANGEEXTPROC)
GLESFUNC(glMultiDrawElementsIndirectEXT, PFNGLMULTIDRAWELEMENTSINDIRECTEXTPROC)
GLESFUNC(glGetQueryObjecti64vEXT, PFNGLGETQUERYOBJECTI64VEXTPROC)
GLESFUNC(glGetQueryObjectui64vEXT, PFNGLGETQUERYOBJECTUI64VEXTPROC)
GLESFUNC(glQueryCounterEXT, PFNGLQUERYCOUNTEREXTPROC)

// OpenGL ES 3.1 core functions (same names as desktop GL; resolved via
// eglGetProcAddress on ES 3.1+ devices).  Loaded here so LTW can verify
// availability before advertising the matching ARB extension, and use
// them internally when needed.  NULL on ES 3.0 — that is expected.
// NOTE: glMultiDrawArraysIndirect / glMultiDrawElementsIndirect are NOT ES
// core — they are desktop GL 4.3 core and only exist in ES as the
// GL_EXT_multi_draw_indirect extension (EXT-suffixed entry points, already
// declared above).  Do not add the unsuffixed typedef here: the NDK ES
// headers do not define PFNGL*MULTIDRAW*INDIRECTPROC without an EXT/OES suffix,
// and doing so breaks the build with "unknown type name".
GLESFUNC(glFramebufferParameteri, PFNGLFRAMEBUFFERPARAMETERIPROC)
GLESFUNC(glGetFramebufferParameteriv, PFNGLGETFRAMEBUFFERPARAMETERIVPROC)
GLESFUNC(glBindVertexBuffer, PFNGLBINDVERTEXBUFFERPROC)
GLESFUNC(glVertexAttribBinding, PFNGLVERTEXATTRIBBINDINGPROC)
GLESFUNC(glVertexAttribFormat, PFNGLVERTEXATTRIBFORMATPROC)
GLESFUNC(glVertexAttribIFormat, PFNGLVERTEXATTRIBIFORMATPROC)
GLESFUNC(glVertexBindingDivisor, PFNGLVERTEXBINDINGDIVISORPROC)

// OpenGL ES 3.1 core — compute, memory barriers, image units, clear buffer
GLESFUNC(glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC)
GLESFUNC(glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC)
GLESFUNC(glMemoryBarrier, PFNGLMEMORYBARRIERPROC)
GLESFUNC(glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC)

// OpenGL ES 3.2 core — framebuffer texture, copy image
// NOTE: glPrimitiveRestartIndex is implemented in gl_compat.c and exposed via
// es3_overrides.h; it is NOT loaded through eglGetProcAddress here because the
// NDK ES headers do not define PFNGLPRIMITIVERESTARTINDEXPROC.
GLESFUNC(glFramebufferTexture, PFNGLFRAMEBUFFERTEXTUREPROC)
GLESFUNC(glCopyImageSubData, PFNGLCOPYIMAGESUBDATAPROC)

// NOTE: glTextureView / glMinSampleShading are ES 3.2 core functions promoted
// from OES extensions.  The NDK gl32.h does NOT define PFNGLTEXTUREVIEWPROC or
// PFNGLMINSAMPLESHADINGPROC (only the OES-suffixed variants).  Do not add the
// unsuffixed GLESFUNC lines here — they break the build with "unknown type name".
// When support is needed, use the OES-suffixed entry points with a forwarding
// override in es3_overrides.h.