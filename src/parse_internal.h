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
};

enum ReservedWord
{
    R_ZERO,
    R_LET = 1,
    R_IF,
    R_ELSE,

    RESERVED_WORD_NR
};

typedef struct ParseScope
{
    LinkedNode _linked;

    size_t register_counter;
    Hash *symbol_table;
    Hash *upper_table;
} ParseScope;

int _lex_peak(ParseState *state);
int _lex_next(ParseState *state);

#define scope_stack_top(state)  (list_get(list_head(&state->scope_stack), ParseScope, _linked))

static inline Value
_parse_symbol_table_lookup(ParseState *state, CString *string)
{
    Value result;
    list_for_each(&state->scope_stack, node) {
        ParseScope *scope = list_get(node, ParseScope, _linked);
        if (!value_is_undefined(result = hash_find(scope->upper_table, (uintptr_t)string))) return result;
        if (!value_is_undefined(result = hash_find(scope->symbol_table, (uintptr_t)string))) return result;
    }
    return value_undefined();
}

static inline ParseScope *
_parse_find_define_scope(ParseState *state, CString *string)
{
    list_for_each(&state->scope_stack, node) {
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
