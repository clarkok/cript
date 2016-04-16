//
// Created by c on 4/16/16.
//

#include <ctype.h>

#include "parse.h"
#include "parse_internal.h"

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
                state->peaking_token = state->current[-1];
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
                state->peaking_token = state->current[-1];
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
parse_state_from_string(const char *content)
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

    return state;
}

void
parse_state_destroy(ParseState *state)
{
    if (state->string_pool) { string_pool_destroy(state->string_pool); }
    if (state->inst_list)   { inst_list_destroy(state->inst_list); }
    free(state);
}
