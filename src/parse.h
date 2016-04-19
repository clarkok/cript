//
// Created by c on 4/16/16.
//

#ifndef CRIPT_PARSE_H
#define CRIPT_PARSE_H

#include "list.h"

#include "inst_list.h"
#include "string_pool.h"
#include "hash.h"

typedef struct VMFunction VMFunction;

typedef struct ParseState
{
    const char *filename;
    int line;
    int column;

    const char *content;
    size_t content_length;

    StringPool *string_pool;

    const char *current;
    const char *limit;

    int peaking_token;
    intptr_t peaking_value;

    LinkedList functions;

    LinkedList function_stack;
} ParseState;

ParseState *parse_state_new_from_string(const char *content);
void parse_state_destroy(ParseState *state);

VMFunction *parse_get_main_function(ParseState *state);

void parse(ParseState *state);

#endif //CRIPT_PARSE_H
