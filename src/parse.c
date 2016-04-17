//
// Created by c on 4/16/16.
//

#include <assert.h>
#include <ctype.h>

#include "error.h"
#include "parse.h"
#include "parse_internal.h"

#define parse_error(state, fmt, ...)                                \
                error_f(                                            \
                    fmt "\n  at %s:%d %d\n",                        \
                    __VA_ARGS__,                                    \
                    state->filename,                                \
                    state->line,                                    \
                    state->column                                   \
                )

#define parse_warn(state, fmt, ...)                                 \
                warn_f(                                             \
                    fmt "\n  at %s:%d %d\n",                        \
                    __VA_ARGS__,                                    \
                    state->filename,                                \
                    state->line,                                    \
                    state->column                                   \
                )

#define parse_expect_error(state, expect, tok)                                      \
        switch (tok) {                                                              \
            case TOK_EOF:                                                           \
                parse_error(state, expect " expected, but %s",                      \
                            "EOF"                                                   \
                );                                                                  \
                break;                                                              \
            case TOK_NUM:                                                           \
                parse_error(state, expect " expected, but number %d met",           \
                            state->peaking_value                                    \
                );                                                                  \
                break;                                                              \
            case TOK_ID:                                                            \
            {                                                                       \
                CString *literal = (CString*)(state->peaking_value);                \
                parse_error(state, expect " expected, but identifier '%.*s' met",   \
                            literal->length,                                        \
                            literal->content                                        \
                );                                                                  \
                break;                                                              \
            }                                                                       \
            case TOK_STRING:                                                        \
            {                                                                       \
                CString *literal = (CString*)(state->peaking_value);                \
                parse_error(state, expect " expected, but string '%.*s' met",       \
                            literal->length,                                        \
                            literal->content                                        \
                );                                                                  \
                break;                                                              \
            }                                                                       \
            default:                                                                \
                parse_error(state, expect " expected, but token '%c'(%d) met",      \
                            tok,                                                    \
                            tok                                                     \
                );                                                                  \
                break;                                                              \
        }


void
_lex_parse_number(ParseState *state)
{
    state->peaking_token = TOK_NUM;
    state->peaking_value = 0;
    while (state->current != state->limit && isdigit(*(state->current))) {
        state->peaking_value *= 10;
        state->peaking_value += *(state->current) - '0';
        state->current++;
        state->column++;
    }
}

void
_lex_parse_identifier(ParseState *state)
{
    char buffer[MAX_SHORT_STRING_LENGTH + 1],
         *buf_ptr = buffer;

    while (state->current != state->limit && isalnum(*(state->current))) {
        if (buf_ptr - buffer < MAX_SHORT_STRING_LENGTH) {
            *buf_ptr++ = *(state->current);
        }
        state->current++;
        state->column++;
    }

    state->peaking_token = TOK_ID;
    state->peaking_value = (intptr_t)string_pool_insert_vec(&state->string_pool, buffer, buf_ptr - buffer);
}

void
_lex_parse_block_comment(ParseState *state)
{
    state->current += 2;
    state->column += 2;
    while (state->current != state->limit) {
        while (*(state->current) != '*') {
            if (*(state->current) == '\n') {
                state->line++;
                state->column = 0;
            }
            else {
                state->column++;
            }

            if (++(state->current) == state->limit) {
                return;
            }
        }

        if (state->current + 1 == state->limit) {
            return;
        }
        else if (state->current[1] == '/') {
            state->current += 2;
            state->column += 2;
            return;
        }
        else {
            state->current ++;
            state->column ++;
        }
    }
}

void
_lex_parse_line_comment(ParseState *state)
{
    state->current += 2;
    state->column += 2;

    while (state->current != state->limit && *(state->current) != '\n') {
        state->current++;
        state->column++;
    }

    if (state->current != state->limit) {
        state->current++;
        state->line++;
        state->column = 0;
    }
}

