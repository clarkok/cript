//
// Created by c on 4/16/16.
//

#include <assert.h>
#include <ctype.h>

#include "cvm.h"

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

#define _parse_push_inst(state, inst)                                               \
    do {                                                                            \
        FunctionScope *function = function_stack_top(state);                        \
        inst_list_push(function->inst_list, inst);                                  \
    } while (0)

#define _parse_current_inst_list(state)                                             \
    function_stack_top(state)->inst_list

#define _parse_inst_list_size(state)                                                \
    _parse_current_inst_list(state)->count


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

    while (
        state->current != state->limit &&
        (isalnum(*(state->current)) || *(state->current) == '_' || *(state->current) == '$')
    ) {
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

void
_lex_parse_string(ParseState *state)
{
    char buffer[256],
         *ptr = buffer,
         start = *(state->current);

    state->current++;
    state->column++;

    while (state->current != state->limit && *(state->current) != start) {
        char ch = *(state->current++);
        state->column++;
        if (ch == '\n') {
            state->line++;
            state->column = 0;
        }
        else if (ch == '\\') {
            switch (*(state->current)) {
                default:
                    ch = *(state->current++);
                    state->column++;
                    break;
            }
        }
        *ptr = ch;
        if (++ptr - buffer == 256) {
            parse_error(state, "%s", "string too long");
            return;
        }
    }

    if (state->current == state->limit) {
        parse_error(state, "%s", "uncompleted string literal");
        return;
    }

    state->current++;
    state->column++;
    *ptr = 0;

    state->peaking_token = TOK_STRING;
    state->peaking_value = (size_t)string_pool_insert_vec(&state->string_pool, buffer, ptr - buffer);
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
        else if (*(state->current) == '"' || *(state->current) == '\'') {
            _lex_parse_string(state);
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
        else if (
                *(state->current) == '>' || 
                *(state->current) == '<'
        ) {
            if (state->current + 1 == state->limit) {
                state->peaking_token = *state->current++;
                state->column++;
            }
            else if (state->current[1] == '=') {
                state->peaking_token = 
                    state->current[0] == '>' ? TOK_GE :
                                               TOK_LE ;
                state->current += 2;
                state->column += 2;
            }
            else if (state->current[1] == state->current[0]) {
                state->peaking_token =
                    state->current[0] == '>' ? TOK_SHR :
                                               TOK_SHL ;
                state->current += 2;
                state->column += 2;
            }
            else {
                state->peaking_token = *state->current++;
                state->column++;
            }
        }
        else if (
                *(state->current) == '=' ||
                *(state->current) == '!'
        ) {
            if (state->current + 1 == state->limit) {
                state->peaking_token = *state->current++;
                state->column++;
            }
            else if (state->current[1] == '=') {
                state->peaking_token = 
                    state->current[0] == '=' ? TOK_EQ :
                                               TOK_NE ;
                state->current += 2;
                state->column += 2;
            }
            else {
                state->peaking_token = *state->current++;
                state->column++;
            }
        }
        else if (*(state->current) == '&') {
            if (state->current + 1 == state->limit) {
                state->peaking_token = *state->current++;
                state->column++;
            }
            else if (state->current[1] == '&') {
                state->peaking_token = TOK_AND;
                state->current += 2;
                state->column += 2;
            }
            else {
                state->peaking_token = *state->current++;
                state->column++;
            }
        }
        else if (*(state->current) == '|') {
            if (state->current + 1 == state->limit) {
                state->peaking_token = *state->current++;
                state->column++;
            }
            else if (state->current[1] == '|') {
                state->peaking_token = TOK_OR;
                state->current += 2;
                state->column += 2;
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

static inline size_t
_parse_allocate_register(ParseState *state)
{ return _parse_allocate_reg_for_func(function_stack_top(state)); }

static inline void
_parse_insert_into_scope(ParseScope *scope, CString *string, size_t reg)
{ hash_set_and_update(scope->upper_table, (uintptr_t)string, value_from_int(reg)); }

static inline void
_parse_define_into_scope(ParseScope *scope, CString *string, size_t reg)
{ hash_set_and_update(scope->symbol_table, (uintptr_t)string, value_from_int(reg)); }

static inline FunctionScope *
_parse_function_scope_new()
{
    FunctionScope *ret = malloc(sizeof(FunctionScope));
    list_node_init(&ret->_linked);
    list_init(&ret->scopes);
    ret->arguments_nr = 0;
    ret->register_nr = 1;
    ret->capture_list = hash_new(HASH_MIN_CAPACITY);
    ret->inst_list = inst_list_new(16);

    ParseScope *scope = malloc(sizeof(ParseScope));
    scope->symbol_table = hash_new(HASH_MIN_CAPACITY);
    scope->upper_table = hash_new(HASH_MIN_CAPACITY);
    list_prepend(&ret->scopes, &scope->_linked);

    hash_set_and_update(ret->capture_list, 0, value_from_int(0));

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

    state->current = state->content;
    state->limit = state->content + state->content_length;

    state->peaking_token = 0;
    state->peaking_value = 0;

    list_init(&state->functions);

    list_init(&state->function_stack);
    FunctionScope *function = _parse_function_scope_new();
    list_prepend(&state->function_stack, &function->_linked);

    size_t global_reg = _parse_allocate_register(state);

    _parse_define_into_scope(
        list_get(list_head(&function->scopes), ParseScope, _linked),
        string_pool_insert_str(&state->string_pool, "global"),
        global_reg
    );
    return state;
}

void
parse_state_destroy(ParseState *state)
{
    if (state->string_pool)     { string_pool_destroy(state->string_pool); }
    while (list_size(&state->function_stack)) {
        FunctionScope *function = list_get(list_unlink(list_head(&state->function_stack)), FunctionScope, _linked);

        while (list_size(&function->scopes)) {
            ParseScope *scope = list_get(list_unlink(list_head(&function->scopes)), ParseScope, _linked);
            hash_destroy(scope->symbol_table);
            free(scope);
        }

        hash_destroy(function->capture_list);
        inst_list_destroy(function->inst_list);
        free(function);
    }
    free(state);
}

static inline void
_parse_push_scope(ParseState *state)
{
    ParseScope *scope = malloc(sizeof(ParseScope));
    scope->symbol_table = hash_new(HASH_MIN_CAPACITY);
    scope->upper_table = hash_new(HASH_MIN_CAPACITY);
    list_prepend(&function_stack_top(state)->scopes, &scope->_linked);
}

void
_parse_join_scope(ParseState *state)
{
    ParseScope *scope = scope_stack_top(state);
    list_unlink(list_head(&function_stack_top(state)->scopes));

    hash_for_each(scope->upper_table, node) {
        _parse_push_inst(state,
                       cvm_inst_new_d_type(
                           I_ADD,
                           (size_t)_parse_find_in_symbol_table(state, node->key),
                           (size_t)value_to_int(node->value),
                           0
                       )
        );
    }

    hash_destroy(scope->upper_table);
    hash_destroy(scope->symbol_table);
    free(scope);
}

static inline void
_parse_push_function(ParseState *state)
{
    FunctionScope *function_scope = _parse_function_scope_new();
    list_prepend(&state->function_stack, &function_scope->_linked);
}

static inline VMFunction *
_parse_pop_function(ParseState *state)
{
    VMFunction *function = malloc(sizeof(VMFunction));
    FunctionScope *func_scope = list_get(list_unlink(list_head(&state->function_stack)), FunctionScope, _linked);

    list_node_init(&function->_linked);
    function->arguments_nr = func_scope->arguments_nr;
    function->register_nr = func_scope->register_nr;
    function->capture_list = func_scope->capture_list;
    function->inst_list = func_scope->inst_list;

    assert(list_size(&func_scope->scopes) == 1);
    ParseScope *scope = list_get(list_head(&func_scope->scopes), ParseScope, _linked);
    hash_destroy(scope->symbol_table);
    hash_destroy(scope->upper_table);
    free(scope);
    free(func_scope);

    list_append(&state->functions, &function->_linked);

    return function;
}

static inline void
_parse_inject_reserved(ParseState *state, CString *reserved[])
{
    ParseScope *global_scope = scope_stack_top(state);

#define inject_reserved(id, literal)                                                                        \
    reserved[id] = string_pool_insert_str(&state->string_pool, literal);                                    \
    hash_set_and_update(global_scope->symbol_table, (uintptr_t)reserved[id], value_from_int(-id))

    inject_reserved(R_LET, "let");
    inject_reserved(R_IF, "if");
    inject_reserved(R_ELSE, "else");
    inject_reserved(R_WHILE, "while");
    inject_reserved(R_FUNCTION, "function");
    inject_reserved(R_RETURN, "return");

#undef inject_reserved
}

size_t _parse_right_hand_expr(ParseState *state);

size_t
_parse_object_literal(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == '{');

    size_t ret = _parse_allocate_register(state);
    _parse_push_inst(
        state,
        cvm_inst_new_d_type(
            I_NEW_OBJ,
            ret,
            0, 0
        )
    );

    tok = _lex_peak(state);
    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }

    if (tok != '}') {
        size_t key_reg = _parse_allocate_register(state);
        while (1) {
            tok = _lex_next(state);
            if (tok != TOK_ID && tok != TOK_STRING) {
                parse_expect_error(state, "id or string", tok);
                return 0;
            }

            CString *string = (CString*)state->peaking_value;

            if ((tok = _lex_next(state)) != ':') {
                parse_expect_error(state, "':'", tok);
                return 0;
            }

            size_t value_reg = _parse_right_hand_expr(state);

            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_LSTR,
                    key_reg,
                    (intptr_t)string
                )
            );

            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_SET_OBJ,
                    ret,
                    value_reg,
                    key_reg
                )
            );

            tok = _lex_peak(state);
            if (tok == TOK_EOF) {
                parse_error(state, "%s", "Unexpected EOF");
                return 0;
            }
            else if (tok == ',') {
                _lex_next(state);
            }
            else if (tok == '}') {
                break;
            }
            else {
                parse_expect_error(state, "',' or '}'", tok);
            }
        }
    }

    _lex_next(state);   // for '}'

    return ret;
}

