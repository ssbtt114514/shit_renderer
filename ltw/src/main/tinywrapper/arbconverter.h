#ifndef LTW_ARBCONVERTER_H
#define LTW_ARBCONVERTER_H

#include <stdbool.h>

char* ltw_convert_arb_to_glsl(const char* arb_code, bool is_vertex, char** error_msg, int* error_pos);
bool ltw_is_arb_program(const char* code);

#endif // LTW_ARBCONVERTER_H