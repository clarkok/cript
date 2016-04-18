//
// Created by c on 4/4/16.
//

#ifndef CRIPT_CVM_H
#define CRIPT_CVM_H

#include <stddef.h>

#include "value.h"
#include "string_pool.h"
#include "parse.h"

#include "inst.h"
#include "inst_list.h"

typedef struct YoungGen YoungGen;

typedef struct VMState
{
    StringPool *string_pool;
    YoungGen *young_gen;

    InstList *inst_list;
    size_t pc;
    Value regs[65536];
} VMState;

VMState *cvm_state_new(InstList *inst_list, StringPool *string_pool);

static inline VMState*
cvm_state_new_from_parse_state(ParseState *state)
{
    VMState *ret = cvm_state_new(state->inst_list, state->string_pool);
    state->inst_list = NULL;
    state->string_pool = NULL;

    return ret;
}

void cvm_state_run(VMState *vm);
void cvm_state_destroy(VMState *vm);

Value cvm_get_register(VMState *vm, unsigned int reg_id);
void cvm_set_register(VMState *vm, unsigned int reg_id, Value value);

void cvm_young_gc(VMState *vm);

#endif //CRIPT_CVM_H
