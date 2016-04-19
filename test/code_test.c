//
// Created by c on 4/19/16.
//

#include "CuTest.h"
#include "parse.h"
#include "parse_internal.h"
#include "cvm.h"

#include "inst_output.h"

static inline VMState *
_vm_from_code_snippet(const char *code)
{
    ParseState *state = parse_state_new_from_string(code);
    parse(state);

    inst_list_push(state->inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new_from_parse_state(state);

    parse_state_destroy(state);

    return vm;
}

static Value mock_print(Value value)
{
    return value_undefined();
}

void
code_light_function_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let print;";
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    Value print = cvm_create_light_function(vm, mock_print);
    cvm_register_in_global(vm, print, "print");

    (void)tc;
}

CuSuite *
code_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, code_light_function_test);

    return suite;
}