size_t
_parse_array_literal(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == '[');

    size_t ret = _parse_allocate_register(state);
    _parse_push_inst(
        state, 
        cvm_inst_new_d_type(
            I_NEW_ARR,
            ret,
            0, 0
        )
    );

    tok = _lex_peak(state);
    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }

    if (tok != ']') {
        size_t one_reg = _parse_allocate_register(state);
        _parse_push_inst(
            state,
            cvm_inst_new_i_type(
                I_LI,
                one_reg,
                1
            )
        );

        size_t key_reg = _parse_allocate_register(state);
        _parse_push_inst(
            state,
            cvm_inst_new_i_type(
                I_LI,
                key_reg,
                0
            )
        );

        while (1) {
            size_t value_reg = _parse_right_hand_expr(state);
            tok = _lex_peak(state);

            if (tok == ':') {
                _lex_next(state);
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_ADD,
                        key_reg,
                        value_reg,
                        0
                    )
                );

                value_reg = _parse_right_hand_expr(state);
                tok = _lex_peak(state);
            }

            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_SET_OBJ,
                    ret,
                    value_reg,
                    key_reg
                )
            );

            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_ADD,
                    key_reg,
                    key_reg,
                    one_reg
                )
            );

            if (tok == TOK_EOF) {
                parse_error(state, "%s", "unexpected EOF");
                return 0;
            }

            if (tok == ']') {
                break;
            }
            else if (tok == ',') {
                _lex_next(state);
            }
            else {
                parse_expect_error(state, "','", tok);
            }
        }
    }
    _lex_next(state);

    return ret;
}

