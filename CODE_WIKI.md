# LTW (shit renderer) Code Wiki

## 1. 项目概述

LTW (Lightweight Tiny Wrapper) 是一个**桌面 OpenGL Core 到 OpenGL ES 的薄包装器**，主要用于在 Android 设备上运行 Minecraft: Java Edition。项目将桌面 OpenGL 3.3+ 的 API 调用转换为 OpenGL ES 3.0+ 的调用，并模拟了 Minecraft 所需的多个 ARB 扩展。

**核心目标**:
- 在 Android 设备上提供桌面级 OpenGL 兼容性
- 支持 Minecraft 1.17+ 的渲染需求
- 高性能的着色器编译和缓存
- 多 ABI 架构支持

---

## 2. 项目架构

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                       应用层 (Minecraft)                            │
│                    OpenGL 3.3 Core + ARB Extensions                 │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        LTW 包装层                                   │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────────────────┐ │
│  │   EGL 层     │ │   GL 函数层   │ │       着色器系统           │ │
│  │  egl.c/h     │ │  main.c      │ │  shader_wrapper.c          │ │
│  │  上下文管理   │ │  GL API包装  │ │  glsl_optimizer (子模块)   │ │
│  └──────────────┘ └──────────────┘ └─────────────────────────────┘ │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────────────────┐ │
│  │  扩展支持    │ │  格式转换    │ │       性能优化             │ │
│  │  proc.c      │ │  glformats.c │ │  mempool.c, swizzle.c      │ │
│  │  basevertex.c│ │              │ │  fast_gl 缓存              │ │
│  │  blending.c  │ │              │ │                             │ │
│  │  query.c     │ │              │ │                             │ │
│  │  multidraw.c │ │              │ │                             │ │
│  └──────────────┘ └──────────────┘ └─────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    系统层 (Android)                                  │
│               OpenGL ES 3.0/3.1/3.2 + EGL                          │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 目录结构

| 目录 | 职责 |
|------|------|
| `ltw/src/main/tinywrapper/` | 核心包装器源代码 |
| `ltw/src/main/tinywrapper/glsl_optimizer/` | GLSL 编译器/优化器 (子模块) |
| `ltw/src/main/tinywrapper/unordered_map/` | 轻量级哈希表实现 |
| `ltw/src/main/tinywrapper/vgpu_shaderconv/` | 着色器转换工具 |
| `ltw/src/main/tinywrapper/GL/` | 桌面 GL 头文件 |
| `.github/workflows/` | CI 工作流配置 |

---

## 3. 核心模块详解

### 3.1 EGL 上下文管理 (`egl.c/h`)

**职责**: 管理 EGL 上下文的创建、销毁和切换，维护渲染状态。

#### 关键数据结构

```c
typedef struct {
    EGLContext phys_context;           // 物理 EGL 上下文
    bool context_rdy;                  // 上下文是否就绪
    bool es31, es32;                   // ES 版本标志
    bool buffer_storage;               // EXT_buffer_storage 支持
    bool buffer_texture_ext;           // EXT_texture_buffer 支持
    bool multidraw_indirect;           // EXT_multi_draw_indirect 支持
    bool timer_query;                  // EXT_disjoint_timer_query 支持
    GLint shader_version;              // 着色器版本
    basevertex_renderer_t basevertex;  // 基础顶点渲染器
    blending_functions_t blending;     // 混合函数
    unordered_map* shader_map;         // 着色器映射表
    unordered_map* program_map;        // 程序映射表
    unordered_map* framebuffer_map;    // 帧缓冲映射表
    mempool_t* shader_info_pool;       // 着色器信息内存池
    mempool_t* program_info_pool;      // 程序信息内存池
    format_cache_entry_t format_cache[FORMAT_CACHE_SIZE];  // 格式缓存
    struct {
        void (*glDrawArrays)(...);
        void (*glDrawElements)(...);
        // ... 其他热路径函数
    } fast_gl;                         // 热路径函数指针缓存
} context_t;
```

