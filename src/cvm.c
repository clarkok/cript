//
// Created by c on 4/4/16.
//

#include <stdlib.h>

#include "cvm.h"
#include "error.h"

VMState *
cvm_state_new(InstList *inst_list, StringPool *string_pool)
{
    VMState *vm = malloc(sizeof(VMState));
    if (!vm) return NULL;

    vm->string_pool = string_pool;
    vm->inst_list = inst_list;
    vm->pc = 0;
    vm->regs[0] = value_from_int(0);
    return vm;
}

void
cvm_state_destroy(VMState *vm)
{
    if (vm->inst_list) {
        inst_list_destroy(vm->inst_list);
    }
    if (vm->string_pool) {
        string_pool_destroy(vm->string_pool);
    }
    free(vm);
}

inline void
cvm_set_register(VMState *vm, unsigned int reg, Value value)
{ if (reg) { vm->regs[reg] = value; } }

inline Value
cvm_get_register(VMState *vm, unsigned int reg_id)
{ return vm->regs[reg_id]; }

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
            case I_SGT:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) >
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SGE:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) >=
                        value_to_int(cvm_get_register(vm, inst.i_rt))
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
            default:
                error_f("Unknown VM instrument (offset %d)", vm->pc - 1);
                break;
        }
    }
}
