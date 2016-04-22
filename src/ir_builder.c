//
// Created by c on 4/21/16.
//

#include <assert.h>

#include "error.h"
#include "ir_builder.h"
#include "cvm.h"

#define ir_builder_from_bb(bb)  \
    container_of(list_from_node(&bb->_linked), IRBuilder, basic_blocks)

enum SubExprType {
    S_ADD = 0,
    S_SUB,
    S_MUL,
    S_DIV,
    S_MOD,
    S_SEQ,
    S_SLT,
    S_SLE,
    S_LNOT,

    SUB_EXPR_TYPE_NR
};

#define SUBEXPR_TYPE_BITS           (4)
#define SUBEXPR_REG_BITS            ((sizeof(uintptr_t) * 8) - SUBEXPR_TYPE_BITS)
#define SUBEXPR_REG_LIMIT_BITS      (SUBEXPR_REG_BITS / 2)
#define SUBEXPR_REG_LIMIT           (1 << SUBEXPR_REG_LIMIT_BITS)

_Static_assert(
    (SUB_EXPR_TYPE_NR <= (1 << SUBEXPR_TYPE_BITS)),
    "Current SubExprType is 4 bits"
);

static inline uintptr_t
_ir_builder_subexpr_key(unsigned op, size_t reg_a, size_t reg_b)
{
    if (reg_a >= SUBEXPR_REG_LIMIT || reg_b >= SUBEXPR_REG_LIMIT) return 0;
    return (op << SUBEXPR_REG_BITS) | (reg_a << SUBEXPR_REG_LIMIT_BITS) | reg_b;
}

static inline BasicBlock *
_ir_builder_basic_block_new(IRBuilder *builder, BasicBlock *dominator)
{
    BasicBlock *ret = malloc(sizeof(BasicBlock));
    list_append(&builder->basic_blocks, &ret->_linked);
    ret->dominator = dominator;
    ret->inst_list = inst_list_new(16);
    ret->entry_point = value_from_ptr((void*)(0xFFFFFFFFu << 2));
    ret->br_reg = 0;
    ret->constant_table = hash_new(HASH_MIN_CAPACITY);
    hash_set_and_update(
        ret->constant_table,
        value_from_int(0)._int,
        value_from_int(0)
    );
    ret->numeric_value = hash_new(HASH_MIN_CAPACITY);
    hash_set_and_update(
        ret->numeric_value,
        0,
        value_from_int(0)
    );
    ret->subexpr_table = hash_new(HASH_MIN_CAPACITY);
    ret->returned = 0;
    ret->then_bb = NULL;
    ret->else_bb = NULL;
    return ret;
}

static inline void
_ir_builder_basic_block_destroy(BasicBlock *bb)
{
    inst_list_destroy(bb->inst_list);
    hash_destroy(bb->constant_table);
    hash_destroy(bb->numeric_value);
    hash_destroy(bb->subexpr_table);
    free(bb);
}

IRBuilder *
ir_builder_new()
{
    IRBuilder *ret = malloc(sizeof(IRBuilder));
    ret->register_nr = 0;
    ret->register_usage = hash_new(HASH_MIN_CAPACITY);
    list_init(&ret->basic_blocks);
    _ir_builder_basic_block_new(ret, NULL);
    return ret;
}

BasicBlock *
ir_builder_entry(IRBuilder *builder)
{ return list_get(list_head(&builder->basic_blocks), BasicBlock, _linked); }

BasicBlock *
ir_builder_new_basic_block(IRBuilder *builder, BasicBlock *dominator_bb)
{
    assert(!dominator_bb || list_from_node(&dominator_bb->_linked) == &builder->basic_blocks);
    return _ir_builder_basic_block_new(builder, dominator_bb);
}

static inline void
_ir_builder_use_register(BasicBlock *bb, size_t reg)
{
    IRBuilder *builder = container_of(list_from_node(&bb->_linked), IRBuilder, basic_blocks);
    Value current = hash_find(builder->register_usage, reg);

    if (value_is_undefined(current)) {
        hash_set_and_update(
            builder->register_usage,
            reg,
            value_from_int(1)
        );
    }
    else {
        hash_set_and_update(
            builder->register_usage,
            reg,
            value_from_int(value_to_int(current) + 1)
        );
    }
}