#### 关键函数

| 函数 | 说明 |
|------|------|
| `eglCreateContext()` | 创建 EGL 上下文并初始化 LTW 状态 |
| `eglDestroyContext()` | 销毁上下文并释放资源 |
| `eglMakeCurrent()` | 切换当前上下文 |
| `build_extension_string()` | 构建扩展字符串，决定暴露哪些 ARB 扩展 |
| `find_esversion()` | 检测设备 ES 版本和可用扩展 |
| `init_incontext()` | 初始化上下文内部状态 |

#### 扩展暴露机制

LTW 通过 `build_extension_string()` 动态构建扩展字符串：

- **始终暴露**: `GL_ARB_buffer_storage`, `GL_ARB_texture_buffer_object`, `GL_ARB_draw_elements_base_vertex`, `GL_ARB_draw_buffers_blend`, `GL_ARB_timer_query`, `GL_ARB_texture_storage`, `GL_ARB_shading_language_packing`
- **ES 3.1+ 暴露**: `GL_ARB_draw_indirect`, `GL_ARB_shader_storage_buffer_object`, `GL_ARB_compute_shader` 等
- **条件暴露**: 根据函数指针解析结果暴露 `GL_ARB_framebuffer_no_attachments`, `GL_ARB_vertex_attrib_binding`

---

### 3.2 GL 函数包装 (`main.c`)

**职责**: 实现桌面 OpenGL 核心函数，将其转换为 OpenGL ES 调用。

#### 核心实现模式

```c
void glFunctionName(GLenum param1, GLtype param2, ...) {
    if(!current_context) return;           // 上下文检查
    // 参数转换/处理逻辑
    es3_functions.glFunctionName(param1, param2, ...);  // 转发到 ES
}
```

#### 关键函数实现

| 函数 | 转换逻辑 |
|------|----------|
| `glMapBuffer()` | 转换为 `glMapBufferRange()` |
| `glClearDepth()` | 转换为 `glClearDepthf()` (float 版本) |
| `glDepthRange()` | 转换为 `glDepthRangef()` |
| `glBufferStorage()` | 使用 `glBufferStorageEXT`，处理 `LTW_NEVER_FLUSH_BUFFERS` |
| `glGetString()` | 伪造 `GL_VERSION`, `GL_VENDOR`, `GL_SHADING_LANGUAGE_VERSION` |
| `glGetIntegerv()` | 伪造 `GL_MAJOR_VERSION`, `GL_MINOR_VERSION`, `GL_CONTEXT_PROFILE_MASK` |

#### 兼容性模式

通过环境变量 `LTW_GL_VERSION=3.2` 启用：
- 报告 `GL_VERSION` 为 `3.2` 而非 `3.3`
- 报告 `GL_SHADING_LANGUAGE_VERSION` 为 `1.50` 而非 `4.60`
- 隐藏 ES 3.1/3.2 专属扩展

这用于兼容 Minecraft 1.17 及更早版本。

---

### 3.3 函数指针解析 (`proc.c`)

**职责**: 加载系统 EGL 库，解析 OpenGL ES 函数指针。

#### 初始化流程

```c
__attribute__((constructor)) void proc_init() {
    // 1. 加载 libEGL.so
    void* eglHandle = dlopen("libEGL.so", RTLD_LAZY | RTLD_LOCAL);
    
    // 2. 获取 eglGetProcAddress
    host_eglGetProcAddress = dlsym(eglHandle, "eglGetProcAddress");
    
    // 3. 初始化 EGL 层
    init_egl();
    
    // 4. 初始化 ES 函数指针
    init_es3_proc();
}
```

#### 函数重定向机制

`eglGetProcAddress()` 实现了函数重定向：

