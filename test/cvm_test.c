//
// Created by c on 4/5/16.
//

#include "CuTest.h"
#include "cvm.h"

void
cvm_test_basic_insts(CuTest *tc)
{
    InstList *inst_list = inst_list_new(16);

    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 1, 5));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 2, 10));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_ADD, 3, 1, 2));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list, NULL);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 5, value_to_int(cvm_get_register(vm, 1)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 2)));
    CuAssertIntEquals(tc, 15, value_to_int(cvm_get_register(vm, 3)));

    cvm_state_destroy(vm);
}

void
cvm_test_branch_insts(CuTest *tc)
{
    InstList *inst_list = inst_list_new(16);

    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 1, 1));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 2, 10));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 3, 0));

    inst_list_push(inst_list, cvm_inst_new_i_type(I_J, 0, 5));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_ADD, 3, 3, 1));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_SLE, 4, 2, 3));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_BNR, 4, -3));

    inst_list_push(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list, NULL);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 1, value_to_int(cvm_get_register(vm, 1)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 2)));
    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 3)));

    cvm_state_destroy(vm);
}

void
cvm_register_zero_should_be_zero_test(CuTest *tc)
{
    VMState *vm = cvm_state_new(NULL, NULL);

    CuAssertIntEquals(tc, 0, value_to_int(cvm_get_register(vm, 0)));

    cvm_state_destroy(vm);
}

void
cvm_object_test(CuTest *tc)
{
    InstList *inst_list = inst_list_new(16);
    StringPool *string_pool = string_pool_new();

    CString *test_key = string_pool_insert_str(&string_pool, "_test_key_");

    inst_list_push(inst_list, cvm_inst_new_d_type(I_NEW_OBJ, 1, 0, 0));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_LSTR, 2, (intptr_t)test_key));
    inst_list_push(inst_list, cvm_inst_new_i_type(I_LI, 3, 10));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_SET_OBJ, 1, 3, 2));
    inst_list_push(inst_list, cvm_inst_new_d_type(I_GET_OBJ, 4, 1, 2));

    inst_list_push(inst_list, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(inst_list, string_pool);
    cvm_state_run(vm);

    CuAssertIntEquals(tc, 10, value_to_int(cvm_get_register(vm, 4)));

    cvm_state_destroy(vm);
}

CuSuite *
cvm_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, cvm_test_basic_insts);
    SUITE_ADD_TEST(suite, cvm_test_branch_insts);
    SUITE_ADD_TEST(suite, cvm_register_zero_should_be_zero_test);
    SUITE_ADD_TEST(suite, cvm_object_test);

    return suite;
}
