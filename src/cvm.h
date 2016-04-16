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
    I_SEQ,
    I_SLT,
    I_SLE,
    I_SGT,
    I_SGE,

    I_BR,
    I_J,
    I_JAL,

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
            signed int imm : 32;
        } _i_type;
    } _info;
} Inst;

#define i_rs  _info._d_type.rs
#define i_rt  _info._d_type.rt
#define i_imm _info._i_type.imm

static inline Inst
cvm_inst_new_d_type(unsigned int type, unsigned int rd, unsigned int rs, unsigned int rt)
{ Inst ret; ret.type = type; ret.i_rd = rd; ret.i_rs = rs; ret.i_rt = rt; return ret; }

static inline Inst
cvm_inst_new_i_type(unsigned int type, unsigned int rd, int imm)
{ Inst ret; ret.type = type; ret.i_rd = rd; ret.i_imm = imm; return ret; }

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
#define cvm_list_push(list, inst)   list = cvm_list_append(list, inst)

typedef struct VMState
{
    InstList *inst_list;
    size_t pc;
    Value regs[65536];
} VMState;

VMState *cvm_state_new(InstList *inst_list);
void cvm_state_run(VMState *vm);
void cvm_state_destroy(VMState *vm);

Value cvm_get_register(VMState *vm, unsigned int reg_id);
void cvm_set_register(VMState *vm, unsigned int reg_id, Value value);

#endif //CRIPT_CVM_H
