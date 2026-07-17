## shit renderer
A thin OpenGL core-to-OpenGL ES wrapper, primarily intended for running Minecraft.

# Prerequisites

| Tool | Version |
|---|---|
| JDK | 17 |
| Android SDK | Platform 34 (`android-34`) |
| Android NDK | `28.2.13676358` |
| Gradle | 7.5 (wrapper included) |
| Android Gradle Plugin | 7.4.1 |

The build is driven by the bundled Gradle wrapper, so no system-wide Gradle
installation is required. AGP will download the pinned NDK on demand if it is
not already present in the local SDK.

# Building

```bash
./gradlew :ltw:assembleRelease        # Unix / macOS / Git Bash
gradlew.bat :ltw:assembleRelease      # Windows (cmd / PowerShell)
```

After completion, an AAR with native libraries will be available in
`ltw/build/outputs/aar/ltw-release.aar`.

## Target ABIs

LTW ships native libraries for every Android ABI supported by NDK 28. The
`APP_ABI` directive in [`Application.mk`](ltw/src/main/tinywrapper/Application.mk)
and the `ndk.abiFilters` block in [`ltw/build.gradle`](ltw/build.gradle) are kept
in sync so the same set is compiled by `ndk-build` and packaged into the AAR.

| ABI | Architecture | Typical devices |
|---|---|---|
| `armeabi-v7a` | ARMv7-A (32-bit, arm32) | Older / low-end Android phones |
| `arm64-v8a` | ARMv8-A (64-bit, arm64) | Modern Android phones & tablets |
| `x86` | IA-32 (32-bit) | x86 Android emulators |
| `x86_64` | x86-64 (64-bit, x64) | x86-64 emulators & ChromeOS |

All four ABIs are built from a single invocation; there is no need to select a
target. Each ABI lives in its own `lib/<abi>/libltw.so` entry inside the AAR.

## Continuous integration

The repository ships a GitHub Actions workflow at
[`.github/workflows/ci.yml`](.github/workflows/ci.yml) that:

1. Sets up JDK 17, the Android SDK and the pinned NDK.
2. Builds the release AAR with `./gradlew :ltw:assembleRelease`.
3. Verifies the AAR exists and is non-trivial in size.
4. Checks that `libltw.so` is present for **every** expected ABI
   (`armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`); the run fails if any are missing.
5. Uploads the AAR as a workflow artifact.
6. On a pushed `v*` tag, publishes a GitHub Release with the AAR attached.

The workflow triggers on pushes to `main`/`master`, on pull requests, on version
tags, and via manual `workflow_dispatch`.

# OpenGL requirements

## Minecraft: Java Edition

The latest release of Minecraft: Java Edition (1.21.x) is built on the Blaze3D renderer
on top of LWJGL 3.3.x. Mojang's officially posted minimum remains **OpenGL 4.0**
(in effect since 2017); however, since release 1.21.6 the game relies on **OpenGL 4.4**-class
features in practice and an OpenGL 4.4-capable GPU is required for stable operation. A GPU
exposing **OpenGL 4.5** is recommended.

Rather than depending on a single version number, Blaze3D queries for individual extensions
and adapts its code paths at runtime. The desktop features it depends on are:

| Feature | Core in | Used for | Exposed by LTW as |
|---|---|---|---|
| Persistent mapped buffers (`glBufferStorage`) | OpenGL 4.4 | Upload/streaming vertex & index buffers | `GL_ARB_buffer_storage` |
| GPU timer queries (`glQueryCounter`) | OpenGL 3.3 | GPU usage counter (Blaze3D `TimerQuery`) | `GL_ARB_timer_query` |
| Base-vertex drawing (`glDrawElementsBaseVertex`) | OpenGL 3.2 | Indexed draws with a vertex offset | `GL_ARB_draw_elements_base_vertex` |
| Per-draw-buffer blending | OpenGL 4.0 | Independent blend per render target (required by Iris) | `GL_ARB_draw_buffers_blend` |
| Texture buffers (`glTexBuffer`) | OpenGL 3.1 | Bind a buffer object to a sampler | `GL_ARB_texture_buffer_object` |