static inline void
_ir_builder_bnr_else(InstList **inst_list, BasicBlock *bb, BasicBlock *dst)
{
    int bnr_index = (*inst_list)->count;
    if (value_is_ptr(bb->else_bb->entry_point)) {
        inst_list_push(
            (*inst_list),
            cvm_inst_new_i_type(
                I_BNR,
                bb->br_reg,
                (int)value_to_ptr(dst->entry_point)
            )
        );
        dst->entry_point = value_from_ptr((void*)(bnr_index << 2));
    }
    else {
        inst_list_push(
            (*inst_list),
            cvm_inst_new_i_type(
                I_BNR,
                bb->br_reg,
                value_to_int(dst->entry_point) - bnr_index - 1
            )
        );
    }
}

static inline void
_ir_builder_jump_to(InstList **inst_list, BasicBlock *bb, BasicBlock *dst)
{
    if (!dst) {
        if (bb->returned) return;

        // cannot allocate new register here
        inst_list_push(
            *inst_list,
            cvm_inst_new_d_type(
                I_UNDEFINED,
                1, 0, 0
            )
        );
        inst_list_push(
            *inst_list,
            cvm_inst_new_d_type(
                I_RET,
                1, 0, 0
            )
        );
    }
    else if (!list_next(&bb->_linked) || dst != list_get(list_next(&bb->_linked), BasicBlock, _linked)) {
        if (value_is_ptr(dst->entry_point)) {
            size_t j_index = (*inst_list)->count;
            inst_list_push(
                *inst_list,
                cvm_inst_new_i_type(
                    I_J,
                    0,
                    (int)value_to_ptr(dst->entry_point)
                )
            );
            dst->entry_point = value_from_ptr((void*)(j_index << 2));
        }
        else {
            inst_list_push(
                *inst_list,
                cvm_inst_new_i_type(
                    I_J,
                    0,
                    value_to_int(dst->entry_point)
                )
            );
        }
    }
}

static inline size_t
_ir_builder_result_register(Inst inst)
{
    switch (inst.type) {
        case I_LI:
        case I_ADD:
        case I_SUB:
        case I_MUL:
        case I_DIV:
        case I_MOD:
        case I_SEQ:
        case I_SLT:
        case I_SLE:
        case I_LNOT:
        case I_MOV:
        case I_LSTR:
        case I_NEW_OBJ:
        case I_NEW_ARR:
        case I_GET_OBJ:
        case I_NEW_CLS:
        case I_UNDEFINED:
        case I_NULL:
            return inst.i_rd;
        default:
            return 0;
    }
}

static inline void
_ir_builder_unuse_register(IRBuilder *builder, size_t reg)
{
    Value result = hash_find(builder->register_usage, reg);
    assert(value_is_int(result));
    if (value_to_int(result) == 1) {
        hash_set(
            builder->register_usage,
            reg,
            value_undefined()
        );
    }
    else {
        hash_set(
            builder->register_usage,
            reg,
            value_from_int(value_to_int(result) - 1)
        );
    }
}

static inline void
_ir_builder_update_register_usage(IRBuilder *builder)
{
    list_for_each_r(&builder->basic_blocks, node) {
        BasicBlock *bb = list_get(node, BasicBlock, _linked);
        for (
            Inst *ptr = bb->inst_list->insts + bb->inst_list->count - 1,
                 *limit = bb->inst_list->insts - 1;
            ptr != limit;
            ptr--
        ) {
            size_t result_reg = _ir_builder_result_register(*ptr);
            if (result_reg) {
                Value value = hash_find(builder->register_usage, result_reg);
                if (value_is_undefined(value)) {
                    switch (ptr->type) {
                        case I_ADD:
                        case I_SUB:
                        case I_MUL:
                        case I_DIV:
                        case I_MOD:
                        case I_SEQ:
                        case I_SLT:
                        case I_SLE:
                        case I_GET_OBJ:
                            _ir_builder_unuse_register(builder, ptr->i_rs);
                            _ir_builder_unuse_register(builder, ptr->i_rt);
                            break;
                        case I_LNOT:
                        case I_MOV:
                            _ir_builder_unuse_register(builder, ptr->i_rs);
                            break;
                        case I_LI:
                        case I_LSTR:
                        case I_UNDEFINED:
                        case I_NULL:
                        case I_NEW_OBJ:
                        case I_NEW_ARR:
                        case I_NEW_CLS:
                            break;
                        default:
                            error_f(
                                "Internal error, meeting inst type %d, reg %d",
                                ptr->type, 
                                result_reg
                            );
                            return;
                    }
                }
            }
        }
    }
}

