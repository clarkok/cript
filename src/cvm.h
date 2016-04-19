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

#include "list.h"

typedef struct YoungGen YoungGen;

typedef struct VMFunction
{
    LinkedNode _linked;

    size_t arguments_nr;
    size_t register_nr;
    Hash *capture_list;
    InstList *inst_list;
} VMFunction;

typedef struct VMFrame
{
    LinkedNode _linked;

    VMFunction *function;
    size_t pc;
    Value regs[0];
} VMFrame;

typedef struct VMState
{
    StringPool *string_pool;
    YoungGen *young_gen;
    Hash *global;

    LinkedList functions;
    LinkedList frames;
} VMState;

VMState *cvm_state_new_from_parse_state(ParseState *state);
VMState *cvm_state_new(InstList *main_inst_list, StringPool *string_pool);

void cvm_state_run(VMState *vm);
void cvm_state_destroy(VMState *vm);

Value cvm_get_register(VMState *vm, unsigned int reg_id);
void cvm_set_register(VMState *vm, unsigned int reg_id, Value value);

void cvm_young_gc(VMState *vm);

Value cvm_create_light_function(VMState *vm, light_function func);

void cvm_register_in_global(VMState *vm, Value value, const char *name);

#endif //CRIPT_CVM_H
