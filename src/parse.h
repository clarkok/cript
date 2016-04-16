//
// Created by c on 4/16/16.
//

#ifndef CRIPT_PARSE_H
#define CRIPT_PARSE_H

#include "inst_list.h"
#include "string_pool.h"

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
} ParseState;

#endif //CRIPT_PARSE_H