InstList *
ir_builder_destroy(IRBuilder *builder)
{
    _ir_builder_update_register_usage(builder);

    InstList *ret = inst_list_new(16);
    list_for_each(&builder->basic_blocks, node) {
        BasicBlock *bb = list_get(node, BasicBlock, _linked);
        size_t entry_point = ret->count;

        if (value_is_ptr(bb->entry_point)) {
            int inst_to_fill = (int)value_to_ptr(bb->entry_point) >> 2;
            while (inst_to_fill >= 0) {
                Inst *inst = ret->insts + inst_to_fill;
                inst_to_fill = inst->i_imm >> 2;
                if (inst->type == I_J) {
                    inst->i_imm = entry_point;
                }
                else {
                    inst->i_imm = entry_point - (inst - ret->insts) - 1;
                }
            }
        }

        bb->entry_point = value_from_int(entry_point);

        for (
            Inst *ptr = bb->inst_list->insts,
                 *limit = bb->inst_list->insts + bb->inst_list->count;
            ptr != limit;
            ++ptr
        ) {
            size_t result_reg = _ir_builder_result_register(*ptr);
            if (!result_reg) {
                inst_list_push(ret, *ptr);
            }
            else {
                Value value = hash_find(builder->register_usage, result_reg);
                if (!value_is_undefined(value)) {
                    inst_list_push(ret, *ptr);
                }
            }
        }

        if (bb->br_reg) {
            _ir_builder_bnr_else(&ret, bb, bb->else_bb);
            _ir_builder_jump_to(&ret, bb, bb->then_bb);
        }
        else {
            _ir_builder_jump_to(&ret, bb, bb->else_bb);
        }
    }

    while (list_size(&builder->basic_blocks)) {
        _ir_builder_basic_block_destroy(
            list_get(list_unlink(list_head(&builder->basic_blocks)), BasicBlock, _linked)
        );
    }
    hash_destroy(builder->register_usage);
    free(builder);

    return ret;
}

size_t
ir_builder_allocate_argument(IRBuilder *builder)
{
    assert(list_size(&builder->basic_blocks) == 1);
    assert(
        list_get(
            list_head(&builder->basic_blocks),
            BasicBlock,
            _linked
        )->inst_list->count == 0);
    return ir_builder_allocate_register(builder);
}

size_t
ir_builder_allocate_register(IRBuilder *builder)
{ return ++(builder->register_nr); }

#define assert_basic_block_not_end(bb)      \
    assert(!bb->br_reg && !bb->else_bb)

void
ir_builder_br(BasicBlock *bb, size_t rd, BasicBlock *then_bb, BasicBlock *else_bb)
{
    assert_basic_block_not_end(bb);
    bb->br_reg = rd;
    bb->then_bb = then_bb;
    bb->else_bb = else_bb;

    _ir_builder_use_register(bb, rd);
}

void
ir_builder_j(BasicBlock *bb, BasicBlock *successor_bb)
{
    assert_basic_block_not_end(bb);
    bb->br_reg = 0;
    bb->else_bb = successor_bb;
}

static inline int
_ir_builder_find_in_constant_table(BasicBlock *bb, Value constant)
{
    BasicBlock *begin_bb = bb;
    while (bb) {
        Value result = hash_find(bb->constant_table, (uintptr_t)constant._int);
        if (!value_is_undefined(result)) {
            while (begin_bb) {
                Value path_result = hash_find(begin_bb->constant_table, (uintptr_t)constant._int);
                if (value_is_undefined(path_result)) {
                    hash_set_and_update(
                        begin_bb->constant_table,
                        (uintptr_t)constant._int,
                        result
                    );
                    begin_bb = begin_bb->dominator;
                }
                else { break; }
            }
            return value_to_int(result);
        }
        bb = bb->dominator;
    }
    return -1;
}

static inline Value
_ir_builder_find_in_numeric_table(BasicBlock *bb, size_t reg)
{
    BasicBlock *begin_bb = bb;
    while (bb) {
        Value result = hash_find(bb->numeric_value, (uintptr_t)reg);
        if (!value_is_undefined(result)) {
            while (begin_bb) {
                Value path_result = hash_find(begin_bb->numeric_value, (uintptr_t)reg);
                if (value_is_undefined(path_result)) {
                    hash_set_and_update(
                        begin_bb->numeric_value,
                        (uintptr_t)reg,
                        result
                    );
                    begin_bb = begin_bb->dominator;
                }
                else { break; }
            }
            return result;
        }
        bb = bb->dominator;
    }
    return value_undefined();
}

