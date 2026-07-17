#include "arbconverter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOK_EOF,
    TOK_NEWLINE,
    TOK_WHITESPACE,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_MINUS,
    TOK_PLUS,
    TOK_DOT,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON
} TokenType;

typedef enum {
    INST_MOV,
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_RCP,
    INST_RSQ,
    INST_SQRT,
    INST_LIT,
    INST_LOG,
    INST_EXP,
    INST_POW,
    INST_MAX,
    INST_MIN,
    INST_SLT,
    INST_SGE,
    INST_SGT,
    INST_SLE,
    INST_EQ,
    INST_NEQ,
    INST_AND,
    INST_OR,
    INST_XOR,
    INST_NOT,
    INST_FRC,
    INST_FLR,
    INST_CEIL,
    INST_TRUNC,
    INST_ROUND,
    INST_SIN,
    INST_COS,
    INST_TAN,
    INST_ASIN,
    INST_ACOS,
    INST_ATAN,
    INST_ATAN2,
    INST_DDX,
    INST_DDY,
    INST_TEX,
    INST_TXP,
    INST_KIL,
    INST_END,
    INST_UNKNOWN
} InstType;

typedef enum {
    VARTYPE_TEMP,
    VARTYPE_INPUT,
    VARTYPE_OUTPUT,
    VARTYPE_CONST,
    VARTYPE_ADDRESS,
    VARTYPE_TEXCOORD,
    VARTYPE_COLOR,
    VARTYPE_FOG,
    VARTYPE_POSITION,
    VARTYPE_WPOS,
    VARTYPE_NONE
} VarType;

typedef struct {
    char name[32];
    VarType type;
    float values[4];
    int array_size;
} Variable;

typedef struct {
    int swizzle[4];
    int negate;
    char var_name[32];
    int array_index;
} Operand;

typedef struct {
    InstType type;
    Operand dest;
    Operand src[3];
    bool saturated;
} Instruction;

typedef struct {
    const char* code;
    const char* pos;
    TokenType token;
    char token_str[256];
    float token_num;
    Variable vars[128];
    int var_count;
    Instruction insts[512];
    int inst_count;
    bool has_fog;
    bool has_depth_replacement;
    bool is_vertex;
} ConverterState;

static void skip_whitespace(ConverterState* state) {
    while (*state->pos && (isspace((unsigned char)*state->pos) || *state->pos == '\r')) {
        state->pos++;
    }
}

static void skip_line(ConverterState* state) {
    while (*state->pos && *state->pos != '\n') {
        state->pos++;
    }
    if (*state->pos == '\n') {
        state->pos++;
    }
}

static void read_token(ConverterState* state) {
    skip_whitespace(state);
    
    if (*state->pos == '\0') {
        state->token = TOK_EOF;
        return;
    }
    
    if (*state->pos == '\n') {
        state->pos++;
        state->token = TOK_NEWLINE;
        return;
    }
    
    if (*state->pos == ';') {
        state->pos++;
        state->token = TOK_SEMICOLON;
        return;
    }
    
    if (*state->pos == ',') {
        state->pos++;
        state->token = TOK_COMMA;
        return;
    }
    
    if (*state->pos == '-') {
        state->pos++;
        state->token = TOK_MINUS;
        return;
    }
    
    if (*state->pos == '+') {
        state->pos++;
        state->token = TOK_PLUS;
        return;
    }
    
    if (*state->pos == '.') {
        state->pos++;
        state->token = TOK_DOT;
        return;
    }
    
    if (*state->pos == '[') {
        state->pos++;
        state->token = TOK_LBRACKET;
        return;
    }
    
    if (*state->pos == ']') {
        state->pos++;
        state->token = TOK_RBRACKET;
        return;
    }
    
    if (*state->pos == '(') {
        state->pos++;
        state->token = TOK_LPAREN;
        return;
    }
    
    if (*state->pos == ')') {
        state->pos++;
        state->token = TOK_RPAREN;
        return;
    }
    
    if (*state->pos == ':') {
        state->pos++;
        state->token = TOK_COLON;
        return;
    }
    
    if (isdigit((unsigned char)*state->pos) || *state->pos == '.') {
        char* end;
        state->token_num = strtof(state->pos, &end);
        state->pos = end;
        state->token = TOK_NUMBER;
        return;
    }
    
    if (isalpha((unsigned char)*state->pos) || *state->pos == '_') {
        int len = 0;
        while (*state->pos && (isalnum((unsigned char)*state->pos) || *state->pos == '_')) {
            if (len < 255) {
                state->token_str[len++] = *state->pos;
            }
            state->pos++;
        }
        state->token_str[len] = '\0';
        state->token = TOK_IDENTIFIER;
        return;
    }
    
    state->pos++;
    read_token(state);
}

