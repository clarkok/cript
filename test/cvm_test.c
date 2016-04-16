//
// Created by c on 4/5/16.
//

#include "CuTest.h"
#include "cvm.h"

void
cvm_test_basic_insts(CuTest *tc)
{
    InstList *inst_list = cvm_list_new(16);

    cvm_list_push(inst_list, cvm_inst_new_i_type(I_LI, 1, 5));
    cvm_list_push(inst_list, cvm_inst_new_i_type(I_LI, 2, 10));
    cvm_list_push(inst_list, cvm_inst_new_d_type(I_ADD, 3, 1, 2));
    cvm_list_push(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 5, value_to_int(cvm_get_register(vm, 1)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 2)));
    CuAssertIntEquals(tc, 15, value_to_int(cvm_get_register(vm, 3)));

    cvm_state_destroy(vm);
}

void
cvm_test_branch_insts(CuTest *tc)
{
    InstList *inst_list = cvm_list_new(16);

    cvm_list_push(inst_list, cvm_inst_new_i_type(I_LI, 1, 1));
    cvm_list_push(inst_list, cvm_inst_new_i_type(I_LI, 2, 10));
    cvm_list_push(inst_list, cvm_inst_new_i_type(I_LI, 3, 0));

    cvm_list_push(inst_list, cvm_inst_new_i_type(I_J, 0, 5));
    cvm_list_push(inst_list, cvm_inst_new_d_type(I_ADD, 3, 3, 1));
    cvm_list_push(inst_list, cvm_inst_new_d_type(I_SLT, 4, 2, 3));
    cvm_list_push(inst_list, cvm_inst_new_i_type(I_BR, 4, -3));

    cvm_list_push(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(cvm_get_register(vm, 1)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 2)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 3)));

    cvm_state_destroy(vm);
}

CuSuite *
cvm_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, cvm_test_basic_insts);
    SUITE_ADD_TEST(suite, cvm_test_branch_insts);

    return suite;
}
