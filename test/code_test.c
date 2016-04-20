//
// Created by c on 4/19/16.
//

#include "CuTest.h"
#include "parse.h"
#include "cvm.h"
#include "math.h"

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

static Value _random(VMState *vm, Value value)
{ return value_from_int(random()); }

static Value _assert(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);
    if (!value_to_int(hash_find(args, 1))) {
        printf("C Assert Failed!\n");
        __builtin_trap();
    }
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

void
code_sort_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let sort = function (array, len) {\n"
        "  let i = 0,\n"
        "      j = 0;\n"
        "  while (i < len) {\n"
        "    j = i + 1;\n"
        "    while (j < len) {\n"
        "      if (array[i] > array[j]) {\n"
        "        let t = array[i];\n"
        "        array[i] = array[j];\n"
        "        array[j] = t;\n"
        "      }\n"
        "      j = j + 1;\n"
        "    }\n"
        "    i = i + 1;\n"
        "  }\n"
        "};\n"
        "\n"
        "let i = 0;\n"
        "let array = [];\n"
        "while (i < 10) {\n"
        "  array[i] = global.random() % 10 + 10;\n"
        "  i = i + 1;\n"
        "}\n"
        "sort(array, 10);\n"
        "i = 0;\n"
        "while (i < 9) {\n"
        "  global.assert(array[i] <= array[i+1]);\n"
        "  i = i + 1;\n"
        "}\n"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    cvm_register_in_global(vm, cvm_create_light_function(vm, _mock_print), "print");
    cvm_register_in_global(vm, cvm_create_light_function(vm, _random), "random");
    cvm_register_in_global(vm, cvm_create_light_function(vm, _assert), "assert");

    cvm_state_run(vm);
    (void)tc;
}

void
code_closure_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let create_closure = function (num) {\n"
        "  let ret = [];\n"
        "  let i = 0;\n"
        "  while (i < num) {\n"
        "    ret[i] = function () {\n"
        "      return i;\n"
        "    };\n"
        "    i = i + 1;\n"
        "  }\n"
        "  return ret;\n"
        "};\n"
        "let closures = create_closure(10);\n"
        "let i = 0;\n"
        "while (i < 10) {\n"
        "  global.assert(closures[i]() == i);\n"
        "  i = i + 1;\n"
        "}\n"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    cvm_register_in_global(vm, cvm_create_light_function(vm, _mock_print), "print");
    cvm_register_in_global(vm, cvm_create_light_function(vm, _random), "random");
    cvm_register_in_global(vm, cvm_create_light_function(vm, _assert), "assert");

    cvm_state_run(vm);
    (void)tc;
}

typedef struct LongString
{
    size_t length;
    char content[0];
} LongString;

static void
_long_string_destructor(void *ptr)
{
    LongString *long_str = ptr;
    printf("long string destructed: %.*ss\n", long_str->length, long_str->content);
    free(ptr);
}

static Value _new_long_string(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);
    CString *string = value_to_string(hash_find(args, 1));
    LongString *long_string = malloc(sizeof(LongString) + string->length + 1);
    long_string->length = string->length;
    strncpy(long_string->content, string->content, string->length);

    return cvm_create_userdata(vm, long_string, _long_string_destructor);
}

void
code_userdata_test(CuTest *tc)
{
    static const char TEST_CONTENT[] =
        "let test_func = function () {\n"
        "  let t = global.new_long_string('test_long_string');\n"
        "};\n"
        "test_func();\n"
        "let i = 0;\n"
        "while (i < 300000) {\n"
        "  let a = {};\n"
        "  i = i + 1;\n"
        "}\n"
    ;

    VMState *vm = _vm_from_code_snippet(TEST_CONTENT);

    cvm_register_in_global(vm, cvm_create_light_function(vm, _new_long_string), "new_long_string");

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
    SUITE_ADD_TEST(suite, code_sort_test);
    SUITE_ADD_TEST(suite, code_closure_test);
    SUITE_ADD_TEST(suite, code_userdata_test);

    return suite;
}
