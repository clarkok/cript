//
// Created by c on 4/17/16.
//

#ifndef CRIPT_INST_OUTPUT_H
#define CRIPT_INST_OUTPUT_H

#include <stdio.h>

#include "inst.h"
#include "inst_list.h"
#include "value.h"
#include "string_pool.h"

extern const char INST_NAME[];

static inline void
output_inst(FILE *fout, Inst inst)
{
    fprintf(fout, "%8.8s", INST_NAME + (inst.type << 3));
    switch (inst.type) {
        case I_LI:
        case I_BNR:
        case I_JAL:
            fprintf(fout, "$%d,\t%d\n", inst.i_rd, inst.i_imm);
            break;
        case I_ADD:
        case I_SUB:
        case I_MUL:
        case I_DIV:
        case I_MOD:
        case I_SEQ:
        case I_SLT:
        case I_SLE:
        case I_SET_OBJ:
        case I_GET_OBJ:
            fprintf(fout, "$%d,\t$%d,\t$%d\n", inst.i_rd, inst.i_rs, inst.i_rt);
            break;
        case I_J:
            fprintf(fout, "%d\n", inst.i_imm);
            break;
        case I_LNOT:
            fprintf(fout, "$%d,\t$%d\n", inst.i_rd, inst.i_rs);
            break;
        case I_LSTR:
        {
            CString *string = (CString*)(inst.i_imm);
            fprintf(fout, "$%d,\t\"%.*s\"\n", inst.i_rd, string->length, string->content);
            break;
        }
        case I_NEW_OBJ:
        case I_NEW_ARR:
            fprintf(fout, "$%d\n", inst.i_rd);
            break;
        default:
            fprintf(fout, "\n");
            break;
    }
}

static inline void
output_inst_list(FILE *fout, InstList *list)
{
    Inst *ptr = list->insts,
         *limit = list->insts + list->count;
    for (; ptr != limit; ++ptr) {
        output_inst(fout, *ptr);
    }
}

#endif //CRIPT_INST_OUTPUT_H