void _parse_statement(ParseState *state);

size_t
_parse_function_literal(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_FUNCTION);

    _parse_push_function(state);

    tok = _lex_next(state);
    if (tok != '(') {
        parse_expect_error(state, "'('", tok);
        return 0;
    }

    size_t this_reg = _parse_allocate_register(state);
    assert(this_reg == 1);

    _parse_define_into_scope(
        scope_stack_top(state),
        string_pool_insert_str(&state->string_pool, "this"),
        this_reg
    );

    tok = _lex_peak(state);
    while (tok != TOK_EOF && tok != ')') {
        if (tok != TOK_ID) {
            parse_expect_error(state, "identifier", tok);
            break;
        }
        _lex_next(state);
        CString *literal = (CString*)state->peaking_value;
        _parse_define_into_scope(scope_stack_top(state), literal, _parse_allocate_register(state));
        function_stack_top(state)->arguments_nr++;

        tok = _lex_peak(state);
        if (tok == ')') {
            break;
        }
        else if (tok == ',') {
            _lex_next(state);
            tok = _lex_peak(state);
        }
        else {
            parse_expect_error(state, "','", tok);
            break;
        }
    }

    tok = _lex_next(state);
    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }
    assert(tok == ')');

    tok = _lex_next(state);
    if (tok != '{') {
        parse_expect_error(state, "'{'", tok);
        return 0;
    }

    tok = _lex_peak(state);
    while (tok != TOK_EOF && tok != '}') {
        _parse_statement(state);
        tok = _lex_peak(state);
    }

    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }
    assert(tok == '}');
    _lex_next(state);

    size_t ret_reg = _parse_allocate_register(state);
    _parse_push_inst(
        state,
        cvm_inst_new_i_type(
            I_UNDEFINED,
            ret_reg,
            0
        )
    );

    _parse_push_inst(
        state,
        cvm_inst_new_i_type(
            I_RET,
            ret_reg,
            0
        )
    );

    VMFunction *func = _parse_pop_function(state);

    size_t ret = _parse_allocate_register(state);
    _parse_push_inst(
        state,
        cvm_inst_new_i_type(
            I_NEW_CLS,
            ret,
            (size_t)func
        )
    );

    return ret;
}

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
            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_LI,
                    ret,
                    state->peaking_value
                )
            );
            _lex_next(state);
            break;
        case TOK_STRING:
            ret = _parse_allocate_register(state);
            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_LSTR,
                    ret,
                    state->peaking_value
                )
            );
            _lex_next(state);
            break;
        case TOK_ID:
        {
            CString *literal = (CString*)state->peaking_value;
            if (!_parse_exists_in_symbol_table(state, literal)) {
                parse_error(state, "Undefined identifier: '%.*s'",
                            literal->length,
                            literal->content
                );
            }
            intptr_t reg = _parse_find_in_symbol_table(state, literal);
            if (reg == -R_FUNCTION) {
                ret = _parse_function_literal(state);
            }
            else if (reg < 0) {
                parse_expect_error(state, "unary expr", tok);
                break;
            }
            else {
                if (reg == 0) {
                    parse_warn(state, "variable '%.*s' used before initializing it",
                               literal->length,
                               literal->content
                    );
                }
                _lex_next(state);
                ret = (size_t)reg;
            }
            break;
        }
        case '{':
            ret = _parse_object_literal(state);
            break;
        case '[':
            ret = _parse_array_literal(state);
            break;
        default:
            parse_expect_error(state, "unary expr", tok);
            break;
    }

    return ret;
}

