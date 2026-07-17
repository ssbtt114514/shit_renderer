#include "glsl_for_es.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char* replace_all(const char* src, const char* old, const char* repl) {
    if (!src || !old || !repl) return NULL;
    size_t src_len = strlen(src);
    size_t old_len = strlen(old);
    if (old_len == 0) return NULL;
    size_t repl_len = strlen(repl);
    
    size_t count = 0;
    const char* p = src;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }
    
    if (count == 0) {
        char* out = (char*)malloc(src_len + 1);
        if (out) memcpy(out, src, src_len + 1);
        return out;
    }
    
    size_t new_len = src_len + count * (repl_len - old_len);
    char* out = (char*)malloc(new_len + 1);
    if (!out) return NULL;
    
    const char* s = src;
    char* d = out;
    while ((p = strstr(s, old)) != NULL) {
        size_t len = p - s;
        memcpy(d, s, len);
        d += len;
        memcpy(d, repl, repl_len);
        d += repl_len;
        s = p + old_len;
    }
    strcpy(d, s);
    return out;
}

char* ltw_replace_version_line(const char* text, const char* new_version) {
    if (!text || !new_version) return NULL;
    
    const char* p = text;
    const char* line_start = text;
    size_t text_len = strlen(text);
    size_t new_version_len = strlen(new_version);
    
    while (*p) {
        const char* line_end = strchr(p, '\n');
        const char* next_line = NULL;
        
        if (line_end) {
            next_line = line_end + 1;
        } else {
            line_end = text + text_len;
            next_line = line_end;
        }
        
        const char* cur = p;
        while (cur < line_end && isspace((unsigned char)*cur) && *cur != '\n' && *cur != '\r')
            cur++;
        
        const char* tok = "#version";
        size_t toklen = strlen(tok);
        if ((size_t)(line_end - cur) >= toklen && strncmp(cur, tok, toklen) == 0) {
            const char next_char = (cur + toklen < line_end) ? *(cur + toklen) : '\0';
            if (next_char == '\0' || isspace((unsigned char)next_char)) {
                size_t prefix_len = p - text;
                const char* suffix_start = next_line;
                size_t suffix_len = strlen(suffix_start);
                size_t new_total = prefix_len + new_version_len + suffix_len;
                char* out = (char*)malloc(new_total + 1);
                if (!out) return NULL;
                memcpy(out, text, prefix_len);
                memcpy(out + prefix_len, new_version, new_version_len);
                memcpy(out + prefix_len + new_version_len, suffix_start, suffix_len);
                out[new_total] = '\0';
                return out;
            }
        }
        p = next_line;
    }
    
    return strdup(text);
}

int ltw_contains_glfragcolor(const char* src) {
    if (!src) return 0;
    const char* target = "gl_FragColor";
    size_t target_len = strlen(target);
    const char* p = src;
    
    while ((p = strstr(p, target)) != NULL) {
        int valid = 1;
        if (p != src && (isalnum((unsigned char)p[-1]) || p[-1] == '_')) {
            valid = 0;
        }
        if (p[target_len] != '\0' && (isalnum((unsigned char)p[target_len]) || p[target_len] == '_')) {
            valid = 0;
        }
        if (valid) return 1;
        p += target_len;
    }
    return 0;
}

char* ltw_replace_glfragcolor(const char* src) {
    if (!src) return NULL;
    const char* target = "gl_FragColor";
    const char* replacement = "gl4es_FragColor";
    size_t target_len = strlen(target);
    size_t replacement_len = strlen(replacement);
    
    size_t count = 0;
    const char* p = src;
    while ((p = strstr(p, target)) != NULL) {
        if ((p == src || !((p[-1] >= 'a' && p[-1] <= 'z') || (p[-1] >= 'A' && p[-1] <= 'Z') ||
                           (p[-1] >= '0' && p[-1] <= '9') || p[-1] == '_')) &&
            !((p[target_len] >= 'a' && p[target_len] <= 'z') || (p[target_len] >= 'A' && p[target_len] <= 'Z') ||
              (p[target_len] >= '0' && p[target_len] <= '9') || p[target_len] == '_')) {
            count++;
        }
        p += target_len;
    }
    
    size_t new_len = strlen(src) + count * (replacement_len - target_len) + 1;
    char* result = (char*)malloc(new_len);
    if (!result) return NULL;
    
    const char* src_p = src;
    char* dst_p = result;
    while ((p = strstr(src_p, target)) != NULL) {
        int valid =
            (p == src_p || !((p[-1] >= 'a' && p[-1] <= 'z') || (p[-1] >= 'A' && p[-1] <= 'Z') ||
                             (p[-1] >= '0' && p[-1] <= '9') || p[-1] == '_')) &&
            !((p[target_len] >= 'a' && p[target_len] <= 'z') || (p[target_len] >= 'A' && p[target_len] <= 'Z') ||
              (p[target_len] >= '0' && p[target_len] <= '9') || p[target_len] == '_');
        
        size_t copy_len = p - src_p;
        memcpy(dst_p, src_p, copy_len);
        dst_p += copy_len;
        
        if (valid) {
            memcpy(dst_p, replacement, replacement_len);
            dst_p += replacement_len;
        } else {
            memcpy(dst_p, target, target_len);
            dst_p += target_len;
        }
        src_p = p + target_len;
    }
    strcpy(dst_p, src_p);
    return result;
}

