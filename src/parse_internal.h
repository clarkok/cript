//
// Created by c on 4/16/16.
//

#ifndef CRIPT_PARSE_INTERNAL_H
#define CRIPT_PARSE_INTERNAL_H

enum Token
{
    TOK_EOF = -1,
    TOK_UNKNOWN = 0,
    TOK_ID = 256,
    TOK_NUM,
    TOK_STRING,
};

int _lex_peak(ParseState *state);
int _lex_next(ParseState *state);

#endif //CRIPT_PARSE_INTERNAL_H