size_t
_parse_arguments(ParseState *state, size_t this_reg)
{
    int tok = _lex_next(state);
    assert(tok == '(');

    size_t arg_reg = _parse_allocate_register(state);
    size_t key_reg = _parse_allocate_register(state);
    size_t one_reg = _parse_allocate_register(state);

    _parse_push_inst(
        state,
        cvm_inst_new_d_type(
            I_NEW_ARR,
            arg_reg,
            0, 0
        )
    );

    _parse_push_inst(
        state,
        cvm_inst_new_i_type(
            I_LI,
            one_reg,
            1
        )
    );

    _parse_push_inst(
        state,
        cvm_inst_new_i_type(
            I_LI,
            key_reg,
            0
        )
    );

    _parse_push_inst(
        state,
        cvm_inst_new_d_type(
            I_SET_OBJ,
            arg_reg,
            this_reg,
            key_reg
        )
    );

    tok = _lex_peak(state);
    while (tok != TOK_EOF && tok != ')') {
        size_t result_reg = _parse_right_hand_expr(state);

        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_ADD,
                key_reg,
                key_reg,
                one_reg
            )
        );

        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_SET_OBJ,
                arg_reg,
                result_reg,
                key_reg
            )
        );

        tok = _lex_peak(state);
        if (tok == ')') {
            break;
        }
        else if (tok == ',') {
            _lex_next(state);
        }
        else {
            parse_expect_error(state, "','", tok);
        }
    }

    _lex_next(state);

    return arg_reg;
}

