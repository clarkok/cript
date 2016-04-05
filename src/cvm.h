//
// Created by c on 4/4/16.
//

#ifndef CRIPT_CVM_H
#define CRIPT_CVM_H

#include <stddef.h>

#include "value.h"

enum InstType
{
    I_UNKNOWN = 0,
    I_HALT,
    I_LI,
    I_ADD,
    I_SUB,
    I_MUL,
    I_DIV,
    I_MOD,

    INST_NR
};

typedef struct Inst
{
    unsigned int type : 16;

    unsigned int i_rd : 16;
    union {
        struct {
            unsigned int rs : 16;
            unsigned int rt : 16;
        } _d_type;
        struct {
            intptr_t imm : 32;
        } _i_type;
    } _info;
} Inst;

#define i_rs  _info._d_type.rs
#define i_rt  _info._d_type.rt
#define i_imm _info._i_type.imm

Inst cvm_inst_new_d_type(unsigned int type, unsigned int rd, unsigned int rs, unsigned int rt);
Inst cvm_inst_new_i_type(unsigned int type, unsigned int rd, unsigned int imm);

typedef struct InstList
{
    size_t capacity;
    size_t count;
    Inst insts[0];
} InstList;

InstList *cvm_list_new(size_t capacity);
InstList *cvm_list_resize(InstList *list, size_t capacity);
InstList *cvm_list_append(InstList *list, Inst inst);
void cvm_list_destroy(InstList *list);

typedef struct VMState
{
    InstList *inst_list;
    size_t pc;
    Value regs[65536];
} VMState;

VMState *cvm_state_new(InstList *inst_list);
void cvm_state_run(VMState *vm);
void cvm_state_destroy(VMState *vm);

#endif //CRIPT_CVM_H
