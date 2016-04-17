//
// Created by c on 4/16/16.
//

#include "CuTest.h"
#include "parse.h"
#include "parse_internal.h"
#include "cvm.h"

#include "inst_output.h"

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
        "the code ends here\n"
        "single /\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);

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

    CuAssertIntEquals(tc, TOK_ID, _lex_next(state));
    CuAssertIntEquals(tc, 7, state->line);
    CuAssertIntEquals(tc, 6, state->column);
    literal = (CString*)state->peaking_value;
    CuAssertIntEquals(tc, 0, strncmp("single", literal->content, strlen("single")));

    CuAssertIntEquals(tc, '/', _lex_next(state));
    CuAssertIntEquals(tc, 7, state->line);
    CuAssertIntEquals(tc, 8, state->column);

    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));
    CuAssertIntEquals(tc, TOK_EOF, _lex_next(state));

    parse_state_destroy(state);
}

#define get_reg_from_parse_state(state, name)   \
    _parse_find_in_symbol_table(state, (uintptr_t)string_pool_find_str(state->string_pool, name))

#define get_reg_value_from_vm(vm, reg)  \
    vm->regs[reg]

void
parse_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1;\n"
        "let b = 2;\n"
        "a = a + b;\n"
        "b = a - b;\n"
        "a = a - b;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    intptr_t reg_a = get_reg_from_parse_state(state, "a");
    intptr_t reg_b = get_reg_from_parse_state(state, "b");

    VMState *vm = cvm_state_new_from_parse_state(state);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 2, value_to_int(get_reg_value_from_vm(vm, reg_a)));
    CuAssertIntEquals(tc, 1, value_to_int(get_reg_value_from_vm(vm, reg_b)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_complex_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a;\n"
        "a = 1 * 2 / 3 + 4 % (5 + 6);\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    intptr_t reg_a = get_reg_from_parse_state(state, "a");

    VMState *vm = cvm_state_new_from_parse_state(state);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 1 * 2 / 3 + 4 % (5 + 6), value_to_int(get_reg_value_from_vm(vm, reg_a)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_block_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;\n"
        "let b = 0;\n"
        "{\n"
        "  let a = 1;\n"
        "  b = a + 1;\n"
        "}\n"
        "a = b + 1;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    intptr_t reg_a = get_reg_from_parse_state(state, "a");
    intptr_t reg_b = get_reg_from_parse_state(state, "b");

    VMState *vm = cvm_state_new_from_parse_state(state);

    cvm_state_run(vm);

    CuAssertIntEquals(tc, 3, value_to_int(get_reg_value_from_vm(vm, reg_a)));
    CuAssertIntEquals(tc, 2, value_to_int(get_reg_value_from_vm(vm, reg_b)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_nested_block_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;\n"
        "let b = 0;\n"
        "{\n"
        "  let a = 1;\n"
        "  b = b + a;\n"
        "  {\n"
        "    let a = 2;\n"
        "    b = b + a;\n"
        "  }\n"
        "  b = b + a;\n"
        "}\n"
        "b = b + a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    intptr_t reg_a = get_reg_from_parse_state(state, "a");
    intptr_t reg_b = get_reg_from_parse_state(state, "b");

    VMState *vm = cvm_state_new_from_parse_state(state);

    cvm_state_run(vm);

    CuAssertIntEquals(tc, 0, value_to_int(get_reg_value_from_vm(vm, reg_a)));
    CuAssertIntEquals(tc, 4, value_to_int(get_reg_value_from_vm(vm, reg_b)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_if_true_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;"
        "if (1) {\n"
        "  a = 1;\n"
        "}\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));
    output_inst_list(stdout, state->inst_list);

    intptr_t reg_a = get_reg_from_parse_state(state, "a");

    VMState *vm = cvm_state_new_from_parse_state(state);

    cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(get_reg_value_from_vm(vm, reg_a)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

CuSuite *
parse_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, lex_test);
    SUITE_ADD_TEST(suite, parse_test);
    SUITE_ADD_TEST(suite, parse_complex_test);
    SUITE_ADD_TEST(suite, parse_block_test);
    SUITE_ADD_TEST(suite, parse_nested_block_test);
    SUITE_ADD_TEST(suite, parse_if_true_test);

    return suite;
}