char* ltw_insert_precision_declarations(const char* shader) {
    if (!shader) return NULL;
    
    const char* precision_lines = 
        "precision highp float;\n"
        "precision highp int;\n"
        "precision mediump sampler2D;\n"
        "precision mediump sampler3D;\n"
        "precision mediump samplerCube;\n";
    
    const char* p = shader;
    while (*p && isspace((unsigned char)*p)) p++;
    
    if (strncmp(p, "#version", 8) == 0) {
        const char* newline = strchr(p, '\n');
        if (newline) {
            size_t before_len = newline - shader + 1;
            size_t after_len = strlen(newline + 1);
            size_t total_len = before_len + strlen(precision_lines) + after_len;
            char* result = (char*)malloc(total_len + 1);
            if (!result) return NULL;
            
            memcpy(result, shader, before_len);
            memcpy(result + before_len, precision_lines, strlen(precision_lines));
            memcpy(result + before_len + strlen(precision_lines), newline + 1, after_len);
            result[total_len] = '\0';
            return result;
        }
    }
    
    size_t total_len = strlen(precision_lines) + strlen(shader);
    char* result = (char*)malloc(total_len + 1);
    if (!result) return NULL;
    
    strcpy(result, precision_lines);
    strcpy(result + strlen(precision_lines), shader);
    return result;
}

char* ltw_convert_texture_calls(const char* glsl) {
    if (!glsl) return NULL;
    
    char* result = strdup(glsl);
    if (!result) return NULL;
    
    char* temp;
    
    temp = replace_all(result, "texture2D(", "texture(");
    free(result);
    result = temp;
    
    temp = replace_all(result, "textureCube(", "texture(");
    free(result);
    result = temp;
    
    temp = replace_all(result, "texture3D(", "texture(");
    free(result);
    result = temp;
    
    temp = replace_all(result, "texture2DProj(", "textureProj(");
    free(result);
    result = temp;
    
    temp = replace_all(result, "textureCubeProj(", "textureProj(");
    free(result);
    result = temp;
    
    temp = replace_all(result, "textureRect(", "texture(");
    free(result);
    result = temp;
    
    return result;
}

char* ltw_add_fragcolor_declaration(const char* shader) {
    if (!shader) return NULL;
    if (!ltw_contains_glfragcolor(shader)) {
        return strdup(shader);
    }
    
    const char* insert_line = "out vec4 gl4es_FragColor;\n";
    
    const char* pos = shader;
    const char* insert_pos = shader;
    
    if (strncmp(shader, "#version", 8) == 0) {
        const char* newline = strchr(shader, '\n');
        if (newline) {
            insert_pos = newline + 1;
        }
    }
    
    size_t new_len = strlen(shader) + strlen(insert_line) + 1;
    char* new_shader = (char*)malloc(new_len);
    if (!new_shader) return NULL;
    
    size_t head_len = insert_pos - shader;
    memcpy(new_shader, shader, head_len);
    strcpy(new_shader + head_len, insert_line);
    strcpy(new_shader + head_len + strlen(insert_line), insert_pos);
    
    return new_shader;
}

char* ltw_convert_glsl_for_es(const char* glsl, GLenum shader_type, int target_es_version) {
    if (!glsl) return NULL;
    
    char* result = strdup(glsl);
    if (!result) return NULL;
    
    char* temp;
    
    if (target_es_version <= 20) {
        temp = ltw_replace_version_line(result, "#version 100\n");
        free(result);
        result = temp;
        
        if (shader_type == GL_FRAGMENT_SHADER) {
            temp = ltw_insert_precision_declarations(result);
            free(result);
            result = temp;
            
            temp = ltw_replace_glfragcolor(result);
            free(result);
            result = temp;
            
            temp = ltw_add_fragcolor_declaration(result);
            free(result);
            result = temp;
        }
    } else if (target_es_version <= 30) {
        temp = ltw_replace_version_line(result, "#version 300 es\n");
        free(result);
        result = temp;
        
        if (shader_type == GL_FRAGMENT_SHADER) {
            temp = ltw_insert_precision_declarations(result);
            free(result);
            result = temp;
        }
    }
    
    temp = ltw_convert_texture_calls(result);
    free(result);
    result = temp;
    
    temp = replace_all(result, "gl_ClipDistance", "gl_ClipDistance[0]");
    free(result);
    result = temp;
    
    return result;
}