//
// Created by c on 4/21/16.
//

#ifndef CRIPT_IR_BUILDER_H
#define CRIPT_IR_BUILDER_H

#include "inst.h"
#include "inst_list.h"
#include "string_pool.h"
#include "value.h"
#include "hash.h"

#include "list.h"

typedef struct BasicBlock
{
    LinkedNode _linked;

    struct BasicBlock *dominator;
    InstList *inst_list;
    Value entry_point;
    size_t br_reg;
    Hash *constant_table;
    struct BasicBlock *then_bb;
    struct BasicBlock *else_bb;
} BasicBlock;

typedef struct IRBuilder
{
    size_t register_nr;
    LinkedList basic_blocks;
} IRBuilder;

IRBuilder *ir_builder_new(size_t reserved_register);
BasicBlock *ir_builder_entry(IRBuilder *builder);
BasicBlock *ir_builder_new_basic_block(IRBuilder *builder, BasicBlock *dominator_bb);
InstList *ir_builder_destroy(IRBuilder *builder);
size_t ir_builder_allocate_register(IRBuilder *builder);

void ir_builder_br(BasicBlock *bb, size_t rd, BasicBlock *then_bb, BasicBlock *else_bb);
void ir_builder_j(BasicBlock *bb, BasicBlock *successor_bb);

size_t ir_builder_li(BasicBlock *bb, int imm);
size_t ir_builder_lstr(BasicBlock *bb, CString *string);
size_t ir_builder_add(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_sub(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_mul(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_div(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_mod(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_seq(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_slt(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_sle(BasicBlock *bb, size_t rs, size_t rt);
size_t ir_builder_lnot(BasicBlock *bb, size_t rs);
size_t ir_builder_mov(BasicBlock *bb, size_t rs);
void ir_builder_mov_upper(BasicBlock *bb, size_t rd, size_t rs);
size_t ir_builder_new_obj(BasicBlock *bb);
size_t ir_builder_new_arr(BasicBlock *bb);
void ir_builder_set_obj(BasicBlock *bb, size_t r_obj, size_t r_val, size_t r_key);
size_t ir_builder_get_obj(BasicBlock *bb, size_t r_obj, size_t r_key);
size_t ir_builder_call(BasicBlock *bb, size_t r_func, size_t r_reg);
void ir_builder_ret(BasicBlock *bb, size_t r_val);
size_t ir_builder_new_cls(BasicBlock *bb, VMFunction *func);
size_t ir_builder_undefined(BasicBlock *bb);
size_t ir_builder_null(BasicBlock *bb);
void ir_builder_halt(BasicBlock *bb);

#endif
