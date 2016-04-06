//
// Created by c on 4/5/16.
//

#include "CuTest.h"
#include "cvm.h"

void cvm_test_basic_insts(CuTest *tc)
{
    InstList *inst_list = cvm_list_new(16);
    inst_list = cvm_list_append(inst_list, cvm_inst_new_i_type(I_LI, 1, 5));
    inst_list = cvm_list_append(inst_list, cvm_inst_new_i_type(I_LI, 2, 10));
    inst_list = cvm_list_append(inst_list, cvm_inst_new_d_type(I_ADD, 3, 1, 2));
    inst_list = cvm_list_append(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 5, value_to_int(vm->regs[1]));
    CuAssertIntEquals(tc, 10, value_to_int(vm->regs[2]));
    CuAssertIntEquals(tc, 15, value_to_int(vm->regs[3]));

    cvm_state_destroy(vm);
}

CuSuite *cvm_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, cvm_test_basic_insts);

    return suite;
}
