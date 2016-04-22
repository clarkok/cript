//
// Created by c on 4/19/16.
//

#include <math.h>

#include "error.h"
#include "libs.h"

#define register_function(vm, func, name)   \
    cvm_register_in_global(vm, cvm_create_light_function(vm, func), name)

#define register_value(vm, value, name)     \
    cvm_register_in_global(vm, value, name)

static inline Hash *
_lib_get_hash_from_value(Value val)
{
    Hash *ret = value_to_ptr(val);
    while (ret->type == HT_REDIRECT) {
        ret = (Hash*)ret->size;
    }
    return ret;
}

static Value
_lib_print(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);

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
    Hash *args = _lib_get_hash_from_value(value);

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
    Hash *args = _lib_get_hash_from_value(value);
    Hash *array = _lib_get_hash_from_value(hash_find(args, 1));
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
    Hash *args = _lib_get_hash_from_value(value);
    CString *path = value_to_string(hash_find(args, 1));
    ParseState *state = parse_state_expand_from_file(vm, path->content);
    parse(state);
    Value ret = cvm_state_import_from_parse_state(vm, state);
    parse_state_destroy(state);
    return ret;
}

static Value
_lib_concat(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);

    char buffer[256],
        *ptr = buffer;

    for (size_t i = 1; i < hash_size(args); ++i) {
        CString *string = value_to_string(hash_find(args, i));
        strncpy(ptr, string->content, 255 - (ptr - buffer));
        ptr += string->length;
        if (ptr - buffer >= 255) {
            ptr = buffer + 255;
            break;
        }
    }

    return value_from_string(string_pool_insert_vec(&vm->string_pool, buffer, ptr - buffer));
}

static Value
_lib_typeof(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    Value query_val = hash_find(args, 1);

    if (value_is_int(query_val)) {
        return cvm_get_cstring_value(vm, "integer");
    }
    else if (value_is_string(query_val)) {
        return cvm_get_cstring_value(vm, "string");
    }
    else if (value_is_null(query_val)) {
        return cvm_get_cstring_value(vm, "null");
    }
    else if (value_is_undefined(query_val)) {
        return cvm_get_cstring_value(vm, "undefined");
    }
    else {
        Hash *hash = _lib_get_hash_from_value(query_val);
        while (hash->type == HT_REDIRECT) hash = (Hash*)hash->size;
        assert(hash->type != HT_GC_LEFT);

        switch (hash->type) {
            case HT_OBJECT:         return cvm_get_cstring_value(vm, "object");
            case HT_ARRAY:          return cvm_get_cstring_value(vm, "array");
            case HT_LIGHTFUNC:      return cvm_get_cstring_value(vm, "light_function");
            case HT_CLOSURE:        return cvm_get_cstring_value(vm, "closure");
            case HT_USERDATA:       return cvm_get_cstring_value(vm, "userdata");
            default: assert(0);     return value_undefined();
        }
    }
}

static Value
_lib_sizeof(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    Value query_val = hash_find(args, 1);

    if (value_is_string(query_val)) {
        CString *string = value_to_string(query_val);
        return value_from_int(string->length);
    }
    else if (value_is_ptr(query_val)) {
        Hash *hash = _lib_get_hash_from_value(query_val);
        while (hash->type == HT_REDIRECT) hash = (Hash*)hash->size;
        assert(hash->type != HT_GC_LEFT);

        switch (hash->type) {
            case HT_OBJECT:
            case HT_ARRAY:
                return value_from_int(hash_size(hash));
            case HT_LIGHTFUNC:
            case HT_CLOSURE:
            case HT_USERDATA:
                return value_from_int(-1);
            default: assert(0);
        }
    }

    return value_from_int(-1);
}

static Value
_lib_to_string(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    Value query_val = hash_find(args, 1);

    if (value_is_int(query_val)) {
        char buffer[30];
        sprintf(buffer, "%d", value_to_int(query_val));
        return cvm_get_cstring_value(vm, buffer);
    }
    else {
        return _lib_typeof(vm, value);
    }
}

static Value
_lib_parse_number(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    CString *string = value_to_string(hash_find(args, 1));
    int ret_val;
    sscanf(string->content, "%d", &ret_val);
    return value_from_int(ret_val);
}

static Value
_lib_char_at(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    CString *string = value_to_string(hash_find(args, 1));
    int index = value_to_int(hash_find(args, 2));

    if (index < 0 || index >= string->length) {
        error_f("Out of range when get char_at(%.*s, %d)", string->length, string->content, index);
        return value_undefined();
    }

    char buffer[2];
    buffer[0] = string->content[index];
    buffer[1] = 0;

    return cvm_get_cstring_value(vm, buffer);
}

static Value
_lib_char_code_at(VMState *vm, Value value)
{
    Hash *args = _lib_get_hash_from_value(value);
    CString *string = value_to_string(hash_find(args, 1));
    int index = value_to_int(hash_find(args, 2));

    if (index < 0 || index >= string->length) {
        error_f("Out of range when get char_at(%.*s, %d)", string->length, string->content, index);
        return value_undefined();
    }

    return value_from_int(string->content[index]);
}

void
lib_register(VMState *vm)
{
    register_value(vm, value_null(), "null");
    register_value(vm, value_undefined(), "undefined");

    register_function(vm, _lib_char_at, "char_at");
    register_function(vm, _lib_char_code_at, "char_code_at");
    register_function(vm, _lib_concat, "concat");
    register_function(vm, _lib_foreach, "foreach");
    register_function(vm, _lib_import, "import");
    register_function(vm, _lib_parse_number, "parse_number");
    register_function(vm, _lib_print, "print");
    register_function(vm, _lib_println, "println");
    register_function(vm, _lib_random, "random");
    register_function(vm, _lib_sizeof, "sizeof");
    register_function(vm, _lib_to_string, "to_string");
    register_function(vm, _lib_typeof, "typeof");
}