size_t
_parse_postfix_expr(ParseState *state)
{
    size_t ret = _parse_unary_expr(state);
    size_t this_reg = _parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));

    int tok = _lex_peak(state);
    while (tok == '.' || tok == '[' || tok == '(') {
        if (tok == '.') {
            _lex_next(state);
            tok = _lex_next(state);
            if (tok != TOK_ID) {
                parse_expect_error(state, "identifier", tok);
                return 0;
            }

            size_t key_reg = _parse_allocate_register(state);
            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_LSTR,
                    key_reg,
                    state->peaking_value
                )
            );

            size_t temp_reg = _parse_allocate_register(state);
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_GET_OBJ,
                    temp_reg,
                    ret,
                    key_reg
                )
            );

            this_reg = ret;
            ret = temp_reg;
        }
        else if (tok == '[') {
            _lex_next(state);
            size_t key_reg = _parse_right_hand_expr(state);
            size_t temp_reg = _parse_allocate_register(state);
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_GET_OBJ,
                    temp_reg,
                    ret,
                    key_reg
                )
            );
            tok = _lex_next(state);
            if (tok != ']') {
                parse_expect_error(state, "']'", tok);
                return 0;
            }

            this_reg = ret;
            ret = temp_reg;
        }
        else {
            size_t func_reg = ret;
            size_t arg_reg = _parse_arguments(state, this_reg);

            size_t temp_reg = _parse_allocate_register(state);
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_CALL,
                    temp_reg,
                    func_reg,
                    arg_reg
                )
            );

            this_reg = _parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));
            ret = temp_reg;
        }

        tok = _lex_peak(state);
    }

    return ret;
}

size_t
_parse_mul_expr(ParseState *state)
{
    size_t ret = _parse_postfix_expr(state);

    int tok = _lex_peak(state);
    while (tok == '*' || tok == '/' || tok == '%') {
        _lex_next(state);
        size_t b_reg = _parse_postfix_expr(state);
        size_t temp_reg = _parse_allocate_register(state);

        _parse_push_inst(
            state,
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

    return ret;
}

size_t
_parse_add_expr(ParseState *state)
{
    size_t ret = _parse_mul_expr(state);

    int tok = _lex_peak(state);
    while (tok == '+' || tok == '-') {
        _lex_next(state);
        size_t b_reg = _parse_mul_expr(state);
        size_t temp_reg = _parse_allocate_register(state);

        _parse_push_inst(
            state,
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

    return ret;
}

size_t
_parse_compare_expr(ParseState *state)
{
    size_t ret = _parse_add_expr(state);

    int tok = _lex_peak(state);
    while (
            tok == '<' ||
            tok == '>' ||
            tok == TOK_LE ||
            tok == TOK_GE ||
            tok == TOK_EQ ||
            tok == TOK_NE
    ) {
        _lex_next(state);
        size_t b_reg = _parse_add_expr(state);
        size_t temp_reg = _parse_allocate_register(state);

        switch (tok) {
            case '<':
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SLT,
                        temp_reg,
                        ret,
                        b_reg
                    )
                );
                break;
            case '>':
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SLT,
                        temp_reg,
                        b_reg,
                        ret
                    )
                );
                break;
            case TOK_LE:
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SLE,
                        temp_reg,
                        ret,
                        b_reg
                    )
                );
                break;
            case TOK_GE:
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SLE,
                        temp_reg,
                        b_reg,
                        ret
                    )
                );
                break;
            case TOK_EQ:
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SEQ,
                        temp_reg,
                        b_reg,
                        ret
                    )
                );
                break;
            case TOK_NE:
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_SEQ,
                        temp_reg,
                        b_reg,
                        ret
                    )
                );
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_LNOT,
                        temp_reg,
                        temp_reg,
                        0
                    )
                );
                break;
            default:
                assert(0);
        }
        ret = temp_reg;
        tok = _lex_peak(state);
    }

    return ret;
}