static inline Value
_ir_builder_find_in_subexpr_table(BasicBlock *bb, unsigned op, size_t reg_a, size_t reg_b)
{
    uintptr_t key = _ir_builder_subexpr_key(op, reg_a, reg_b);
    if (!key) { return value_undefined(); }
    BasicBlock *begin_bb = bb;
    while (bb) {
        Value result = hash_find(bb->subexpr_table, key);
        if (!value_is_undefined(result)) {
            while (begin_bb) {
                Value path_result = hash_find(begin_bb->subexpr_table, key);
                if (value_is_undefined(path_result)) {
                    hash_set_and_update(
                        begin_bb->subexpr_table,
                        key,
                        result
                    );
                    begin_bb = begin_bb->dominator;
                }
                else { break; }
            }
            return result;
        }
        bb = bb->dominator;
    }
    return value_undefined();
}

static inline void
_ir_builder_set_subexpr_table(
    BasicBlock *bb,
    unsigned op,
    size_t reg_a,
    size_t reg_b,
    size_t result
    )
{
    uintptr_t key = _ir_builder_subexpr_key(op, reg_a, reg_b);
    if (!key) { return; }
    hash_set_and_update(
        bb->subexpr_table,
        key,
        value_from_int(result)
    );
}

size_t
ir_builder_li(BasicBlock *bb, int imm)
{
    int constant = _ir_builder_find_in_constant_table(bb, value_from_int(imm));
    if (constant >= 0) { return (size_t)constant; }

    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_i_type(
            I_LI,
            ret,
            imm
        )
    );
    hash_set_and_update(
        bb->constant_table,
        (uintptr_t)(value_from_int(imm)._int),
        value_from_int(ret)
    );
    hash_set_and_update(
        bb->numeric_value,
        ret,
        value_from_int(imm)
    );
    return ret;
}

size_t
ir_builder_lstr(BasicBlock *bb, CString *string)
{
    int constant = _ir_builder_find_in_constant_table(bb, value_from_string(string));
    if (constant >= 0) { return (size_t)constant; }

    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_i_type(
            I_LSTR,
            ret,
            (int)string
        )
    );
    hash_set_and_update(
        bb->constant_table,
        (uintptr_t)(value_from_string(string)._int),
        value_from_int(ret)
    );
    return ret;
}

#define _ir_fold_constant(bb, rs, rt, op)                                               \
    do {                                                                                \
        Value const_l = _ir_builder_find_in_numeric_table(bb, rs),                      \
              const_r = _ir_builder_find_in_numeric_table(bb, rt);                      \
        if (value_is_int(const_l) && value_is_int(const_r)) {                           \
            return ir_builder_li(bb, value_to_int(const_l) op value_to_int(const_r));   \
        }                                                                               \
    } while (0)

#define _ir_subexpr_estimate(bb, op, rs, rt)                                            \
    do {                                                                                \
        Value subexpr_result = _ir_builder_find_in_subexpr_table(bb, op, rs, rt);       \
        if (!value_is_undefined(subexpr_result)) {                                      \
            return (size_t)value_to_int(subexpr_result);                                \
        }                                                                               \
    } while (0)

size_t
ir_builder_add(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, +);
    _ir_subexpr_estimate(bb, S_ADD, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_ADD,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_ADD, rs, rt, ret);
    _ir_builder_set_subexpr_table(bb, S_ADD, rt, rs, ret);
    return ret;
}

size_t
ir_builder_sub(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, -);
    _ir_subexpr_estimate(bb, S_SUB, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_SUB,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_SUB, rs, rt, ret);
    return ret;
}

size_t
ir_builder_mul(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, *);
    _ir_subexpr_estimate(bb, S_MUL, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_MUL,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_MUL, rs, rt, ret);
    _ir_builder_set_subexpr_table(bb, S_MUL, rt, rs, ret);
    return ret;
}

size_t
ir_builder_div(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, /);
    _ir_subexpr_estimate(bb, S_DIV, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_DIV,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_DIV, rs, rt, ret);
    return ret;
}

size_t
ir_builder_mod(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, %);
    _ir_subexpr_estimate(bb, S_MOD, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_MOD,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_MOD, rs, rt, ret);
    return ret;
}

size_t
ir_builder_seq(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, ==);
    _ir_subexpr_estimate(bb, S_SEQ, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_SEQ,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_SEQ, rs, rt, ret);
    _ir_builder_set_subexpr_table(bb, S_SEQ, rt, rs, ret);
    return ret;
}

size_t
ir_builder_slt(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, <);
    _ir_subexpr_estimate(bb, S_SLT, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_SLT,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_SLT, rs, rt, ret);
    return ret;
}

size_t
ir_builder_sle(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);
    _ir_fold_constant(bb, rs, rt, <=);
    _ir_subexpr_estimate(bb, S_SLE, rs, rt);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_SLE,
            ret,
            rs,
            rt
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_use_register(bb, rt);
    _ir_builder_set_subexpr_table(bb, S_SLE, rs, rt, ret);
    return ret;
}

