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
    ret->capture_list = hash_new(HASH_MIN_CAPACITY);
    ret->builder = ir_builder_new(0);
    ret->current_bb = ir_builder_entry(ret->builder);

    ParseScope *scope = malloc(sizeof(ParseScope));
    scope->symbol_table = hash_new(HASH_MIN_CAPACITY);
    scope->upper_table = hash_new(HASH_MIN_CAPACITY);
    list_prepend(&ret->scopes, &scope->_linked);

    hash_set_and_update(ret->capture_list, 0, value_from_int(0));

    return ret;
}

ParseState *
_parse_state_new(const char *filename, const char *content, int should_free_content)
{
    ParseState *state = (ParseState*)malloc(sizeof(ParseState));

    state->filename = filename;
    state->line = 0;
    state->column = 0;

    state->content = content;
    state->should_free_content = should_free_content;
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
    assert(global_reg == 1);

    _parse_define_into_scope(
        list_get(list_head(&function->scopes), ParseScope, _linked),
        string_pool_insert_str(&state->string_pool, "global"),
        global_reg
    );
    return state;
}

ParseState *
parse_state_new_from_string(const char *content)
{ return _parse_state_new("<string>", content, 0); }

ParseState *
parse_state_new_from_file(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        error_f("Cannot open file: %s\n", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!fread(content, file_size, 1, file)) {
        error_f("Cannot read file: %s\n", path);
        return NULL;
    }
    fclose(file);
    content[file_size] = '\0';

    return _parse_state_new(path, content, 1);
}

ParseState *
parse_state_expand_from_file(VMState *vm, const char *path)
{
    ParseState *state = parse_state_new_from_file(path);
    string_pool_destroy(state->string_pool);
    state->string_pool = vm->string_pool;   vm->string_pool = NULL;

    size_t global_reg = function_stack_top(state)->builder->register_nr;
    if (global_reg != 1) {
        error_f("Global register should be 1, but actual is %d", global_reg);
        assert(global_reg == 1);
    }

    _parse_define_into_scope(
        list_get(list_head(&function_stack_top(state)->scopes), ParseScope, _linked),
        string_pool_insert_str(&state->string_pool, "global"),
        global_reg
    );

    return state;
}