size_t
_parse_logic_and_expr(ParseState *state)
{
    size_t ret = _parse_compare_expr(state);

    int tok = _lex_peak(state);
    if (tok == TOK_AND) {
        size_t temp_reg = _parse_allocate_register(state);
        size_t last_index = 0;
        size_t current_index = 0;

        while (tok == TOK_AND) {
            _lex_next(state);
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_ADD,
                    temp_reg,
                    ret,
                    0
                )
            );
            current_index = _parse_inst_list_size(state);
            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_BNR,
                    ret,
                    last_index
                )
            );
            last_index = current_index;

            ret = _parse_compare_expr(state);
            tok = _lex_peak(state);
        }

        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_ADD,
                temp_reg,
                ret,
                0
            )
        );

        while (last_index) {
            current_index = last_index;
            last_index = (size_t)_parse_current_inst_list(state)->insts[current_index].i_imm;
            _parse_current_inst_list(state)->insts[current_index].i_imm =
                _parse_current_inst_list(state)->count - current_index - 1;
        }

        ret = temp_reg;
    }
    return ret;
}

size_t
_parse_logic_or_expr(ParseState *state)
{
    size_t ret = _parse_logic_and_expr(state);

    int tok = _lex_peak(state);
    if (tok == TOK_OR) {
        size_t temp_reg = _parse_allocate_register(state);
        size_t last_index = 0;
        size_t current_index = 0;

        while (tok == TOK_OR) {
            _lex_next(state);
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_ADD,
                    temp_reg,
                    ret,
                    0
                )
            );
            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_LNOT,
                    ret,
                    ret,
                    0
                )
            );
            current_index = _parse_inst_list_size(state);
            _parse_push_inst(
                state,
                cvm_inst_new_i_type(
                    I_BNR,
                    ret,
                    last_index
                )
            );
            last_index = current_index;

            ret = _parse_compare_expr(state);
            tok = _lex_peak(state);
        }

        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_ADD,
                temp_reg,
                ret,
                0
            )
        );

        while (last_index) {
            current_index = last_index;
            last_index = (size_t)_parse_current_inst_list(state)->insts[current_index].i_imm;
            _parse_current_inst_list(state)->insts[current_index].i_imm =
                _parse_current_inst_list(state)->count - current_index - 1;
        }

        ret = temp_reg;
    }
    return ret;
}

size_t
_parse_conditional_expr(ParseState *state)
{
    return _parse_logic_or_expr(state);
}

size_t
_parse_right_hand_expr(ParseState *state)
{ return _parse_conditional_expr(state); }

