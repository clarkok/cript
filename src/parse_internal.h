//
// Created by c on 4/16/16.
//

#ifndef CRIPT_PARSE_INTERNAL_H
#define CRIPT_PARSE_INTERNAL_H

#include "list.h"

enum Token
{
    TOK_EOF = -1,
    TOK_UNKNOWN = 0,
    TOK_ID = 256,
    TOK_NUM,
    TOK_STRING,

    TOK_GE,
    TOK_LE,
    TOK_EQ,
    TOK_NE,

    TOK_AND,
    TOK_OR,

    TOK_SHL,
    TOK_SHR,
};

enum ReservedWord
{
    R_ZERO,
    R_LET = 1,
    R_IF,
    R_ELSE,
    R_WHILE,

    RESERVED_WORD_NR
};

typedef struct ParseScope
{
    LinkedNode _linked;

    size_t register_counter;
    Hash *symbol_table;
    Hash *upper_table;
} ParseScope;

typedef struct FunctionScope
{
    LinkedNode _linked;

    LinkedList scopes;
    size_t arguments_nr;
    size_t register_nr;
    Hash *capture_list;
    InstList *inst_list;
} FunctionScope;

int _lex_peak(ParseState *state);
int _lex_next(ParseState *state);

#define function_stack_top(state)   (list_get(list_head(&state->function_stack), FunctionScope, _linked))
#define scope_stack_top(state)      (list_get(list_head(&function_stack_top(state)->scopes), ParseScope, _linked))

static inline Value
_parse_capture_variable(ParseState *state, ParseScope *scope, CString *string)
{
    FunctionScope *function = container_of(list_from_node(&scope->_linked), FunctionScope, scopes);
    intptr_t reg = 0;
    Value result;
    if (!value_is_undefined(result = hash_find(scope->symbol_table, (uintptr_t)string))) {
        reg = value_to_int(result);
    }
    else {
        reg = value_to_int(hash_find(scope->upper_table, (uintptr_t)string));
    }

    if (reg < 0) { return value_from_int(reg); }

    function = list_get(list_prev(&function->_linked), FunctionScope, _linked);
    while (function) {
        size_t new_reg = hash_size(function->capture_list);
        hash_set_and_update(function->capture_list, new_reg, value_from_int(reg));

        ParseScope *base_scope = list_get(list_tail(&function->scopes), ParseScope, _linked);
        hash_set_and_update(
            base_scope->symbol_table,
            (uintptr_t)string,
            value_from_int(new_reg)
        );

        ParseScope *top_scope = list_get(list_head(&function->scopes), ParseScope, _linked);
        if (top_scope != base_scope) {
            hash_set_and_update(
                top_scope->upper_table,
                (uintptr_t)string,
                value_from_int(new_reg)
            );
        }

        reg = new_reg;
        function = list_get(list_prev(&function->_linked), FunctionScope, _linked);
    }

    return value_from_int(reg);
}

static inline Value
_parse_symbol_table_lookup(ParseState *state, CString *string)
{
    Value result;
    list_for_each(&state->function_stack, function_node) {
        FunctionScope *function = list_get(function_node, FunctionScope, _linked);
        list_for_each(&function->scopes, node) {
            ParseScope *scope = list_get(node, ParseScope, _linked);
            if (!value_is_undefined(result = hash_find(scope->symbol_table, (uintptr_t)string))) {
                if (function != function_stack_top(state)) {
                    return _parse_capture_variable(state, scope, string);
                }
                else {
                    return result;
                }
            }
            if (!value_is_undefined(result = hash_find(scope->upper_table, (uintptr_t)string))) {
                if (function != function_stack_top(state)) {
                    return _parse_capture_variable(state, scope, string);
                }
                else {
                    return result;
                }
            }
        }
    }
    return value_undefined();
}

static inline ParseScope *
_parse_find_define_scope(ParseState *state, CString *string)
{
    FunctionScope *function = function_stack_top(state);
    list_for_each(&function->scopes, node) {
        ParseScope *scope = list_get(node, ParseScope, _linked);
        if (!value_is_undefined(hash_find(scope->symbol_table, (uintptr_t)string))) return scope;
    }
    return NULL;
}

#define _parse_exists_in_symbol_table(state, string)    \
    !value_is_undefined(_parse_symbol_table_lookup(state, (CString*)string))

#define _parse_find_in_symbol_table(state, string)      \
    value_to_int(_parse_symbol_table_lookup(state, (CString*)string))

#define _parse_symbol_table_lookup_current(state, string)   \
    hash_find(scope_stack_top(state)->symbol_table, (uintptr_t)string)

#define _parse_exists_in_symbol_table_current(state, string)    \
    !value_is_undefined(_parse_symbol_table_lookup_current(state, (CString*)string))

#endif //CRIPT_PARSE_INTERNAL_H