void
parse_state_destroy(ParseState *state)
{
    if (state->string_pool)     { string_pool_destroy(state->string_pool); }
    if (state->should_free_content) { free((void*)state->content); }
    while (list_size(&state->function_stack)) {
        FunctionScope *function = list_get(list_unlink(list_head(&state->function_stack)), FunctionScope, _linked);

        while (list_size(&function->scopes)) {
            ParseScope *scope = list_get(list_unlink(list_head(&function->scopes)), ParseScope, _linked);
            hash_destroy(scope->symbol_table);
            free(scope);
        }

        hash_destroy(function->capture_list);
        if (function->builder) {
            ir_builder_destroy(function->builder);
        }
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
    list_unlink(&scope->_linked);

    hash_for_each(scope->upper_table, node) {
        if (_parse_find_define_scope(state, (CString*)node->key) == scope_stack_top(state)) {
            _parse_define_into_scope(
                scope_stack_top(state),
                (CString*)node->key,
                (size_t)value_to_int(node->value)
            );
        }
        else {
            _parse_insert_into_scope(
                scope_stack_top(state),
                (CString*)node->key,
                (size_t)value_to_int(node->value)
            );
        }
    }

    hash_destroy(scope->upper_table);
    hash_destroy(scope->symbol_table);
    free(scope);
}

void
_parse_pop_scope(ParseState *state)
{
    ParseScope *scope = scope_stack_top(state);
    list_unlink(&scope->_linked);
    hash_destroy(scope->upper_table);
    hash_destroy(scope->symbol_table);
    free(scope);
}

static inline void
_parse_push_function(ParseState *state)
{
    FunctionScope *function_scope = _parse_function_scope_new();
    list_prepend(&state->function_stack, &function_scope->_linked);

    _parse_define_into_scope(scope_stack_top(state), string_pool_insert_str(&state->string_pool, "this"), 1);
}

static inline VMFunction *
_parse_pop_function(ParseState *state)
{
    VMFunction *function = malloc(sizeof(VMFunction));
    FunctionScope *func_scope = list_get(list_unlink(list_head(&state->function_stack)), FunctionScope, _linked);

    list_node_init(&function->_linked);
    function->arguments_nr = func_scope->arguments_nr;
    function->register_nr = func_scope->builder->register_nr + 1;
    function->capture_list = func_scope->capture_list;
    function->inst_list = ir_builder_destroy(func_scope->builder);
    func_scope->builder = NULL;
    func_scope->current_bb = NULL;

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
    inject_reserved(R_HALT, "halt");

#undef inject_reserved
}

size_t _parse_right_hand_expr(ParseState *state);

size_t
_parse_object_literal(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == '{');

    size_t ret = ir_builder_new_obj(function_stack_top(state)->current_bb);

    tok = _lex_peak(state);
    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }

    if (tok != '}') {
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
            size_t key_reg = ir_builder_lstr(function_stack_top(state)->current_bb, string);
            ir_builder_set_obj(function_stack_top(state)->current_bb, ret, value_reg, key_reg);

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

    size_t ret = ir_builder_new_arr(function_stack_top(state)->current_bb);

    tok = _lex_peak(state);
    if (tok == TOK_EOF) {
        parse_error(state, "%s", "unexpected EOF");
        return 0;
    }

    if (tok != ']') {
        size_t one_reg = ir_builder_li(function_stack_top(state)->current_bb, 1);
        size_t key_reg = ir_builder_li(function_stack_top(state)->current_bb, 0);

        while (1) {
            size_t value_reg = _parse_right_hand_expr(state);
            tok = _lex_peak(state);

            if (tok == ':') {
                _lex_next(state);
                key_reg = ir_builder_mov(function_stack_top(state)->current_bb, value_reg);
                value_reg = _parse_right_hand_expr(state);
                tok = _lex_peak(state);
            }

            ir_builder_set_obj(function_stack_top(state)->current_bb, ret, value_reg, key_reg);
            key_reg = ir_builder_add(function_stack_top(state)->current_bb, key_reg, one_reg);

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

    ir_builder_ret(
        function_stack_top(state)->current_bb,
        ir_builder_undefined(function_stack_top(state)->current_bb)
    );

    VMFunction *func = _parse_pop_function(state);

    return ir_builder_new_cls(function_stack_top(state)->current_bb, func);
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
            ret = ir_builder_li(function_stack_top(state)->current_bb, state->peaking_value);
            _lex_next(state);
            break;
        case TOK_STRING:
            ret = ir_builder_lstr(function_stack_top(state)->current_bb, (CString*)state->peaking_value);
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

    size_t arg_reg = ir_builder_new_arr(function_stack_top(state)->current_bb);
    size_t one_reg = ir_builder_li(function_stack_top(state)->current_bb, 1);
    size_t key_reg = ir_builder_li(function_stack_top(state)->current_bb, 0);

    ir_builder_set_obj(
        function_stack_top(state)->current_bb,
        arg_reg,
        this_reg,
        key_reg
    );

    tok = _lex_peak(state);
    while (tok != TOK_EOF && tok != ')') {
        size_t result_reg = _parse_right_hand_expr(state);
        key_reg = ir_builder_add(
            function_stack_top(state)->current_bb,
            key_reg,
            one_reg
        );

        ir_builder_set_obj(
            function_stack_top(state)->current_bb,
            arg_reg,
            result_reg,
            key_reg
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

    int tok = _lex_peak(state);
    if (tok == '.' || tok == '[' || tok == '(') {
        size_t this_reg = (size_t)_parse_find_in_symbol_table(
            state,
            string_pool_find_str(state->string_pool, "global")
        );

        while (tok == '.' || tok == '[' || tok == '(') {
            if (tok == '.') {
                _lex_next(state);
                tok = _lex_next(state);
                if (tok != TOK_ID) {
                    parse_expect_error(state, "identifier", tok);
                    return 0;
                }

                size_t key_reg = ir_builder_lstr(
                    function_stack_top(state)->current_bb,
                    (CString*)state->peaking_value
                );

                size_t temp_reg = ir_builder_get_obj(
                    function_stack_top(state)->current_bb,
                    ret,
                    key_reg
                );

                this_reg = ret;
                ret = temp_reg;
            }
            else if (tok == '[') {
                _lex_next(state);
                size_t key_reg = _parse_right_hand_expr(state);
                size_t temp_reg = ir_builder_get_obj(
                    function_stack_top(state)->current_bb,
                    ret,
                    key_reg
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
                size_t temp_reg = ir_builder_call(
                    function_stack_top(state)->current_bb,
                    func_reg,
                    arg_reg
                );
                this_reg = (size_t)_parse_find_in_symbol_table(
                    state,
                    string_pool_find_str(state->string_pool, "global")
                );
                ret = temp_reg;
            }

            tok = _lex_peak(state);
        }
    }

    return ret;
}

size_t
_parse_prefix_expr(ParseState *state)
{
    int tok = _lex_peak(state);
    if (tok == '!') {
        _lex_next(state);

        size_t ret = _parse_prefix_expr(state);
        return ir_builder_lnot(
            function_stack_top(state)->current_bb,
            ret
        );
    }
    else if (tok == '-') {
        _lex_next(state);

        size_t ret = _parse_prefix_expr(state);
        return ir_builder_sub(
            function_stack_top(state)->current_bb,
            ir_builder_li(function_stack_top(state)->current_bb, 0),
            ret
        );
    }
    return _parse_postfix_expr(state);
}

size_t
_parse_mul_expr(ParseState *state)
{
    size_t ret = _parse_prefix_expr(state);

    int tok = _lex_peak(state);
    while (tok == '*' || tok == '/' || tok == '%') {
        _lex_next(state);
        size_t b_reg = _parse_prefix_expr(state);
        size_t temp_reg =
            tok == '*' ? ir_builder_mul(function_stack_top(state)->current_bb, ret, b_reg) :
            tok == '/' ? ir_builder_div(function_stack_top(state)->current_bb, ret, b_reg) :
                         ir_builder_mod(function_stack_top(state)->current_bb, ret, b_reg);
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
        size_t temp_reg =
            tok == '+' ? ir_builder_add(function_stack_top(state)->current_bb, ret, b_reg) :
                         ir_builder_sub(function_stack_top(state)->current_bb, ret, b_reg);
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
        size_t temp_reg;
        switch (tok) {
            case '<':
                temp_reg = ir_builder_slt(function_stack_top(state)->current_bb, ret, b_reg);
                break;
            case '>':
                temp_reg = ir_builder_slt(function_stack_top(state)->current_bb, b_reg, ret);
                break;
            case TOK_LE:
                temp_reg = ir_builder_sle(function_stack_top(state)->current_bb, ret, b_reg);
                break;
            case TOK_GE:
                temp_reg = ir_builder_sle(function_stack_top(state)->current_bb, b_reg, ret);
                break;
            case TOK_EQ:
                temp_reg = ir_builder_seq(function_stack_top(state)->current_bb, ret, b_reg);
                break;
            case TOK_NE:
                temp_reg = ir_builder_lnot(
                    function_stack_top(state)->current_bb,
                    ir_builder_seq(function_stack_top(state)->current_bb, ret, b_reg)
                );
                break;
            default:
                assert(0);
                return 0;
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

        BasicBlock *expr_end = ir_builder_new_basic_block(
            function_stack_top(state)->builder,
            function_stack_top(state)->current_bb
        );

        while (tok == TOK_AND) {
            _lex_next(state);
            ir_builder_mov_upper(
                function_stack_top(state)->current_bb,
                temp_reg,
                ret
            );
            BasicBlock *next_block = ir_builder_new_basic_block(
                function_stack_top(state)->builder,
                function_stack_top(state)->current_bb
            );
            ir_builder_br(
                function_stack_top(state)->current_bb,
                temp_reg,
                next_block,
                expr_end
            );

            function_stack_top(state)->current_bb = next_block;
            ret = _parse_compare_expr(state);
            tok = _lex_peak(state);
        }

        ir_builder_mov_upper(
            function_stack_top(state)->current_bb,
            temp_reg,
            ret
        );
        ir_builder_j(
            function_stack_top(state)->current_bb,
            expr_end
        );

        function_stack_top(state)->current_bb = expr_end;
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

        BasicBlock *expr_end = ir_builder_new_basic_block(
            function_stack_top(state)->builder,
            function_stack_top(state)->current_bb
        );

        while (tok == TOK_OR) {
            _lex_next(state);
            ir_builder_mov_upper(
                function_stack_top(state)->current_bb,
                temp_reg,
                ret
            );
            BasicBlock *next_block = ir_builder_new_basic_block(
                function_stack_top(state)->builder,
                function_stack_top(state)->current_bb
            );
            ir_builder_br(
                function_stack_top(state)->current_bb,
                temp_reg,
                expr_end,
                next_block
            );
            function_stack_top(state)->current_bb = next_block;

            ret = _parse_logic_and_expr(state);
            tok = _lex_peak(state);
        }

        ir_builder_mov_upper(
            function_stack_top(state)->current_bb,
            temp_reg,
            ret
        );
        ir_builder_j(
            function_stack_top(state)->current_bb,
            expr_end
        );

        function_stack_top(state)->current_bb = expr_end;
        ret = temp_reg;
    }
    return ret;
}

size_t
_parse_right_hand_expr(ParseState *state)
{ return _parse_logic_or_expr(state); }

void
_parse_assignment_expr(ParseState *state)
{
    int tok = _lex_next(state);
    if (
        tok != TOK_ID ||
        !_parse_exists_in_symbol_table(state, state->peaking_value) ||
        _parse_find_in_symbol_table(state, state->peaking_value) < 0
    ) {
        parse_expect_error(state, "identifier", tok);
        return;
    }

    CString *literal = (CString*)state->peaking_value;
    size_t object_reg = (size_t)_parse_find_in_symbol_table(state, state->peaking_value);
    size_t this_reg = (size_t)_parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));
    size_t key_reg = ~0u;
    tok = _lex_peak(state);

    while (tok == '.' || tok == '[' || tok == '(') {
        if (tok == '(') {
            size_t arg_reg = _parse_arguments(state, this_reg);
            size_t temp_reg = ir_builder_call(function_stack_top(state)->current_bb, object_reg, arg_reg);
            object_reg = temp_reg;
            this_reg = (size_t)_parse_find_in_symbol_table(state, string_pool_find_str(state->string_pool, "global"));
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
                key_reg = ir_builder_lstr(function_stack_top(state)->current_bb, string);
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
                size_t temp_reg = ir_builder_get_obj(
                    function_stack_top(state)->current_bb,
                    object_reg,
                    key_reg
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

    size_t current_reg = function_stack_top(state)->builder->register_nr;
    size_t result_reg = _parse_right_hand_expr(state);

    if (key_reg != ~0u) {
        ir_builder_set_obj(
            function_stack_top(state)->current_bb,
            object_reg,
            result_reg,
            key_reg
        );
    }
    else {
        if (current_reg == function_stack_top(state)->builder->register_nr) {
            result_reg = ir_builder_mov(function_stack_top(state)->current_bb, result_reg);
        }

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
            size_t current_reg = function_stack_top(state)->builder->register_nr;
            size_t new_reg = _parse_right_hand_expr(state);
            size_t register_used = function_stack_top(state)->builder->register_nr - current_reg;
            if (!register_used) {
                new_reg = ir_builder_mov(function_stack_top(state)->current_bb, new_reg);
            }
            _parse_define_into_scope(scope_stack_top(state), literal, new_reg);
        }
        else {
            _parse_define_into_scope(
                scope_stack_top(state),
                literal,
                ir_builder_undefined(function_stack_top(state)->current_bb)
            );
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
    (void)tok;

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

    BasicBlock *if_start = function_stack_top(state)->current_bb;

    BasicBlock *then_start_bb = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        if_start
    );

    Hash *newer = hash_new(HASH_MIN_CAPACITY);
    Hash *original = hash_new(HASH_MIN_CAPACITY);

    function_stack_top(state)->current_bb = then_start_bb;
    _parse_push_scope(state);
    _parse_statement(state);
    BasicBlock *then_bb = function_stack_top(state)->current_bb;

    ParseScope *then_scope = scope_stack_top(state);
    list_unlink(&then_scope->_linked);
    hash_for_each(then_scope->upper_table, node) {
        hash_set_and_update(
            original,
            node->key,
            value_from_int(_parse_find_in_symbol_table(state, node->key))
        );
        hash_set_and_update(
            newer,
            node->key,
            node->value
        );
    }
    list_prepend(&function_stack_top(state)->scopes, &then_scope->_linked);
    _parse_join_scope(state);

    BasicBlock *else_start_bb = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        if_start
    );

    ir_builder_br(
        if_start,
        reg,
        then_start_bb,
        else_start_bb
    );

    tok = _lex_peak(state);
    if (tok == TOK_ID && _parse_find_in_symbol_table(state, state->peaking_value) == -R_ELSE) {
        _lex_next(state);
        function_stack_top(state)->current_bb = else_start_bb;
        _parse_push_scope(state);
        _parse_statement(state);
        BasicBlock *else_bb = function_stack_top(state)->current_bb;

        ParseScope *else_scope = scope_stack_top(state);    list_unlink(&else_scope->_linked);
        ParseScope *base_scope = scope_stack_top(state);
        hash_for_each(else_scope->upper_table, node) {
            Value then = hash_find(newer, node->key);
            if (value_is_undefined(then)) {
                ir_builder_mov_upper(
                    then_bb,
                    (size_t)value_to_int(node->value),
                    (size_t)_parse_find_in_symbol_table(state, node->key)
                );
                _parse_insert_into_scope(
                    base_scope,
                    (CString*)node->key,
                    (size_t)value_to_int(node->value)
                );
            }
            else {
                ir_builder_mov_upper(
                    else_bb,
                    (size_t)value_to_int(then),
                    (size_t)value_to_int(node->value)
                );
                hash_set(newer, node->key, value_undefined());
            }
        }

        hash_for_each(newer, node) {
            ir_builder_mov_upper(
                else_bb,
                (size_t)value_to_int(node->value),
                (size_t)_parse_find_in_symbol_table(state, node->key)
            );
        }

        list_prepend(&function_stack_top(state)->scopes, &else_scope->_linked);
        _parse_pop_scope(state);
        else_start_bb = else_bb;
    }
    else {
        hash_for_each(newer, node) {
            size_t inner = (size_t)value_to_int(node->value),
                   outer = (size_t)value_to_int(hash_find(original, node->key));
            if (inner != outer) {
                ir_builder_mov_upper(
                    else_start_bb,
                    inner,
                    outer
                );
            }
        }
    }

    BasicBlock *if_end = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        if_start
    );

    hash_destroy(newer);
    hash_destroy(original);

    ir_builder_j(then_bb, if_end);
    ir_builder_j(else_start_bb, if_end);
    function_stack_top(state)->current_bb = if_end;
}

void
_parse_while_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_WHILE);

    BasicBlock *while_cond_start = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        NULL
    );

    ir_builder_j(function_stack_top(state)->current_bb, while_cond_start);
    function_stack_top(state)->current_bb = while_cond_start;

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

    BasicBlock *while_cond_end = function_stack_top(state)->current_bb;

    BasicBlock *while_body_start = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        while_cond_end
    );

    function_stack_top(state)->current_bb = while_body_start;

    _parse_push_scope(state);
    _parse_statement(state);
    BasicBlock *while_body_end = function_stack_top(state)->current_bb;

    Hash *newer = hash_new(HASH_MIN_CAPACITY);
    ParseScope *inner_scope = scope_stack_top(state);
    list_unlink(&inner_scope->_linked);

    BasicBlock *while_end = ir_builder_new_basic_block(
        function_stack_top(state)->builder,
        while_cond_end
    );

    hash_for_each(inner_scope->upper_table, node) {
        size_t old_reg = (size_t)_parse_find_in_symbol_table(state, node->key);
        hash_set_and_update(
            newer,
            node->key,
            value_from_int((int)old_reg)
        );
        ir_builder_mov_upper(
            while_body_end,
            old_reg,
            (size_t)value_to_int(node->value)
        );
    }

    list_prepend(&function_stack_top(state)->scopes, &inner_scope->_linked);
    _parse_join_scope(state);

    hash_for_each(newer, node) {
        ir_builder_mov_upper(
            while_end,
            (size_t)_parse_find_in_symbol_table(state, node->key),
            (size_t)value_to_int(node->value)
        );
    }

    hash_destroy(newer);

    ir_builder_br(while_cond_end, reg, while_body_start, while_end);
    ir_builder_j(while_body_end, while_cond_start);
    function_stack_top(state)->current_bb = while_end;
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
        ret_reg = ir_builder_undefined(function_stack_top(state)->current_bb);
    }
    ir_builder_ret(function_stack_top(state)->current_bb, ret_reg);

    tok = _lex_next(state);
    if (tok != ';') {
        parse_expect_error(state, "';'", tok);
    }
}

void
_parse_halt_stmt(ParseState *state)
{
    int tok = _lex_next(state);
    assert(tok == TOK_ID);
    assert(_parse_find_in_symbol_table(state, state->peaking_value) == -R_HALT);
    ir_builder_halt(function_stack_top(state)->current_bb);
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
                    case -R_HALT:
                        _parse_halt_stmt(state);
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
    function->register_nr = func_scope->builder->register_nr + 1;
    function->capture_list = func_scope->capture_list;  func_scope->capture_list = NULL;
    function->inst_list = ir_builder_destroy(func_scope->builder);
    func_scope->builder = NULL;
    func_scope->current_bb = NULL;

    return function;
}
