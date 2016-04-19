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

    VMState *vm = cvm_state_new_from_parse_state(state);

    parse_state_destroy(state);

    return vm;
}

static Value _mock_print(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);

    for (uintptr_t i = 1; i < hash_size(args); ++i) {
        Value val = hash_find(args, i);

        if (value_is_int(val)) {
            printf("%d ", value_to_int(val));
        }
        else {
            CString *string = value_to_string(val);
            printf("%.*s ", string->length, string->content);
        }
    }

    printf("\n");
    return value_undefined();
}

void
code_light_function_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let print_result = global.print('light function tested', 1, 2, 3);"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    Value print = cvm_create_light_function(vm, _mock_print);
    cvm_register_in_global(vm, print, "print");

    cvm_state_run(vm);
    (void)tc;
}

void
code_left_hand_function_call_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "global.print('left hand function call tested', 2, 3, 4);"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    Value print = cvm_create_light_function(vm, _mock_print);
    cvm_register_in_global(vm, print, "print");

    cvm_state_run(vm);
    (void)tc;
}

void
code_pass_function_around_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let print_func = global.print;\n"
        "print_func('pass function around tested', 3, 4, 5);\n"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    Value print = cvm_create_light_function(vm, _mock_print);
    cvm_register_in_global(vm, print, "print");

    cvm_state_run(vm);
    (void)tc;
}

CuSuite *
code_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, code_light_function_test);
    SUITE_ADD_TEST(suite, code_left_hand_function_call_test);
    SUITE_ADD_TEST(suite, code_pass_function_around_test);

    return suite;
}