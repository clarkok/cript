//
// Created by c on 4/16/16.
//

#include "CuTest.h"
#include "parse.h"
#include "parse_internal.h"

void
lex_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "hello world\n"
        "Can you hear me?\n"
        "/* this is a block comment \n"
        " * and the comment will end here */\n"
        "the number is 42\n"
        "i am a line with a // line comment \n"
        "the code ends here\n";

    ParseState *state = parse_state_from_string(TEST_CONTENT);

    CuAssertIntEquals(tc, TOK_ID, _lex_peak(state));
    CuAssertIntEquals(tc, 0, state->line);
    CuAssertIntEquals(tc, 5, state->column);
    CString *literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("hello", literal->content, strlen("hello")));
    _lex_next(state);

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 0, state->line);
    CuAssertIntEquals(tc, 11, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("world", literal->content, strlen("world")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 1, state->line);
    CuAssertIntEquals(tc, 3, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("Can", literal->content, strlen("Can")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 1, state->line);
    CuAssertIntEquals(tc, 7, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("you", literal->content, strlen("you")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 1, state->line);
    CuAssertIntEquals(tc, 12, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("hear", literal->content, strlen("hear")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 1, state->line);
    CuAssertIntEquals(tc, 15, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("me", literal->content, strlen("me")));

    CuAssertIntEquals(tc, '?', _lex_next(state));
    CuAssertIntEquals(tc, 1, state->line);
    CuAssertIntEquals(tc, 16, state->column);

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 4, state->line);
    CuAssertIntEquals(tc, 3, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("the", literal->content, strlen("the")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 4, state->line);
    CuAssertIntEquals(tc, 10, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("number", literal->content, strlen("number")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 4, state->line);
    CuAssertIntEquals(tc, 13, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("is", literal->content, strlen("is")));

    CuAssertIntEquals(tc, TOK_NUM, _lex_next(state));
    CuAssertIntEquals(tc, 4, state->line);
    CuAssertIntEquals(tc, 16, state->column);
    CuAssertIntEquals(tc, 42, state->peaking_value);

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 1, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("i", literal->content, strlen("i")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 4, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("am", literal->content, strlen("am")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 6, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("a", literal->content, strlen("a")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 11, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("line", literal->content, strlen("line")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 16, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("with", literal->content, strlen("with")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 5, state->line);
    CuAssertIntEquals(tc, 18, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("a", literal->content, strlen("a")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 6, state->line);
    CuAssertIntEquals(tc, 3, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("the", literal->content, strlen("the")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 6, state->line);
    CuAssertIntEquals(tc, 8, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("code", literal->content, strlen("code")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 6, state->line);
    CuAssertIntEquals(tc, 13, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("ends", literal->content, strlen("ends")));

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 6, state->line);
    CuAssertIntEquals(tc, 18, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("here", literal->content, strlen("here")));

    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));

    parse_state_destroy(state);
}

CuSuite *
parse_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, lex_test);

    return suite;
}