1. **EGL 函数**: 检查是否为 LTW 实现的 EGL 函数 (`eglCreateContext`, `eglDestroyContext`, `eglMakeCurrent`)
2. **GL 覆盖函数**: 检查 `es3_overrides.h` 中定义的覆盖函数
3. **系统函数**: 转发到 `host_eglGetProcAddress()`
4. **Stub 函数**: 如果系统也找不到，返回 stub 实现

---

### 3.4 着色器系统 (`shader_wrapper.c`)

**职责**: 管理着色器编译、优化和缓存，处理桌面 GLSL 到 ESSL 的转换。

#### 关键数据结构

```c
typedef struct {
    GLenum shader_type;        // 着色器类型
    GLchar* source;            // 优化后的源代码
} shader_info_t;

typedef struct {
    GLuint frag_shader;        // 片段着色器
    GLchar* colorbindings[MAX_DRAWBUFFERS];  // 颜色绑定
} program_info_t;
```

#### 着色器优化流程

1. **接收源码**: `glShaderSource()` 接收桌面 GLSL 源代码
2. **缓存查找**: 计算哈希值，检查着色器缓存
3. **优化编译**: 调用 `optimize_shader()` (基于 glsl_optimizer)
4. **缓存存储**: 将优化结果存入 LRU 缓存
5. **片段输出修补**: `glLinkProgram()` 时插入 `layout(location = N)` 修饰符

#### 缓存机制

- **大小**: 256 个条目，最大 32MB
- **策略**: LRU (最近最少使用) + 访问计数
- **统计**: 定期输出命中率统计

---

### 3.5 格式转换 (`glformats.c`)

**职责**: 将桌面 OpenGL 的纹理/渲染缓冲格式转换为 OpenGL ES 支持的格式。

#### 转换策略

| 桌面格式 | ES 格式 | 说明 |
|----------|---------|------|
| `GL_DEPTH_COMPONENT` | `GL_DEPTH_COMPONENT32F` | 使用 32 位浮点避免 z-fighting |
| `GL_RGB16F`, `GL_RGB32F` | `GL_R11F_G11F_B10F` | ES 唯一支持的 3 通道浮点格式 |
| `GL_RGB8I`, `GL_RGB16I` | `GL_R11F_G11F_B10F` | 整数格式转换为浮点 |
| `GL_RGB8_SNORM` | `GL_R16F` | SNORM 格式不支持渲染 |
| `GL_RGBA12`, `GL_RGBA16` | `GL_RGBA16F` | 遗留格式转换 |

#### 整数格式处理

ES 需要明确的整数格式声明：

```c
// 桌面 GL: GL_RGBA8UI + GL_RGBA + GL_UNSIGNED_BYTE
// ES:      GL_RGBA8UI + GL_RGBA_INTEGER + GL_UNSIGNED_BYTE
case GL_RGBA8UI:
    *format = GL_RGBA_INTEGER;
    break;
```

---

### 3.6 扩展实现模块

#### 3.6.1 基础顶点绘制 (`basevertex.c`)

**职责**: 实现 `glDrawElementsBaseVertex` 和 `glMultiDrawElementsBaseVertex`。

| 实现方式 | 条件 |
|----------|------|
| 原生实现 | ES 3.2 或 `GL_OES/EXT_draw_elements_base_vertex` |
| 间接绘制模拟 | ES 3.1 + `GL_EXT_multi_draw_indirect` |
| 回退循环 | 不支持上述条件时 |

#### 3.6.2 混合函数 (`blending.c`)

**职责**: 实现每缓冲混合控制 (`glBlendEquationi`, `glBlendFunci` 等)。

使用宏定义生成 ARB 和非 ARB 版本：

```c
#define GL_BLEND_FUNC(name, func, args, ...) \
void name args { \
    if(!current_context || !current_context->blending.available) return; \
    current_context->blending.func(__VA_ARGS__); \
}
```

#### 3.6.3 查询函数 (`query.c`)