static InstType parse_instruction_type(const char* name) {
    if (strcmp(name, "MOV") == 0) return INST_MOV;
    if (strcmp(name, "ADD") == 0) return INST_ADD;
    if (strcmp(name, "SUB") == 0) return INST_SUB;
    if (strcmp(name, "MUL") == 0) return INST_MUL;
    if (strcmp(name, "DIV") == 0) return INST_DIV;
    if (strcmp(name, "RCP") == 0) return INST_RCP;
    if (strcmp(name, "RSQ") == 0) return INST_RSQ;
    if (strcmp(name, "SQRT") == 0) return INST_SQRT;
    if (strcmp(name, "LIT") == 0) return INST_LIT;
    if (strcmp(name, "LOG") == 0) return INST_LOG;
    if (strcmp(name, "EXP") == 0) return INST_EXP;
    if (strcmp(name, "POW") == 0) return INST_POW;
    if (strcmp(name, "MAX") == 0) return INST_MAX;
    if (strcmp(name, "MIN") == 0) return INST_MIN;
    if (strcmp(name, "SLT") == 0) return INST_SLT;
    if (strcmp(name, "SGE") == 0) return INST_SGE;
    if (strcmp(name, "SGT") == 0) return INST_SGT;
    if (strcmp(name, "SLE") == 0) return INST_SLE;
    if (strcmp(name, "EQ") == 0) return INST_EQ;
    if (strcmp(name, "NEQ") == 0) return INST_NEQ;
    if (strcmp(name, "AND") == 0) return INST_AND;
    if (strcmp(name, "OR") == 0) return INST_OR;
    if (strcmp(name, "XOR") == 0) return INST_XOR;
    if (strcmp(name, "NOT") == 0) return INST_NOT;
    if (strcmp(name, "FRC") == 0) return INST_FRC;
    if (strcmp(name, "FLR") == 0) return INST_FLR;
    if (strcmp(name, "CEIL") == 0) return INST_CEIL;
    if (strcmp(name, "TRUNC") == 0) return INST_TRUNC;
    if (strcmp(name, "ROUND") == 0) return INST_ROUND;
    if (strcmp(name, "SIN") == 0) return INST_SIN;
    if (strcmp(name, "COS") == 0) return INST_COS;
    if (strcmp(name, "TAN") == 0) return INST_TAN;
    if (strcmp(name, "ASIN") == 0) return INST_ASIN;
    if (strcmp(name, "ACOS") == 0) return INST_ACOS;
    if (strcmp(name, "ATAN") == 0) return INST_ATAN;
    if (strcmp(name, "ATAN2") == 0) return INST_ATAN2;
    if (strcmp(name, "DDX") == 0) return INST_DDX;
    if (strcmp(name, "DDY") == 0) return INST_DDY;
    if (strcmp(name, "TEX") == 0) return INST_TEX;
    if (strcmp(name, "TXP") == 0) return INST_TXP;
    if (strcmp(name, "KIL") == 0) return INST_KIL;
    if (strcmp(name, "END") == 0) return INST_END;
    return INST_UNKNOWN;
}

static void parse_swizzle(ConverterState* state, int* swizzle) {
    for (int i = 0; i < 4; i++) {
        if (*state->pos == 'x' || *state->pos == 'X') { swizzle[i] = 0; state->pos++; }
        else if (*state->pos == 'y' || *state->pos == 'Y') { swizzle[i] = 1; state->pos++; }
        else if (*state->pos == 'z' || *state->pos == 'Z') { swizzle[i] = 2; state->pos++; }
        else if (*state->pos == 'w' || *state->pos == 'W') { swizzle[i] = 3; state->pos++; }
        else if (*state->pos == '0') { swizzle[i] = -1; state->pos++; }
        else if (*state->pos == '1') { swizzle[i] = -2; state->pos++; }
        else { swizzle[i] = i; break; }
    }
}

static void parse_operand(ConverterState* state, Operand* op) {
    op->negate = 0;
    op->array_index = -1;
    for (int i = 0; i < 4; i++) op->swizzle[i] = i;
    
    if (*state->pos == '-') {
        op->negate = 1;
        state->pos++;
    } else if (*state->pos == '+') {
        state->pos++;
    }
    
    int len = 0;
    while (*state->pos && isalnum((unsigned char)*state->pos)) {
        if (len < 31) op->var_name[len++] = *state->pos;
        state->pos++;
    }
    op->var_name[len] = '\0';
    
    if (*state->pos == '[') {
        state->pos++;
        op->array_index = 0;
        while (*state->pos && isdigit((unsigned char)*state->pos)) {
            op->array_index = op->array_index * 10 + (*state->pos - '0');
            state->pos++;
        }
        if (*state->pos == ']') state->pos++;
    }
    
    if (*state->pos == '.') {
        state->pos++;
        parse_swizzle(state, op->swizzle);
    }
}

