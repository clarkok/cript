//
// Created by c on 4/21/16.
//

#include "CuTest.h"
#include "parse.h"
#include "cvm.h"
#include "inst_output.h"

void
ir_builder_constant_sharing_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let a = 1 + 1 + 1;\n"
    ;

    ParseState *state = parse_state_new_from_string(TEST_CONTENT);
    parse(state);

    VMState *vm = cvm_state_new_from_parse_state(state);
    output_vm_state(stdout, vm);
    cvm_state_run(vm);

    cvm_state_destroy(vm);
    parse_state_destroy(state);
}

CuSuite *
ir_builder_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, ir_builder_constant_sharing_test);

    return suite;
}