**职责**: 实现 GPU 计时器查询 (`glQueryCounter`, `glGetQueryObjecti64v`)。

- 支持 `GL_EXT_disjoint_timer_query` 扩展
- 无扩展时返回模拟值（环境变量控制）

#### 3.6.4 多重绘制 (`multidraw.c`)

**职责**: 实现 `glMultiDrawArrays` 和 `glMultiDrawElements`。

- `glMultiDrawArrays`: 使用 `glMultiDrawArraysEXT` 或循环
- `glMultiDrawElements`: 合并索引到临时缓冲区，单次绘制

---

### 3.7 性能优化模块

#### 3.7.1 内存池 (`mempool.c/h`)

**职责**: 高效管理频繁分配/释放的小对象（着色器信息、程序信息等）。

```c
typedef struct mempool {
    size_t object_size;       // 对象大小（对齐后）
    size_t chunk_size;        // 每个 chunk 的对象数
    mempool_block_t* free_list;  // 空闲链表
    void* chunks;             // chunk 链表头
    size_t used_count;        // 使用中的对象数
    size_t free_count;        // 空闲对象数
} mempool_t;
```

**特性**:
- 预分配 chunk，减少 `malloc/free` 调用
- 空闲链表快速分配/释放
- 自动扩展（当空闲链表为空时）

#### 3.7.2 纹理 Swizzle (`swizzle.c`)

**职责**: 处理纹理上传时的字节顺序和通道顺序转换。

**支持的转换**:
- BGRA → RGBA (通道交换)
- 反转字节序（处理某些驱动的特殊格式）

#### 3.7.3 热路径缓存 (`fast_gl`)

**职责**: 在 `context_t` 中缓存常用 GL 函数指针，减少虚函数调用开销。

```c
struct {
    void (*glDrawArrays)(GLenum, GLint, GLsizei);
    void (*glDrawElements)(GLenum, GLsizei, GLenum, const void*);
    void (*glBindBuffer)(GLenum, GLuint);
    void (*glBindTexture)(GLenum, GLuint);
    void* (*glMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    // ...
} fast_gl;
```

---

### 3.8 辅助模块

#### 3.8.1 环境变量 (`env.c/h`)

提供环境变量读取和设备检测：

| 函数 | 说明 |
|------|------|
| `env_istrue()` | 检查环境变量是否为 "true"/"1" |
| `env_istrue_d()` | 带默认值的版本 |
| `detect_device_memory_mb()` | 检测设备内存大小（用于调整缓冲区大小） |

#### 3.8.2 调试宏 (`debug.h`)

```c
#define LTW_DEBUG_PRINTF(fmt, ...) do { if(debug) printf("[LTW DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LTW_ERROR_PRINTF(fmt, ...) printf("[LTW ERROR] " fmt "\n", ##__VA_ARGS__)
```

---

## 4. 环境变量参考

| 变量 | 默认值 | 效果 |
|------|--------|------|
| `LTW_GL_VERSION` | unset | 设置为 `3.2` 启用兼容性模式 |
| `LIBGL_NOERROR` | unset | 设置为 `1` 时 `glGetError()` 始终返回 0 |
| `LTW_DEBUG` | unset | 设置为 `1` 启用调试模式和额外日志 |
| `LTW_NEVER_FLUSH_BUFFERS` | `1` | 禁止显式缓冲区刷新 (`glFlushMappedBufferRange`) |
| `LTW_COHERENT_DYNAMIC_STORAGE` | `1` | 强制动态存储缓冲区为一致性模式 |
| `LTW_HIDE_BUFFER_STORAGE` | unset | 设置为 `1` 隐藏 `GL_ARB_buffer_storage` 扩展 |
| `LTW_ENABLE_TIMER_QUERY` | unset | 设置为 `1` 强制启用计时器查询（模拟） |
| `LIBGL_EGL` | unset | 指定自定义 EGL 库路径 |

