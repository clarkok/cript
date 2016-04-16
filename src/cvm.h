//
// Created by c on 4/4/16.
//

#ifndef CRIPT_CVM_H
#define CRIPT_CVM_H

#include <stddef.h>

#include "value.h"
#include "string_pool.h"

#include "inst.h"
#include "inst_list.h"

typedef struct VMState
{
    StringPool *string_pool;

    InstList *inst_list;
    size_t pc;
    Value regs[65536];
} VMState;

VMState *cvm_state_new(InstList *inst_list, StringPool *string_pool);
void cvm_state_run(VMState *vm);
void cvm_state_destroy(VMState *vm);

Value cvm_get_register(VMState *vm, unsigned int reg_id);
void cvm_set_register(VMState *vm, unsigned int reg_id, Value value);

#endif //CRIPT_CVM_H
