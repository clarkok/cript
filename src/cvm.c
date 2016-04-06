//
// Created by c on 4/4/16.
//

#include <stdlib.h>
#include <string.h>

#include "cvm.h"
#include "error.h"

static const size_t CVM_LIST_DEFAULT_CAPACITY = 16;

static inline size_t
cvm_list_insts_size(size_t capacity)
{ return capacity * sizeof(Inst); }

static inline size_t
cvm_list_resize_capacity(size_t original)
{ return original ? (original + (original >> 1)) : CVM_LIST_DEFAULT_CAPACITY; }

InstList *
cvm_list_new(size_t capacity)
{
    size_t insts_size = cvm_list_insts_size(capacity);

    InstList *ret = malloc(sizeof(InstList) + insts_size);
    if (!ret) return NULL;

    memset((char *)(ret) + sizeof(InstList), 0, insts_size);

    ret->capacity = capacity;
    ret->count = 0;

    return ret;
}

InstList *
cvm_list_resize(InstList *list, size_t capacity)
{
    size_t insts_size = cvm_list_insts_size(capacity);
    size_t original_size = cvm_list_insts_size(list->capacity);

    list = realloc(list, sizeof(InstList) + insts_size);
    if (capacity > list->capacity) {
        memset(
            (char*)(list) + original_size + sizeof(InstList),
            0,
            insts_size - original_size
        );
    }
    list->capacity = capacity;

    return list;
}

InstList *
cvm_list_append(InstList *list, Inst inst)
{
    if (list->count == list->capacity) {
        list = cvm_list_resize(list, cvm_list_resize_capacity(list->capacity));
    }

    list->insts[list->count++] = inst;
    return list;
}

void
cvm_list_destroy(InstList *list)
{ free(list); }

VMState *
cvm_state_new(InstList *inst_list)
{
    VMState *vm = malloc(sizeof(VMState));
    if (!vm) return NULL;

    vm->inst_list = inst_list;
    vm->pc = 0;
    return vm;
}

void
cvm_state_destroy(VMState *vm)
{
    if (vm->inst_list) {
        cvm_list_destroy(vm->inst_list);
    }
    free(vm);
}

static inline void
cvm_set_register(VMState *vm, unsigned int reg, Value value)
{ if (reg) { vm->regs[reg] = value; } }

static inline Value
cvm_get_register(VMState *vm, unsigned int reg)
{ return reg ? vm->regs[reg] : value_from_int(0); }

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
                        value_to_int(cvm_get_register(vm, inst.i_rt)) +
                        value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_SUB:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rt)) +
                        value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_MUL:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rt)) *
                        value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_DIV:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rt)) /
                        value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_MOD:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rt)) %
                        value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            default:
                error_f("Unknown VM instrument (offset %d)", vm->pc - 1);
                break;
        }
    }
}