void
_parse_assignment_expr(ParseState *state)
{
    size_t object_reg = _parse_unary_expr(state);
    size_t this_reg = _parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));
    size_t key_reg = 0;
    int tok = _lex_peak(state);
    CString *literal = (CString*)state->peaking_value;

    while (tok == '.' || tok == '[' || tok == '(') {
        if (tok == '(') {
            size_t arg_reg = _parse_arguments(state, this_reg);
            size_t temp_reg = _parse_allocate_register(state);

            _parse_push_inst(
                state,
                cvm_inst_new_d_type(
                    I_CALL,
                    temp_reg,
                    object_reg,
                    arg_reg
                )
            );

            object_reg = temp_reg;
            this_reg = _parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));
            key_reg = 0;

            tok = _lex_peak(state);
        }
        else {
            if (tok == '.') {
                _lex_next(state);
                tok = _lex_next(state);
                if (tok != TOK_ID) {
                    parse_expect_error(state, "identifier", tok);
                    return;
                }

                CString *string = (CString*)state->peaking_value;
                key_reg = _parse_allocate_register(state);
                _parse_push_inst(
                    state,
                    cvm_inst_new_i_type(
                        I_LSTR,
                        key_reg,
                        (intptr_t)string
                    )
                );
            }
            else {
                _lex_next(state);
                key_reg = _parse_right_hand_expr(state);
                tok = _lex_next(state);
                if (tok != ']') {
                    parse_expect_error(state, "']'", tok);
                    return;
                }
            }

            tok = _lex_peak(state);
            if (tok != '.' && tok != '[' && tok != '(') {
                break;
            }
            else {
                size_t temp_reg = _parse_allocate_register(state);
                _parse_push_inst(
                    state,
                    cvm_inst_new_d_type(
                        I_GET_OBJ,
                        temp_reg,
                        object_reg,
                        key_reg
                    )
                );
                key_reg = 0;
                this_reg = object_reg;
                object_reg = temp_reg;
            }
        }
    }

    tok = _lex_peak(state);
    if (tok != '=') {
        if (tok == ';') {
            return;
        }
        else {
            parse_expect_error(state, "'='", tok);
            return;
        }
    }
    else {
        _lex_next(state);
    }

    size_t reg_count = function_stack_top(state)->register_nr;
    size_t result_reg = _parse_right_hand_expr(state);

    if (function_stack_top(state)->register_nr == reg_count) {
        size_t temp_reg = _parse_allocate_register(state);
        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_ADD,
                temp_reg,
                result_reg,
                0
            )
        );
        result_reg = temp_reg;
    }

    if (key_reg) {
        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_SET_OBJ,
                object_reg,
                result_reg,
                key_reg
            )
        );
    }
    else {
        if (_parse_find_define_scope(state, literal) == scope_stack_top(state)) {
            _parse_define_into_scope(scope_stack_top(state), literal, result_reg);
        }
        else {
            _parse_insert_into_scope(scope_stack_top(state), literal, result_reg);
        }
    }
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
        if (_parse_exists_in_symbol_table_current(state, literal)) {
            parse_error(state, "Duplicated symbol: '%.*s'",
                        literal->length,
                        literal->content
            );
            return;
        }

        if (_lex_peak(state) == '=') {
            _lex_next(state);
            _parse_define_into_scope(scope_stack_top(state), literal, _parse_right_hand_expr(state));
        }
        else {
            _parse_define_into_scope(scope_stack_top(state), literal, 0);
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

void _parse_statement(ParseState *state);

void
_parse_block_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == '{');

    _parse_push_scope(state);

    while (_lex_peak(state) != TOK_EOF && _lex_peak(state) != '}') {
        _parse_statement(state);
    }

    _parse_join_scope(state);

    if (_lex_next(state) == TOK_EOF) {
        parse_error(state, "%s", "Unexpected EOF");
    }
}

