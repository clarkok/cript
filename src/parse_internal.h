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

    RESERVED_WORD_NR
};

typedef struct ParseScope
{
    LinkedNode _linked;

    size_t register_count;
    Hash *symbol_table;
} ParseScope;

int _lex_peak(ParseState *state);
int _lex_next(ParseState *state);

#define scope_stack_top(state)  (list_get(list_head(&state->scope_stack), ParseScope, _linked))

static inline Value
_parse_symbol_table_lookup(ParseState *state, CString *string)
{
    Value result = value_undefined();
    list_for_each(&state->scope_stack, node) {
        ParseScope *scope = list_get(node, ParseScope, _linked);
        result = hash_find(scope->symbol_table, (uintptr_t)string);
        if (!value_is_undefined(result)) { break; }
    }
    return result;
}

#define _parse_exists_in_symbol_table(state, string)    \
    !value_is_undefined(_parse_symbol_table_lookup(state, (CString*)string))

#define _parse_find_in_symbol_table(state, string)      \
    value_to_int(_parse_symbol_table_lookup(state, (CString*)string))

#endif //CRIPT_PARSE_INTERNAL_H
