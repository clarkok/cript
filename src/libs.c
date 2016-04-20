//
// Created by c on 4/19/16.
//

#include <math.h>

#include "libs.h"

#define register_function(vm, func, name)   \
    cvm_register_in_global(vm, cvm_create_light_function(vm, func), name)

static Value
_lib_print(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);

    for (uintptr_t i = 1; i < hash_size(args); ++i) {
        Value val = hash_find(args, i);

        if (value_is_int(val)) {
            printf("%d", value_to_int(val));
        }
        else {
            CString *string = value_to_string(val);
            printf("%.*s", string->length, string->content);
        }
    }

    return value_undefined();
}

static Value
_lib_println(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);

    for (uintptr_t i = 1; i < hash_size(args); ++i) {
        Value val = hash_find(args, i);

        if (value_is_int(val)) {
            printf("%d", value_to_int(val));
        }
        else {
            CString *string = value_to_string(val);
            printf("%.*s", string->length, string->content);
        }
    }
    printf("\n");

    return value_undefined();
}

static Value
_lib_foreach(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);
    Hash *array = value_to_ptr(hash_find(args, 1));
    Value callback = hash_find(args, 2);

    for (size_t i = 0; i < hash_size(array); ++i) {
        Hash *cb_arg = cvm_create_array(vm, HASH_MIN_CAPACITY);

        cb_arg = cvm_set_hash(vm, cb_arg, 0, value_from_ptr(cvm_get_global(vm)));
        cb_arg = cvm_set_hash(vm, cb_arg, 1, hash_find(array, i));

        cvm_state_call_function(vm, callback, value_from_ptr(cb_arg));
    }

    return value_undefined();
}

static Value
_lib_random(VMState *vm, Value value)
{ return value_from_int(random()); }

static Value
_lib_import(VMState *vm, Value value)
{
    Hash *args = value_to_ptr(value);
    CString *path = value_to_string(hash_find(args, 1));
    ParseState *state = parse_state_expand_from_file(vm, path->content);
    parse(state);
    Value ret = cvm_state_import_from_parse_state(vm, state);
    parse_state_destroy(state);
    return ret;
}

void
lib_register(VMState *vm)
{
    register_function(vm, _lib_print, "print");
    register_function(vm, _lib_println, "println");
    register_function(vm, _lib_foreach, "foreach");
    register_function(vm, _lib_random, "random");
    register_function(vm, _lib_import, "import");
}