---

## 5. 构建系统

### 5.1 构建配置

**Android.mk** 定义两个模块：
1. `glsl_optimizer`: 静态库，包含 Mesa 的 GLSL 编译器
2. `sr`: 共享库 (`libsr.so`)，包含 LTW 核心代码

**Application.mk** 配置：
```makefile
APP_PLATFORM := android-21
APP_STL := c++_static
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
```

### 5.2 构建命令

```bash
# Unix / macOS / Git Bash
./gradlew :ltw:assembleRelease

# Windows
gradlew.bat :ltw:assembleRelease
```

### 5.3 输出产物

| 文件 | 路径 | 说明 |
|------|------|------|
| AAR | `ltw/build/outputs/aar/ltw-release.aar` | 包含所有 ABI 的 Android 库 |
| .so 文件 | `ltw/build/outputs/native-libs/<abi>/libsr.so` | 独立的原生库 |

### 5.4 CI 流程

[.github/workflows/ci.yml](.github/workflows/ci.yml) 执行：
1. 设置 JDK 17、Android SDK、NDK 28.2.13676358
2. 构建 Release AAR
3. 验证所有 ABI 的 `.so` 文件存在
4. 上传 AAR 作为工作流产物
5. 推送 `v*` 标签时发布 GitHub Release

---

## 6. 依赖关系

### 6.1 内部依赖

```
egl.c/h          ──→ proc.h, unordered_map.h, shader_wrapper.h, mempool.h
main.c           ──→ egl.h, proc.h, glformats.h, swizzle.h, mempool.h
proc.c           ──→ egl.h, es3_functions.h, es3_extended.h, es3_overrides.h
shader_wrapper.c ──→ egl.h, proc.h, glsl_optimizer/c_wrapper.h, mempool.h
glformats.c      ──→ egl.h, glformats.h
basevertex.c     ──→ egl.h, proc.h
blending.c       ──→ egl.h, proc.h
query.c          ──→ egl.h, proc.h
multidraw.c      ──→ egl.h, proc.h, basevertex.h
swizzle.c        ──→ egl.h, proc.h
mempool.c        ──→ mempool.h, debug.h
env.c            ──→ env.h
```

### 6.2 外部依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| JDK | 17 | 构建工具链 |
| Android SDK | Platform 34 | 编译目标 |
| Android NDK | 28.2.13676358 | 原生代码编译 |
| Gradle | 7.5 | 构建系统 |
| AGP | 7.4.1 | Android Gradle 插件 |
| glsl_optimizer | Mesa 衍生 | GLSL → ESSL 编译器 |

---

## 7. 关键设计模式

### 7.1 函数包装模式

所有 GL 函数都遵循统一的包装模式：
1. 检查 `current_context` 是否有效
2. 参数转换/验证
3. 转发到实际的 ES 函数
4. 维护 LTW 内部状态

### 7.2 延迟初始化

上下文的复杂初始化 (`init_incontext`) 在首次 `eglMakeCurrent` 时执行，而非创建时：
- 避免不必要的初始化开销
- 确保在正确的 GL 环境下检测扩展

### 7.3 条件暴露

扩展仅在满足条件时暴露：
- 设备支持的 ES 版本
- 函数指针成功解析
- 环境变量配置

### 7.4 内存池优化

高频对象（着色器、程序）使用内存池分配：
- 减少内存碎片
- 提高分配/释放性能
- 统一生命周期管理

---

## 8. 扩展支持矩阵

### 8.1 ARB 扩展 → ES 实现映射

