#ifndef LTW_GLSL_FOR_ES_H
#define LTW_GLSL_FOR_ES_H

#include <stdbool.h>

char* ltw_convert_glsl_for_es(const char* glsl, GLenum shader_type, int target_es_version);
char* ltw_replace_version_line(const char* text, const char* new_version);
char* ltw_replace_glfragcolor(const char* src);
int ltw_contains_glfragcolor(const char* src);
char* ltw_insert_precision_declarations(const char* shader);
char* ltw_convert_texture_calls(const char* glsl);
char* ltw_add_fragcolor_declaration(const char* shader);

#endif // LTW_GLSL_FOR_ES_H