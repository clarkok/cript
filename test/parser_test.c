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

#define get_reg_value_from_vm(vm, reg)          \
    list_get(list_head(&list_get(list_head(&vm->scenes), VMScene, _linked)->frames), VMFrame, _linked)->regs[reg]

void
parse_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1;\n"
        "let b = 2;\n"
        "a = a + b;\n"
        "b = a - b;\n"
        "a = a - b;\n"
        "return [a, b];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 2, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_complex_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a;\n"
        "a = 1 * 2 / 3 + 4 % (5 + 6);\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1 * 2 / 3 + 4 % (5 + 6), value_to_int(ret_val));

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
        "return [a, b];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 3, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 2, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

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
        "return [a, b];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 0, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 4, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

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
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_if_false_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;"
        "if (0) {\n"
        "  a = 1;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 0, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_if_else_true_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;"
        "if (1) {\n"
        "  a = 1;\n"
        "}\n"
        "else {\n"
        "  a = 2;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_if_else_false_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;\n"
        "if (0) {\n"
        "  a = 1;\n"
        "}\n"
        "else {\n"
        "  a = 2;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 2, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_nested_if_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;\n"
        "if (1) {\n"
        "  a = 1;\n"
        "  if (a) {\n"
        "    a = 2;\n"
        "  }\n"
        "  else a = 3;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 2, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_nested_if_test_buggy1(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 0;\n"
        "if (1) {\n"
        "  a = 1;\n"
        "  if (a) a = 2;\n"
        "  else a = 3;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 2, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_while_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 10;\n"
        "while (a) {\n"
        "  a = a - 1;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 0, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_compare_expr_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 10;\n"
        "while (a > 5) {\n"
        "  a = a - 1;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 5, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_logic_and_expr_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1 && 2 && 3 && 4 && 5 && 6;\n"
        "let b = 5 && 4 && 3 && 2 && 1 && 0;\n"
        "let c = 1 && 2 && 0 && 3 && 4;\n"
        "return [a, b, c];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 6, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 0, value_to_int(hash_find(value_to_ptr(ret_val), 1)));
    CuAssertIntEquals(tc, 0, value_to_int(hash_find(value_to_ptr(ret_val), 2)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_logic_or_expr_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1 || 2 || 3 || 4 || 5 || 6;\n"
        "let b = 0 || 0 || 1 || 2;\n"
        "let c = 0 || 0 || 0;\n"
        "return [a, b, c];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 1)));
    CuAssertIntEquals(tc, 0, value_to_int(hash_find(value_to_ptr(ret_val), 2)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_fibonacci_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1,\n"
        "    b = 1,\n"
        "    n = 10;\n"
        "while (n > 0) {\n"
        "    let t = a + b;\n"
        "    a = b;\n"
        "    b = t;\n"
        "    n = n - 1;\n"
        "}\n"
        "return a;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 89, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_string_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 'hello world';\n"
        "let b = \"hello world!\";\n"
        "return [a, b];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CString *string;

    string = value_to_string(hash_find(value_to_ptr(ret_val), 0));
    CuAssertTrue(tc, string->length);
    CuAssertIntEquals(tc, 0, strncmp("hello world", string->content, string->length));

    string = value_to_string(hash_find(value_to_ptr(ret_val), 1));
    CuAssertTrue(tc, string->length);
    CuAssertIntEquals(tc, 0, strncmp("hello world!", string->content, string->length));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_object_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = {};\n"
        "let b = {\n"
        "  a : 1,\n"
        "  b : 2\n"
        "};\n"
        "let c = {c : 3};\n"
        "let d = b.a;\n"
        "let e = b['b'];\n"
        "return [d, e];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 2, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
    (void)tc;
}

void
parse_object_set_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = {};\n"
        "let b = {\n"
        "  a : 1,\n"
        "  b : 2\n"
        "};\n"
        "let c = {c : 3};\n"
        "c.c = 4;\n"
        "let d = c.c;\n"
        "c['c'] = 5;\n"
        "let e = c.c;\n"
        "return [d, e];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 4, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 5, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_try_gc_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let i = 300000;\n"
        "let old_obj = { a : 0 };\n"
        "let old_arr = [ 0 ];\n"
        "while (i > 0) {\n"
        "  let a = {};\n"
        "  let b = [];\n"
        "  i = i - 1;\n"
        "  old_obj.a = i;\n"
        "  old_arr[0] = i;\n"
        "}\n"
        "halt;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    cvm_state_run(vm);

    cvm_state_destroy(vm);
    parse_state_destroy(state);

    (void)tc;
}

void
parse_array_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = [];\n"
        "let b = [\n"
        "  1,\n"
        "  2\n"
        "];\n"
        "let c = [3 : 1];\n"
        "let d = b[0];\n"
        "let e = c[3];\n"
        "return [d, e];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 1, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
    (void)tc;
}

void
parse_array_set_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = [];\n"
        "let b = [\n"
        "  1,\n"
        "  2\n"
        "];\n"
        "let c = [3 : 1];\n"
        "c[1] = 4;\n"
        "let d = c[1];\n"
        "c[1] = 5;\n"
        "let e = c[1];\n"
        "return [d, e];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 4, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 5, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_nested_block_buggy_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1;\n"
        "let b = 2;\n"
        "{\n"
        "  let c = 3;\n"
        "  {\n"
        "    b = c;\n"
        "    c = 4;\n"
        "  }\n"
        "  a = c;"
        "}\n"
        "let d = b + a;\n"
        "let e = a;\n"
        "return [d, e];\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 7, value_to_int(hash_find(value_to_ptr(ret_val), 0)));
    CuAssertIntEquals(tc, 4, value_to_int(hash_find(value_to_ptr(ret_val), 1)));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_function_parse_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let test_func1 = function (arg1, arg2) {};\n"
        "let test_func2 = function () {};\n"
        "let test_func3 = function () {\n"
        "  let local = 1;\n"
        "  test_func2();\n"
        "  return local;\n"
        "};\n"
        "return test_func3();\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_function_argument_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let test = function (arg) { return arg; };\n"
        "return test(1);\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(ret_val));

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

void
parse_let_stmt_buggy_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1;\n"
        "let b = a;\n"
        "a = a + 1;\n"
        "return b;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    Value ret_val = cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(ret_val));

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
    SUITE_ADD_TEST(suite, parse_if_false_test);
    SUITE_ADD_TEST(suite, parse_if_else_true_test);
    SUITE_ADD_TEST(suite, parse_if_else_false_test);
    SUITE_ADD_TEST(suite, parse_nested_if_test);
    SUITE_ADD_TEST(suite, parse_nested_if_test_buggy1);
    SUITE_ADD_TEST(suite, parse_while_test);
    SUITE_ADD_TEST(suite, parse_compare_expr_test);
    SUITE_ADD_TEST(suite, parse_logic_and_expr_test);
    SUITE_ADD_TEST(suite, parse_logic_or_expr_test);
    SUITE_ADD_TEST(suite, parse_fibonacci_test);
    SUITE_ADD_TEST(suite, parse_string_test);
    SUITE_ADD_TEST(suite, parse_object_test);
    SUITE_ADD_TEST(suite, parse_object_set_test);
    SUITE_ADD_TEST(suite, parse_try_gc_test);
    SUITE_ADD_TEST(suite, parse_array_test);
    SUITE_ADD_TEST(suite, parse_array_set_test);
    SUITE_ADD_TEST(suite, parse_nested_block_buggy_test);
    SUITE_ADD_TEST(suite, parse_function_parse_test);
    SUITE_ADD_TEST(suite, parse_function_argument_test);
    SUITE_ADD_TEST(suite, parse_let_stmt_buggy_test);

    return suite;
}