int
_lex_peak(ParseState *state)
{
    if (state->peaking_token) {
        return state->peaking_token;
    }

    for (;;) {
        while (state->current != state->limit && isspace(*(state->current))) {
            if (*(state->current) == '\n') {
                state->line++;
                state->column = 0;
            }
            else {
                state->column++;
            }
            state->current++;
        }

        if (state->current == state->limit) {
            return state->peaking_token = TOK_EOF;
        }

        if (isdigit(*(state->current))) {
            _lex_parse_number(state);
        }
        else if (isalpha(*(state->current))) {
            _lex_parse_identifier(state);
        }
        else if (*(state->current) == '/') {
            if (state->current + 1 == state->limit) {
                state->peaking_token = *state->current++;
                state->column++;
            }
            else if (state->current[1] == '*') {
                _lex_parse_block_comment(state);
                continue;
            }
            else if (state->current[1] == '/') {
                _lex_parse_line_comment(state);
                continue;
            }
            else {
                state->peaking_token = *state->current++;
                state->column++;
            }
        }
        else {
            state->peaking_token = *(state->current);
            state->current++;
            state->column++;
        }

        return state->peaking_token;
    }
}

int
_lex_next(ParseState *state)
{
    if (!state->peaking_token) {
        _lex_peak(state);
    }

    int ret = state->peaking_token;
    state->peaking_token = 0;
    return ret;
}

ParseState *
parse_state_new_from_string(const char *content)
{
    ParseState *state = (ParseState*)malloc(sizeof(ParseState));

    state->filename = "<string>";
    state->line = 0;
    state->column = 0;

    state->content = content;
    state->content_length = strlen(content);

    state->string_pool = string_pool_new();
    state->inst_list = inst_list_new(16);

    state->current = state->content;
    state->limit = state->content + state->content_length;

    state->peaking_token = 0;
    state->peaking_value = 0;

    state->register_count = 1;
    state->symbol_table = hash_new(32);

    return state;
}

void
parse_state_destroy(ParseState *state)
{
    if (state->string_pool)     { string_pool_destroy(state->string_pool); }
    if (state->inst_list)       { inst_list_destroy(state->inst_list); }
    if (state->symbol_table)    { hash_destroy(state->symbol_table); }
    free(state);
}

#define _parse_symbol_table_lookup(state, string)       \
    hash_find(state->symbol_table, (uintptr_t)string)

#define _parse_exists_in_symbol_table(state, string)    \
    !value_is_undefined(_parse_symbol_table_lookup(state, string))

#define _parse_find_in_symbol_table(state, string)      \
    value_to_int(_parse_symbol_table_lookup(state, string))

static inline size_t
_parse_allocate_register(ParseState *state)
{ return state->register_count++; }

static inline void
_parse_insert_into_symbol_table(ParseState *state, CString *string, size_t reg)
{ hash_set_and_update(state->symbol_table, (uintptr_t)string, value_from_int(reg)); }

static inline void
_parse_inject_reserved(ParseState *state, CString *reserved[])
{
#define inject_reserved(id, literal)                                                                        \
    reserved[id] = string_pool_insert_str(&state->string_pool, literal);                                    \
    hash_set_and_update(state->symbol_table, (uintptr_t)reserved[id], value_from_int(-id))

    inject_reserved(R_LET, "let");

#undef inject_reserved
}

size_t _parse_right_hand_expr(ParseState *state);

size_t
_parse_unary_expr(ParseState *state)
{
    int tok = _lex_peak(state);
    size_t ret = 0;

    switch (tok) {
        case '(':
            _lex_next(state);
            ret = _parse_right_hand_expr(state);
            tok = _lex_next(state);
            if (tok != ')') {
                parse_expect_error(state, "')'", tok);
            }
            break;
        case TOK_NUM:
            ret = _parse_allocate_register(state);
            inst_list_push(
                state->inst_list,
                cvm_inst_new_i_type(
                    I_LI,
                    ret,
                    state->peaking_value
                )
            );
            _lex_next(state);
            break;
        case TOK_ID:
        {
            CString *literal = (CString*)state->peaking_value;
            intptr_t reg = _parse_find_in_symbol_table(state, literal);
            if (reg < 0) {
                parse_expect_error(state, "unary expr", tok);
                break;
            }
            else if (reg == 0) {
                parse_warn(state, "variable '%.*s' used before initializing it",
                           literal->length,
                           literal->content
                );
            }
            _lex_next(state);
            ret = (size_t)reg;
            break;
        }
        default:
            parse_expect_error(state, "unary expr", tok);
            break;
    }

    return ret;
}