static VarType get_var_type(const char* name) {
    if (strncmp(name, "TEMP", 4) == 0) return VARTYPE_TEMP;
    if (strncmp(name, "INPUT", 5) == 0) return VARTYPE_INPUT;
    if (strncmp(name, "OUTPUT", 6) == 0) return VARTYPE_OUTPUT;
    if (strncmp(name, "CONST", 5) == 0) return VARTYPE_CONST;
    if (strncmp(name, "ADDRESS", 7) == 0) return VARTYPE_ADDRESS;
    if (strcmp(name, "fogcoord") == 0) return VARTYPE_FOG;
    if (strcmp(name, "wpos") == 0) return VARTYPE_WPOS;
    if (strcmp(name, "vertex.position") == 0) return VARTYPE_POSITION;
    if (strcmp(name, "vertex.color") == 0) return VARTYPE_COLOR;
    if (strcmp(name, "vertex.texcoord") == 0) return VARTYPE_TEXCOORD;
    if (strcmp(name, "result.position") == 0) return VARTYPE_OUTPUT;
    if (strcmp(name, "result.color") == 0) return VARTYPE_OUTPUT;
    return VARTYPE_NONE;
}

static void generate_swizzle(char* buf, int* swizzle, int negate) {
    const char* swz = "xyzw";
    for (int i = 0; i < 4; i++) {
        if (swizzle[i] == -1) buf[i] = '0';
        else if (swizzle[i] == -2) buf[i] = '1';
        else buf[i] = swz[swizzle[i]];
    }
    buf[4] = '\0';
}

static void append_str(char** output, int* capacity, int* length, const char* str) {
    int len = strlen(str);
    while (*length + len >= *capacity) {
        *capacity *= 2;
        *output = realloc(*output, *capacity);
    }
    strcpy(*output + *length, str);
    *length += len;
}

static void append_swizzled_operand(char** output, int* capacity, int* length, Operand* op, bool negate_result) {
    if (op->negate || negate_result) append_str(output, capacity, length, "-");
    
    if (strcmp(op->var_name, "fogcoord") == 0) {
        append_str(output, capacity, length, "gl_FogFragCoord");
    } else if (strcmp(op->var_name, "wpos") == 0) {
        append_str(output, capacity, length, "gl_FragCoord.w");
    } else if (strcmp(op->var_name, "vertex.position") == 0) {
        append_str(output, capacity, length, "gl_Vertex");
    } else if (strcmp(op->var_name, "vertex.color") == 0) {
        append_str(output, capacity, length, "gl_Color");
    } else if (strcmp(op->var_name, "vertex.texcoord") == 0) {
        append_str(output, capacity, length, "gl_MultiTexCoord0");
    } else if (strcmp(op->var_name, "result.position") == 0) {
        append_str(output, capacity, length, "gl_Position");
    } else if (strcmp(op->var_name, "result.color") == 0) {
        append_str(output, capacity, length, "gl_FragColor");
    } else if (strncmp(op->var_name, "INPUT", 5) == 0) {
        int idx = atoi(op->var_name + 5);
        append_str(output, capacity, length, "gl_MultiTexCoord");
        char num[8];
        sprintf(num, "%d", idx);
        append_str(output, capacity, length, num);
    } else {
        append_str(output, capacity, length, op->var_name);
        if (op->array_index >= 0) {
            append_str(output, capacity, length, "[");
            char num[8];
            sprintf(num, "%d", op->array_index);
            append_str(output, capacity, length, num);
            append_str(output, capacity, length, "]");
        }
    }
    
    if (op->swizzle[1] != 1 || op->swizzle[2] != 2 || op->swizzle[3] != 3 ||
        op->swizzle[0] != 0) {
        append_str(output, capacity, length, ".");
        char swz[5];
        generate_swizzle(swz, op->swizzle, op->negate);
        append_str(output, capacity, length, swz);
    }
}

bool ltw_is_arb_program(const char* code) {
    return strncmp(code, "!!ARBvp1.0", 10) == 0 || strncmp(code, "!!ARBfp1.0", 10) == 0;
}

