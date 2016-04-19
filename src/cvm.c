//
// Created by c on 4/4/16.
//

#include <stdlib.h>

#include "cvm.h"
#include "error.h"
#include "young_gen.h"
#include "hash_internal.h"

#define type_error(vm, expected, actual)            \
    error_f("TypeError: expected %s, but met %s\n", \
        hash_type_to_str(expected),                 \
        hash_type_to_str(actual))

inline void
cvm_set_register(VMState *vm, unsigned int reg, Value value)
{ if (reg) { vm->regs[reg] = value; } }

inline Value
cvm_get_register(VMState *vm, unsigned int reg_id)
{ return vm->regs[reg_id]; }

static inline Hash *
_cvm_allocate_new_hash(VMState *vm, size_t capacity, int type)
{
    Hash *hash = young_gen_new_hash(vm->young_gen, capacity, type);
    if (!hash) {
        cvm_young_gc(vm);
        hash = young_gen_new_hash(vm->young_gen, capacity, type);
        if (!hash) {
            error_f("Out of memory when allocating %s of capacity %d",
                    hash_type_to_str(type),
                    capacity
            );
        }
    }
    return hash;
}

static inline Hash *
_cvm_allocate_new_object(VMState *vm, size_t capacity)
{ return _cvm_allocate_new_hash(vm, capacity, HT_OBJECT); }

static inline Hash *
_cvm_allocate_new_array(VMState *vm, size_t capacity)
{ return _cvm_allocate_new_hash(vm, capacity, HT_ARRAY); }

static inline Hash *
_cvm_get_hash_in_register(VMState *vm, size_t reg)
{
    Hash *hash = value_to_ptr(cvm_get_register(vm, reg));
    if (hash->type == HT_REDIRECT) {
        while(hash->type == HT_REDIRECT) {
            hash = (Hash*)(hash->size);
        }
        cvm_set_register(vm, reg, value_from_ptr(hash));
    }
    return hash;
}

static inline void
_cvm_set_hash(VMState *vm, size_t reg, uintptr_t key, Value val)
{
    Hash *obj = _cvm_get_hash_in_register(vm, reg);
    hash_set(obj, key, val);
    if (hash_need_expand(obj)) {
        Hash *new_obj = _cvm_allocate_new_hash(vm, _hash_expand_size(obj), obj->type);

        obj = _cvm_get_hash_in_register(vm, reg);
        hash_rehash(new_obj, obj);

        obj->type = HT_REDIRECT;
        obj->capacity = 0;
        obj->size = (size_t)new_obj;
    }
}

VMState *
cvm_state_new(InstList *inst_list, StringPool *string_pool)
{
    VMState *vm = malloc(sizeof(VMState));
    if (!vm) return NULL;

    vm->string_pool = string_pool;
    vm->young_gen = young_gen_new();
    vm->inst_list = inst_list;
    vm->global = _cvm_allocate_new_object(vm, HASH_MIN_CAPACITY);
    vm->pc = 0;

    vm->regs[0] = value_from_int(0);
    vm->regs[1] = value_from_ptr(vm->global);
    for (size_t i = 2; i < 65536; ++i) {
        vm->regs[i] = value_undefined();
    }
    return vm;
}

void
cvm_state_destroy(VMState *vm)
{
    if (vm->inst_list) {
        inst_list_destroy(vm->inst_list);
    }
    if (vm->young_gen) {
        young_gen_destroy(vm->young_gen);
    }
    if (vm->string_pool) {
        string_pool_destroy(vm->string_pool);
    }
    free(vm);
}

void
cvm_young_gc(VMState *vm)
{
    young_gen_gc_start(vm->young_gen);

    // TODO update it
    for (size_t i = 0; i < 65536; ++i) {
        if (value_is_ptr(vm->regs[i])) {
            Hash *hash = _cvm_get_hash_in_register(vm, i);
            young_gen_gc_mark(vm->young_gen, &hash);
            cvm_set_register(vm, i, value_from_ptr(hash));
        }
    }

    young_gen_gc_end(vm->young_gen);
}

Value
cvm_create_light_function(VMState *vm, light_function func)
{
    Hash *ret = _cvm_allocate_new_hash(vm, HASH_MIN_CAPACITY, HT_LIGHTFUNC);
    ret->hi_func = func;
    return value_from_ptr(ret);
}

void
cvm_register_in_global(VMState *vm, Value value, const char *name)
{
    CString *key = string_pool_insert_str(&vm->string_pool, name);
    _cvm_set_hash(vm, 1, (uintptr_t)key, value);
}

void
cvm_state_run(VMState *vm)
{
    for (;;) {
        Inst inst = vm->inst_list->insts[vm->pc++];
        switch (inst.type) {
            case I_HALT:
                return;
            case I_LI:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(inst.i_imm)
                );
                break;
            case I_ADD:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) +
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SUB:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) -
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_MUL:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) *
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_DIV:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) /
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_MOD:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) %
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SEQ:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) ==
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SLT:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SLE:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <=
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_LNOT:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        !value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_BNR:
                if (!value_to_int(cvm_get_register(vm, inst.i_rd))) {
                    vm->pc += inst.i_imm;
                }
                break;
            case I_J:
                vm->pc = (size_t)inst.i_imm;
                break;
            case I_JAL:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(vm->pc)
                );
                vm->pc = (size_t)inst.i_imm;
                break;
            case I_LSTR:
            {
                CString *string = (CString*)inst.i_imm;
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_string(string)
                );
                break;
            }
            case I_NEW_OBJ:
            {
                Hash *new_obj = _cvm_allocate_new_object(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_obj)
                );
                break;
            }
            case I_NEW_ARR:
            {
                Hash *new_arr = _cvm_allocate_new_array(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_arr)
                );
                break;
            }
            case I_SET_OBJ:
            {
                Hash *obj = _cvm_get_hash_in_register(vm, inst.i_rd);
                uintptr_t key;
                if (obj->type == HT_OBJECT) {
                    key = (uintptr_t)value_to_string(cvm_get_register(vm, inst.i_rt));
                }
                else {
                    key = (uintptr_t)value_to_int(cvm_get_register(vm, inst.i_rt));
                }
                _cvm_set_hash(vm, inst.i_rd, key, cvm_get_register(vm, inst.i_rs));
                break;
            }
            case I_GET_OBJ:
            {
                Hash *obj = _cvm_get_hash_in_register(vm, inst.i_rs);
                uintptr_t key;
                if (obj->type == HT_OBJECT) {
                    key = (uintptr_t)value_to_string(cvm_get_register(vm, inst.i_rt));
                }
                else {
                    key = (uintptr_t)value_to_int(cvm_get_register(vm, inst.i_rt));
                }
                cvm_set_register(
                    vm, inst.i_rd,
                    hash_find(obj, key)
                );
                break;
            }
            case I_CALL:
            {
                Hash *func = _cvm_get_hash_in_register(vm, inst.i_rs);
                if (func->type == HT_LIGHTFUNC) {
                    cvm_set_register(
                        vm, inst.i_rd,
                        func->hi_func(cvm_get_register(vm, inst.i_rt))
                    );
                }
                else {
                    type_error(vm, HT_LIGHTFUNC, func->type);
                }
                break;
            }
            default:
                error_f("Unknown VM instrument (offset %d)", vm->pc - 1);
                break;
        }
    }
}
