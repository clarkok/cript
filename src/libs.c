//
// Created by c on 4/19/16.
//

#include "libs.h"

#define register_function(vm, func, name)   \
    cvm_register_in_global(vm, cvm_create_light_function(vm, func), name)

static Value _lib_print(VMState *vm, Value value)
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
lib_register(VMState *vm)
{
    register_function(vm, _lib_print, "print");
}