char* ltw_convert_arb_to_glsl(const char* arb_code, bool is_vertex, char** error_msg, int* error_pos) {
    ConverterState state;
    memset(&state, 0, sizeof(state));
    state.code = arb_code;
    state.pos = arb_code;
    state.is_vertex = is_vertex;
    
    while (*state.pos && strncmp(state.pos, "!!ARB", 5) != 0) {
        state.pos++;
    }
    
    if (*state.pos == '\0') {
        *error_msg = strdup("Invalid ARB program header");
        *error_pos = 0;
        return NULL;
    }
    
    if (is_vertex && strncmp(state.pos, "!!ARBvp1.0", 10) != 0) {
        *error_msg = strdup("Invalid vertex program header");
        *error_pos = (int)(state.pos - state.code);
        return NULL;
    }
    
    if (!is_vertex && strncmp(state.pos, "!!ARBfp1.0", 10) != 0) {
        *error_msg = strdup("Invalid fragment program header");
        *error_pos = (int)(state.pos - state.code);
        return NULL;
    }
    
    state.pos += 10;
    
    while (*state.pos) {
        skip_whitespace(state);
        
        if (*state.pos == '\0') break;
        if (*state.pos == '#') {
            skip_line(state);
            continue;
        }
        
        read_token(&state);
        
        if (state.token == TOK_EOF) break;
        if (state.token == TOK_NEWLINE) continue;
        
        if (state.token == TOK_IDENTIFIER) {
            InstType inst_type = parse_instruction_type(state.token_str);
            
            bool saturated = false;
            read_token(&state);
            if (state.token == TOK_IDENTIFIER && strcmp(state.token_str, "SAT") == 0) {
                saturated = true;
                read_token(&state);
            }
            
            Instruction inst;
            memset(&inst, 0, sizeof(inst));
            inst.type = inst_type;
            inst.saturated = saturated;
            
            if (inst_type != INST_END && inst_type != INST_KIL) {
                parse_operand(&state, &inst.dest);
                
                read_token(&state);
                if (state.token != TOK_SEMICOLON) {
                    parse_operand(&state, &inst.src[0]);
                    
                    if (state.token == TOK_COMMA) {
                        read_token(&state);
                        parse_operand(&state, &inst.src[1]);
                        
                        if (state.token == TOK_COMMA) {
                            read_token(&state);
                            parse_operand(&state, &inst.src[2]);
                        }
                    }
                }
            }
            
            state.insts[state.inst_count++] = inst;
            
            if (inst_type == INST_END) break;
        }
        
        skip_whitespace(state);
        if (*state.pos == ';') state.pos++;
        if (*state.pos == '\n') state.pos++;
    }
    
    int capacity = 8192;
    int length = 0;
    char* output = malloc(capacity);
    
    append_str(&output, &capacity, &length, "#version 120\n\n");
    
    if (is_vertex) {
        append_str(&output, &capacity, &length, "void main() {\n");
    } else {
        append_str(&output, &capacity, &length, "void main() {\n");
    }
    
    for (int i = 0; i < state.inst_count; i++) {
        Instruction* inst = &state.insts[i];
        
        switch (inst->type) {
            case INST_MOV:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ";\n");
                break;
                
            case INST_ADD:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, " + ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                if (inst->saturated) append_str(&output, &capacity, &length, "; // SAT");
                append_str(&output, &capacity, &length, "\n");
                break;
                
            case INST_SUB:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, " - ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                if (inst->saturated) append_str(&output, &capacity, &length, "; // SAT");
                append_str(&output, &capacity, &length, "\n");
                break;
                
            case INST_MUL:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, " * ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                if (inst->saturated) append_str(&output, &capacity, &length, "; // SAT");
                append_str(&output, &capacity, &length, "\n");
                break;
                
            case INST_DIV:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, " / ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                if (inst->saturated) append_str(&output, &capacity, &length, "; // SAT");
                append_str(&output, &capacity, &length, "\n");
                break;
                
            case INST_RCP:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = 1.0 / ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ";\n");
                break;
                
            case INST_RSQ:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = inversesqrt(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_SQRT:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = sqrt(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_LIT:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = lit(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_LOG:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = log2(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_EXP:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = exp2(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_POW:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = pow(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ", ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_MAX:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = max(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ", ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_MIN:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = min(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ", ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_SLT:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = step(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                append_str(&output, &capacity, &length, ", ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_SGE:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = step(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ", ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[1], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_TEX:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = texture2D(gl_Texture[0], ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_TXP:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = texture2DProj(gl_Texture[0], ");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_KIL:
                append_str(&output, &capacity, &length, "\tif (" );
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ".x < 0.0) discard;\n");
                break;
                
            case INST_END:
                break;
                
            case INST_FRC:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = fract(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_FLR:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = floor(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_CEIL:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = ceil(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_SIN:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = sin(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_COS:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = cos(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_DDX:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = dFdx(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            case INST_DDY:
                append_str(&output, &capacity, &length, "\t");
                append_swizzled_operand(&output, &capacity, &length, &inst->dest, false);
                append_str(&output, &capacity, &length, " = dFdy(");
                append_swizzled_operand(&output, &capacity, &length, &inst->src[0], false);
                append_str(&output, &capacity, &length, ");\n");
                break;
                
            default:
                *error_msg = strdup("Unsupported instruction");
                *error_pos = i;
                free(output);
                return NULL;
        }
    }
    
    append_str(&output, &capacity, &length, "}\n");
    
    return output;
}