void
_parse_if_else_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_IF);

    tok = _lex_next(state);
    if (tok != '(') {
        parse_expect_error(state, "'('", tok);
        return;
    }
    size_t reg = _parse_right_hand_expr(state);
    tok = _lex_next(state);
    if (tok != ')') {
        parse_expect_error(state, "')'", tok);
        return;
    }

    size_t bnr_index = _parse_inst_list_size(state);
    _parse_push_inst(state, cvm_inst_new_i_type(I_BNR, reg, 0));

    _parse_push_scope(state);
    _parse_statement(state);
    _parse_join_scope(state);

    tok = _lex_peak(state);
    if (tok == TOK_ID && _parse_find_in_symbol_table(state, state->peaking_value) == -R_ELSE) {
        _lex_next(state);
        size_t j_index = _parse_inst_list_size(state);
        _parse_push_inst(state, cvm_inst_new_i_type(I_J, 0, 0));

        _parse_current_inst_list(state)->insts[bnr_index].i_imm =
            _parse_inst_list_size(state) - bnr_index - 1;
        _parse_push_scope(state);
        _parse_statement(state);
        _parse_join_scope(state);

        _parse_current_inst_list(state)->insts[j_index].i_imm =
            _parse_inst_list_size(state);
    }
    else {
        _parse_current_inst_list(state)->insts[bnr_index].i_imm =
            _parse_inst_list_size(state) - bnr_index - 1;
    }
}

void
_parse_while_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_WHILE);

    size_t while_begin = _parse_inst_list_size(state);

    tok = _lex_next(state);
    if (tok != '(') {
        parse_expect_error(state, "'('", tok);
        return;
    }
    size_t reg = _parse_right_hand_expr(state);
    tok = _lex_next(state);
    if (tok != ')') {
        parse_expect_error(state, "')'", tok);
        return;
    }

    size_t bnr_index = _parse_inst_list_size(state);
    _parse_push_inst(state, cvm_inst_new_i_type(I_BNR, reg, 0));

    _parse_push_scope(state);
    _parse_statement(state);
    _parse_join_scope(state);
    _parse_push_inst(state, cvm_inst_new_i_type(I_J, 0, while_begin));

    _parse_current_inst_list(state)->insts[bnr_index].i_imm =
        _parse_inst_list_size(state) - bnr_index - 1;
}

void
_parse_return_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_RETURN);

    tok = _lex_peak(state);
    size_t ret_reg;
    if (tok != ';') {
        ret_reg = _parse_right_hand_expr(state);
    }
    else {
        ret_reg = _parse_allocate_register(state);
        _parse_push_inst(
            state,
            cvm_inst_new_d_type(
                I_UNDEFINED,
                ret_reg,
                0, 0
            )
        );
    }
    _parse_push_inst(
        state,
        cvm_inst_new_d_type(
            I_RET,
            ret_reg,
            0, 0
        )
    );

    tok = _lex_next(state);
    if (tok != ';') {
        parse_expect_error(state, "';'", tok);
    }
}

void
_parse_statement(ParseState *state)
{
    switch (_lex_peak(state)) {
        case TOK_EOF:
            parse_error(state, "%s", "Unexpected EOF");
            break;
        case '{':
            _parse_block_stmt(state);
            break;
        case TOK_ID:
        {
            if (_parse_exists_in_symbol_table(state, state->peaking_value)) {
                switch (_parse_find_in_symbol_table(state, state->peaking_value)) {
                    case -R_LET:
                        _parse_let_stmt(state);
                        break;
                    case -R_IF:
                        _parse_if_else_stmt(state);
                        break;
                    case -R_WHILE:
                        _parse_while_stmt(state);
                        break;
                    case -R_RETURN:
                        _parse_return_stmt(state);
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

void
parse(ParseState *state)
{
    CString *reserved[RESERVED_WORD_NR];
    _parse_inject_reserved(state, reserved);

    while (_lex_peak(state) != TOK_EOF) {
        _parse_statement(state);
    }
}

VMFunction *
parse_get_main_function(ParseState *state)
{
    VMFunction *function = malloc(sizeof(VMFunction));
    FunctionScope *func_scope = function_stack_top(state);

    list_node_init(&function->_linked);
    function->arguments_nr = func_scope->arguments_nr;
    function->register_nr = func_scope->register_nr;
    function->capture_list = func_scope->capture_list;  func_scope->capture_list = NULL;
    function->inst_list = func_scope->inst_list;        func_scope->inst_list = NULL;

    return function;
}