size_t
ir_builder_lnot(BasicBlock *bb, size_t rs)
{
    assert_basic_block_not_end(bb);
    Value const_reg = _ir_builder_find_in_numeric_table(bb, rs);
    if (value_is_int(const_reg)) {
        return ir_builder_li(bb, !value_to_int(const_reg));
    }
    _ir_subexpr_estimate(bb, S_LNOT, rs, 0);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_LNOT,
            ret,
            rs,
            0
        )
    );
    _ir_builder_use_register(bb, rs);
    _ir_builder_set_subexpr_table(bb, S_SLE, rs, 0, ret);
    return ret;
}

size_t
ir_builder_mov(BasicBlock *bb, size_t rs)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_MOV,
            ret,
            rs,
            0
        )
    );

    Value constant = _ir_builder_find_in_numeric_table(bb, rs);
    if (value_is_int(constant)) {
        hash_set_and_update(
            bb->numeric_value,
            ret,
            constant
        );
    }

    _ir_builder_use_register(bb, rs);
    return ret;
}

void
ir_builder_mov_upper(BasicBlock *bb, size_t rd, size_t rs)
{
    assert_basic_block_not_end(bb);

    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_MOV,
            rd,
            rs,
            0
        )
    );

    Value constant = _ir_builder_find_in_numeric_table(bb, rs);
    if (value_is_int(constant)) {
        hash_set_and_update(
            bb->numeric_value,
            rd,
            constant
        );
    }
    _ir_builder_use_register(bb, rs);
}

size_t
ir_builder_new_obj(BasicBlock *bb)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_NEW_OBJ,
            ret, 0, 0
        )
    );
    return ret;
}

size_t
ir_builder_new_arr(BasicBlock *bb)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_NEW_ARR,
            ret, 0, 0
        )
    );
    return ret;
}

void
ir_builder_set_obj(BasicBlock *bb, size_t r_obj, size_t r_val, size_t r_key)
{
    assert_basic_block_not_end(bb);

    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_SET_OBJ,
            r_obj,
            r_val,
            r_key
        )
    );
    _ir_builder_use_register(bb, r_obj);
    _ir_builder_use_register(bb, r_key);
    _ir_builder_use_register(bb, r_val);
}

size_t
ir_builder_get_obj(BasicBlock *bb, size_t r_obj, size_t r_key)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_GET_OBJ,
            ret,
            r_obj,
            r_key
        )
    );
    _ir_builder_use_register(bb, r_obj);
    _ir_builder_use_register(bb, r_key);
    return ret;
}

size_t
ir_builder_call(BasicBlock *bb, size_t r_func, size_t r_arg)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_CALL,
            ret,
            r_func,
            r_arg
        )
    );
    _ir_builder_use_register(bb, r_func);
    _ir_builder_use_register(bb, r_arg);
    return ret;
}

void
ir_builder_ret(BasicBlock *bb, size_t r_val)
{
    assert_basic_block_not_end(bb);

    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_RET,
            r_val,
            0, 0
        )
    );
    _ir_builder_use_register(bb, r_val);
    bb->returned = 1;
}

size_t
ir_builder_new_cls(BasicBlock *bb, VMFunction *func)
{
    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_i_type(
            I_NEW_CLS,
            ret,
            (int)func
        )
    );

    hash_for_each(func->capture_list, node) {
        _ir_builder_use_register(bb, node->key);
    }
    return ret;
}

size_t
ir_builder_undefined(BasicBlock *bb)
{
    int constant = _ir_builder_find_in_constant_table(bb, value_undefined());
    if (constant >= 0) { return (size_t)constant; }

    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_UNDEFINED,
            ret,
            0, 0
        )
    );
    hash_set_and_update(
        bb->constant_table,
        (uintptr_t)(value_undefined()._int),
        value_from_int(ret)
    );
    return ret;
}

size_t
ir_builder_null(BasicBlock *bb)
{
    int constant = _ir_builder_find_in_constant_table(bb, value_null());
    if (constant >= 0) { return (size_t)constant; }

    assert_basic_block_not_end(bb);

    size_t ret = ir_builder_allocate_register(ir_builder_from_bb(bb));
    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_NULL,
            ret,
            0, 0
        )
    );
    hash_set_and_update(
        bb->constant_table,
        (uintptr_t)(value_null()._int),
        value_from_int(ret)
    );
    return ret;
}

void
ir_builder_halt(BasicBlock *bb)
{
    assert_basic_block_not_end(bb);

    inst_list_push(
        bb->inst_list,
        cvm_inst_new_d_type(
            I_HALT,
            0, 0, 0
        )
    );
}
