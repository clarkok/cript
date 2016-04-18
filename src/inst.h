//
// Created by c on 4/16/16.
//

#ifndef CRIPT_INST_H
#define CRIPT_INST_H

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

    I_LNOT,
    I_LAND,
    I_LOR,

    I_BNR,
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

#endif //CRIPT_INST_H
