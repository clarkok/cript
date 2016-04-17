//
// Created by c on 4/16/16.
//

#ifndef CRIPT_PARSE_H
#define CRIPT_PARSE_H

#include "inst_list.h"
#include "string_pool.h"
#include "hash.h"

typedef struct ParseState
{
    const char *filename;
    int line;
    int column;

    const char *content;
    size_t content_length;

    StringPool *string_pool;
    InstList *inst_list;

    const char *current;
    const char *limit;

    int peaking_token;
    intptr_t peaking_value;

    size_t register_count;
    Hash *symbol_table;
} ParseState;

ParseState *parse_state_new_from_string(const char *content);
void parse_state_destroy(ParseState *state);

void parse(ParseState *state);

#endif //CRIPT_PARSE_H