size_t
_parse_mul_expr(ParseState *state)
{
    size_t ret = _parse_unary_expr(state);

    int tok = _lex_peak(state);
    if (tok == '*' || tok == '/' || tok == '%') {
        while (tok == '*' || tok == '/' || tok == '%') {
            size_t temp_reg = _parse_allocate_register(state);

            _lex_next(state);
            size_t b_reg = _parse_unary_expr(state);

            inst_list_push(
                state->inst_list,
                cvm_inst_new_d_type(
                    tok == '*' ? I_MUL :
                    tok == '/' ? I_DIV :
                                 I_MOD,
                    temp_reg,
                    ret,
                    b_reg
                )
            );

            ret = temp_reg;
            tok = _lex_peak(state);
        }
    }

    return ret;
}

size_t
_parse_add_expr(ParseState *state)
{
    size_t ret = _parse_mul_expr(state);

    int tok = _lex_peak(state);
    if (tok == '+' || tok == '-') {
        while (tok == '+' || tok == '-') {
            size_t temp_reg = _parse_allocate_register(state);

            _lex_next(state);
            size_t b_reg = _parse_mul_expr(state);

            inst_list_push(
                state->inst_list,
                cvm_inst_new_d_type(
                    tok == '+' ? I_ADD : I_SUB,
                    temp_reg,
                    ret,
                    b_reg
                )
            );

            ret = temp_reg;
            tok = _lex_peak(state);
        }
    }

    return ret;
}

size_t
_parse_conditional_expr(ParseState *state)
{
    return _parse_add_expr(state);
}

size_t
_parse_right_hand_expr(ParseState *state)
{ return _parse_conditional_expr(state); }

void
_parse_assignment_expr(ParseState *state)
{
    int tok = _lex_next(state);
    if (tok != TOK_ID) {
        parse_expect_error(state, "Identifier", tok);
        return;
    }

    CString *literal = (CString*)state->peaking_value;
    if (!_parse_exists_in_symbol_table(state, literal)) {
        parse_error(state, "Undefined identifier: '%.*s'",
                    literal->length,
                    literal->content
        );
        return;
    }

    intptr_t reg = _parse_find_in_symbol_table(state, literal);
    if (reg < 0) {
        parse_expect_error(state, "Identifier", tok);
        return;
    }

    tok = _lex_next(state);
    if (tok != '=') {
        parse_expect_error(state, "'='", tok);
        return;
    }

    _parse_insert_into_symbol_table(state, literal, _parse_right_hand_expr(state));
}

void
_parse_let_stmt(ParseState *state)
{
    assert(_lex_peak(state) == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_LET);

    do {
        _lex_next(state);
        int tok = _lex_next(state);
        if (tok != TOK_ID) {
            parse_expect_error(state, "Identifier", tok);
            return;
        }

        CString *literal = (CString*)state->peaking_value;
        if (_parse_exists_in_symbol_table(state, literal)) {
            parse_error(state, "Duplicated symbol: '%.*s'",
                        literal->length,
                        literal->content
            );
            return;
        }

        if (_lex_peak(state) == '=') {
            _lex_next(state);
            _parse_insert_into_symbol_table(state, literal, _parse_right_hand_expr(state));
        }
        else {
            _parse_insert_into_symbol_table(state, literal, 0);
        }
    } while (_lex_peak(state) == ',');

    if (_lex_next(state) != ';') {
        parse_expect_error(state, "';'", _lex_peak(state));
    }
}

void
_parse_expr_stmt(ParseState *state)
{
    _parse_assignment_expr(state);

    if (_lex_next(state) != ';') {
        parse_expect_error(state, "';'", _lex_peak(state));
    }
}

void
parse(ParseState *state)
{
    CString *reserved[RESERVED_WORD_NR];
    _parse_inject_reserved(state, reserved);

    while (_lex_peak(state) != TOK_EOF) {
        switch (_lex_peak(state)) {
            case TOK_EOF:
                parse_error(state, "%s", "Unexpected EOF");
                break;
            case TOK_ID:
            {
                if (_parse_exists_in_symbol_table(state, state->peaking_value)) {
                    switch (_parse_find_in_symbol_table(state, state->peaking_value)) {
                        case -R_LET:
                            _parse_let_stmt(state);
                            break;
                        default:
                            _parse_expr_stmt(state);
                            break;
                    }
                }
                else {
                    CString *literal = (CString*)(state->peaking_value);
                    parse_error(state, "Undefined identifier: '%.*s'",
                                literal->length,
                                literal->content
                    );
                }
                break;
            }
            default:
                parse_error(state, "Unexpected Token: '%c'(%d)",
                            _lex_peak(state),
                            _lex_peak(state)
                );
        }
    }
}