## What LTW exposes

LTW translates these desktop OpenGL calls down to OpenGL ES. To the host application it
reports an **OpenGL 3.3** core profile (`GL_VERSION`) together with a **GLSL 4.60** shading
language version, layered with the ARB extensions listed above. This 3.3 + curated-ARB set
is what satisfies the feature checks Minecraft performs at startup.

Reported strings:

- `GL_VERSION` → `3.3 shit renderer (Built on: <date>/<time>)`
- `GL_SHADING_LANGUAGE_VERSION` → `4.60 shit renderer`
- `GL_VENDOR` → `<ro.hardware>/<ro.board.platform> - ssbtt` (e.g. `qcom/sdm865 - ssbtt`)
- `GL_RENDERER` → `shit renderer`

Exposed ARB extensions (see `build_extension_string()` in [egl.c](ltw/src/main/tinywrapper/egl.c)):

Always exposed:

- `GL_ARB_buffer_storage` — persistent mapped buffers; backed by `GL_EXT_buffer_storage`.
- `GL_ARB_texture_buffer_object` — texture buffers; backed by `GL_EXT_texture_buffer` or OpenGL ES 3.2.
- `GL_ARB_draw_elements_base_vertex` — base-vertex indexed drawing.
- `GL_ARB_draw_buffers_blend` — per-target blend state; required by Iris. Needs OpenGL ES 3.2 or `GL_OES`/`GL_EXT_draw_buffers_indexed`.
- `GL_ARB_timer_query` — GPU timer queries used by Minecraft's GPU usage counter.
- `GL_ARB_texture_storage` — immutable texture storage; `glTexStorage2D/3D` are OpenGL ES 3.0 core and pass through directly.
- `GL_ARB_shading_language_packing` — `packUnorm4x8`/`unpackUnorm4x8` etc.; ESSL 3.00 provides these built-ins natively.

Exposed on OpenGL ES 3.1+ devices (entry points resolve 1:1 to the same-named ES 3.1 core functions; GLSL is lowered by the optimizer):

- `GL_ARB_draw_indirect`, `GL_ARB_texture_multisample`, `GL_ARB_texture_storage_multisample`, `GL_ARB_stencil_texturing`
- `GL_ARB_shader_storage_buffer_object`, `GL_ARB_compute_shader`, `GL_ARB_shader_image_load_store`, `GL_ARB_shader_image_size`, `GL_ARB_shader_atomic_counters`
- `GL_ARB_program_interface_query`
- `GL_ARB_gpu_shader5` — `precise`, `fma`, bitfield operations; ESSL 3.10 provides these.

Exposed when the backing ES 3.1/3.2 core entry points actually resolved via `eglGetProcAddress` (probed at context creation, so no silent no-op stubs):

- `GL_ARB_framebuffer_no_attachments` — `glFramebufferParameteri` / `glGetFramebufferParameteriv` (ES 3.1 core).
- `GL_ARB_vertex_attrib_binding` — `glBindVertexBuffer`, `glVertexAttribBinding`, `glVertexAttribFormat`, `glVertexBindingDivisor` (ES 3.1 core).

Exposed on OpenGL ES 3.2 devices:

- `GL_ARB_copy_image` — `glCopyImageSubData`, OpenGL ES 3.2 core.

## OpenGL ES device requirements

LTW requires a device exposing at least **OpenGL ES 3.0** with **ESSL 3.00**. OpenGL ES 3.1
and 3.2 enable additional fast paths. If the device reports ES < 3.0 or ESSL < 3.00, LTW
prints a warning, as this configuration is unsupported and will cause problems.

The following device-side ES extensions are detected and used when present:

| OpenGL ES extension | Backs | Notes |
|---|---|---|
| `GL_EXT_buffer_storage` | `GL_ARB_buffer_storage` | Persistent mapped buffers |
| `GL_EXT_texture_buffer` | `GL_ARB_texture_buffer_object` | Texture buffers on ES 3.0/3.1 |
| `GL_EXT_multi_draw_indirect` | multi-draw-indirect dispatch | Indirect drawing |
| `GL_EXT_disjoint_timer_query` | `GL_ARB_timer_query` | Accurate GPU timestamps |
| `GL_OES_draw_elements_base_vertex` / `GL_EXT_draw_elements_base_vertex` | `GL_ARB_draw_elements_base_vertex` | Base-vertex on ES 3.0/3.1 |
| `GL_OES_draw_buffers_indexed` / `GL_EXT_draw_buffers_indexed` | `GL_ARB_draw_buffers_blend` | Per-target blending on ES 3.0/3.1 |

The GLSL shader converter additionally injects `GL_EXT_texture_cube_map_array`,
`GL_EXT_texture_buffer`, `GL_OES_texture_storage_multisample_2d_array` and
`GL_EXT_shader_non_constant_global_initializers` when translating desktop GLSL to ESSL.

## Environment variables

| Variable | Default | Effect |
|---|---|---|
| `LTW_HIDE_BUFFER_STORAGE` | unset | When set to `1`, hides `GL_ARB_buffer_storage` from the application. |
| `LTW_ENABLE_TIMER_QUERY` | unset | When set to `1`, force-enables timer queries even without `GL_EXT_disjoint_timer_query` (faked queries, see `query.c`). |
| `LTW_GL_VERSION` | unset | Set to `3.2` to report OpenGL 3.2 / GLSL 1.50 and hide ES 3.1/3.2-only extensions, for compatibility with Minecraft 1.17 and older. |
| `LIBGL_NOERROR` | unset | When set to `1`, glGetError() always returns 0. Useful for mods that don't handle errors properly. |
| `LTW_DEBUG` | unset | When set to `1`, enables GL_DEBUG_OUTPUT and additional debug logging. |
| `LTW_NEVER_FLUSH_BUFFERS` | `1` | When set, prevents explicit buffer flushes (glFlushMappedBufferRange), improves performance on some drivers. |
| `LTW_COHERENT_DYNAMIC_STORAGE` | `1` | When set, forces dynamic storage buffers to be coherent, working around driver bugs. |

## Advanced OpenGL compatibility for Minecraft 1.17 and below