| ARB 扩展 | ES 实现 | 要求 |
|----------|---------|------|
| `GL_ARB_buffer_storage` | `GL_EXT_buffer_storage` | 设备支持该扩展 |
| `GL_ARB_texture_buffer_object` | `GL_EXT_texture_buffer` 或 ES 3.2 | 扩展或 ES 3.2 |
| `GL_ARB_draw_elements_base_vertex` | `GL_OES/EXT_draw_elements_base_vertex` 或 ES 3.2 | 扩展或 ES 3.2 |
| `GL_ARB_draw_buffers_blend` | `GL_OES/EXT_draw_buffers_indexed` 或 ES 3.2 | 扩展或 ES 3.2 |
| `GL_ARB_timer_query` | `GL_EXT_disjoint_timer_query` | 扩展或环境变量强制 |
| `GL_ARB_draw_indirect` | ES 3.1 `glDrawArraysIndirect` | ES 3.1 |
| `GL_ARB_shader_storage_buffer_object` | ES 3.1 SSBO | ES 3.1 |
| `GL_ARB_compute_shader` | ES 3.1 计算着色器 | ES 3.1 |
| `GL_ARB_copy_image` | ES 3.2 `glCopyImageSubData` | ES 3.2 |

---

## 9. 运行时要求

### 9.1 最低要求

| 项目 | 最低版本 |
|------|----------|
| OpenGL ES | 3.0 |
| ESSL | 3.00 |
| Android API | 21 (Android 5.0) |

### 9.2 推荐配置

| 项目 | 推荐版本 |
|------|----------|
| OpenGL ES | 3.2 |
| Android API | 30+ |
| 设备内存 | 4GB+ |

---

## 10. 调试和诊断

### 10.1 启用调试模式

```bash
export LTW_DEBUG=1
```

启用后会输出详细的调试日志，包括：
- 着色器缓存统计
- 格式转换信息
- 扩展检测结果

### 10.2 常见问题排查

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| 片段着色器链接失败 | 缺少 `layout(location)` | 检查 `glBindFragDataLocation` 调用 |
| 纹理显示异常 | 格式不兼容 | 检查 `pick_internalformat` 转换 |
| 性能低下 | 频繁内存分配 | 检查内存池使用 |
| 扩展未检测到 | 函数指针解析失败 | 检查 `find_esversion` 日志 |

---

## 附录：文件索引

| 文件 | 路径 | 行数 |
|------|------|------|
| egl.h | [egl.h](ltw/src/main/tinywrapper/egl.h) | ~144 |
| egl.c | [egl.c](ltw/src/main/tinywrapper/egl.c) | ~434 |
| main.c | [main.c](ltw/src/main/tinywrapper/main.c) | ~546 |
| proc.c | [proc.c](ltw/src/main/tinywrapper/proc.c) | ~88 |
| shader_wrapper.c | [shader_wrapper.c](ltw/src/main/tinywrapper/shader_wrapper.c) | ~350 |
| glformats.c | [glformats.c](ltw/src/main/tinywrapper/glformats.c) | ~325 |
| basevertex.c | [basevertex.c](ltw/src/main/tinywrapper/basevertex.c) | ~130 |
| blending.c | [blending.c](ltw/src/main/tinywrapper/blending.c) | ~25 |
| query.c | [query.c](ltw/src/main/tinywrapper/query.c) | ~44 |
| multidraw.c | [multidraw.c](ltw/src/main/tinywrapper/multidraw.c) | ~52 |
| swizzle.c | [swizzle.c](ltw/src/main/tinywrapper/swizzle.c) | ~121 |
| mempool.c | [mempool.c](ltw/src/main/tinywrapper/mempool.c) | ~113 |
| mempool.h | [mempool.h](ltw/src/main/tinywrapper/mempool.h) | ~31 |
| env.h | [env.h](ltw/src/main/tinywrapper/env.h) | ~15 |
| debug.h | [debug.h](ltw/src/main/tinywrapper/debug.h) | ~12 |
| Android.mk | [Android.mk](ltw/src/main/tinywrapper/Android.mk) | ~419 |
| Application.mk | [Application.mk](ltw/src/main/tinywrapper/Application.mk) | ~4 |
| README.md | [README.md](README.md) | ~154 |