LTW borrows design patterns from [GL4ES](https://github.com/ptitSeb/gl4es) to provide a
comprehensive set of desktop OpenGL compatibility functions in
[`gl_compat.c`](ltw/src/main/tinywrapper/gl_compat.c). These functions are essential for
running Minecraft 1.17 and earlier versions, which use a broader range of desktop GL APIs.

### Implemented function categories

| Category | Functions | Implementation strategy |
|---|---|---|
| **Polygon mode** | `glPolygonMode` | Tracking only (ES only supports GL_FILL); state is queryable via `glGetIntegerv` |
| **1D textures** | `glTexImage1D`, `glTexSubImage1D`, `glCopyTexImage1D`, `glCopyTexSubImage1D`, `glCompressedTexImage1D`, `glCompressedTexSubImage1D`, `glTexStorage1D` | Emulated as 2D textures with height=1 |
| **Sync objects** | `glFenceSync`, `glIsSync`, `glDeleteSync`, `glClientWaitSync`, `glWaitSync`, `glGetSynciv`, `glGetInteger64v`, `glGetInteger64i_v`, `glGetBufferParameteri64v` | Forwarded to OpenGL ES 3.0 core sync functions |
| **Framebuffer texture** | `glFramebufferTexture`, `glFramebufferTexture3D`, `glFramebufferTexture1D` | `glFramebufferTexture` uses native ES 3.2 on 3.2 devices, falls back to `glFramebufferTextureLayer` with layer=0 on older ES |
| **Buffer clearing** | `glClearBufferData`, `glClearBufferSubData` | Emulated via `glMapBufferRange` + `memcpy` pattern fill |
| **Compute & memory** | `glDispatchCompute`, `glDispatchComputeIndirect`, `glMemoryBarrier`, `glBindImageTexture` | Forwarded to ES 3.1 core when available |
| **Copy image** | `glCopyImageSubData` | Forwarded to ES 3.2 core when available |
| **Double precision uniforms** | `glUniform1d`–`glUniform4d`, `glUniform1dv`–`glUniform4dv`, `glUniformMatrix2dv`–`glUniformMatrix4x3dv` | Converted from double to float, forwarded to ES float uniform functions |
| **VertexAttribL** | `glVertexAttribL1d`–`glVertexAttribL4dv` | Double→float conversion, forwarded to `glVertexAttrib*f` |
| **Double queries** | `glGetDoublev` | Calls `glGetFloatv` and converts to double |
| **Packed vertex attribs** | `glVertexAttribP1ui`–`glVertexAttribP4uiv` | Unpacked to float components and forwarded to `glVertexAttrib*f` |
| **Per-index toggles** | `glEnablei`, `glDisablei`, `glIsEnabledi`, `glGetBooleani_v` | Falls back to global enable/disable (ES 3.2 has native support for some) |
| **Primitive restart** | `glPrimitiveRestartIndex` | ES 3.2 native; on older ES, fixed index is used silently |
| **Base vertex draw variants** | `glDrawRangeElements`, `glDrawRangeElementsBaseVertex`, `glDrawElementsInstancedBaseVertex` | Forwards to base vertex renderer or loops over instances |
| **Sampler I params** | `glSamplerParameterIiv`, `glSamplerParameterIuiv`, `glGetSamplerParameterIiv`, `glGetSamplerParameterIuiv` | Converted to regular integer sampler parameters |
| **Program uniforms** | `glProgramUniform1f/1i/1fv/1iv/2f/2i/3f/4f/Matrix4fv` | Saves current program, switches to target program, sets uniform, restores |
| **Texture storage EXT** | `glTexStorage1DEXT`, `glTexStorage2DEXT`, `glTexStorage3DEXT` | Forwarded to ES 3.0 core `glTexStorage2D/3D` |
| **Framebuffer texture ARB** | `glFramebufferTextureARB`, `glFramebufferTextureLayerARB` | Aliases for core framebuffer texture functions |
| **Tex buffer range ARB** | `glTexBufferRangeARB` | Forwards to ES 3.2 or EXT tex buffer range |
| **No-op stubs** | `glClampColor`, `glProvokingVertex`, `glPatchParameteri/fv`, `glTextureBarrier`, `glPolygonOffsetClamp`, `glClipControl`, `glBeginConditionalRender`, `glEndConditionalRender` | Accepted and ignored (no ES equivalent exists) |
| **Debug messages** | `glDebugMessageInsert`, `glDebugMessageCallback`, `glGetDebugMessageLog`, `glPushDebugGroup`, `glPopDebugGroup`, `glObjectLabel`, `glGetObjectLabel` | No-op (ES debug output not exposed by default) |
| **ARB instanced draw** | `glDrawArraysInstancedARB`, `glDrawElementsInstancedARB` | Forwarded to ES 3.0 `glDrawArraysInstanced`/`glDrawElementsInstanced` |
| **ARB matrix uniforms** | `glUniformMatrix2x3fvARB`–`glUniformMatrix4x3fvARB` | Forwarded to ES 3.0 matrix uniform functions |
| **Query objects** | `glGenQueries`, `glDeleteQueries`, `glIsQuery`, `glBeginQuery`, `glEndQuery`, `glGetQueryiv`, `glGetQueryObjectuiv` | Forwarded to ES 3.0 query functions |
| **Transform feedback** | `glBeginTransformFeedback`, `glEndTransformFeedback`, `glTransformFeedbackVaryings` | Forwarded to ES 3.0 transform feedback functions |
| **Texture clearing** | `glClearTexImage`, `glClearTexSubImage` | Best-effort emulation with debug logging |
| **Invalidation** | `glInvalidateFramebuffer`, `glInvalidateSubFramebuffer`, `glInvalidateTexImage`, `glInvalidateTexSubImage`, `glInvalidateBufferData`, `glInvalidateBufferSubData` | Framebuffer variants forwarded to ES 3.0; others are no-op |
| **Internal format** | `glGetInternalformativ` | Forwarded to ES 3.0 core |
| **TexParameter I** | `glGetTexParameterIiv`, `glGetTexParameterIuiv` | Forwarded to regular `glGetTexParameteriv` |
