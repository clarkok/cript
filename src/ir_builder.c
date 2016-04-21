//
// Created by c on 4/21/16.
//

#include <assert.h>

#include "ir_builder.h"

#define ir_builder_from_bb(bb)  \
    container_of(list_from_node(&bb->_linked), IRBuilder, basic_blocks)

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
    ret->then_bb = NULL;
    ret->else_bb = NULL;
    return ret;
}

static inline void
_ir_builder_basic_block_destroy(BasicBlock *bb)
{
    inst_list_destroy(bb->inst_list);
    hash_destroy(bb->constant_table);
    free(bb);
}

IRBuilder *
ir_builder_new(size_t reserved_register)
{
    IRBuilder *ret = malloc(sizeof(IRBuilder));
    ret->register_nr = reserved_register;
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

InstList *
ir_builder_destroy(IRBuilder *builder)
{
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
            inst_list_push(ret, *ptr);
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

    return ret;
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
    while (bb) {
        Value result = hash_find(bb->constant_table, (uintptr_t)constant._int);
        if (!value_is_undefined(result)) { return value_to_int(result); }
        bb = bb->dominator;
    }
    return -1;
}

size_t
ir_builder_li(BasicBlock *bb, int imm)
{
    if (!imm) return 0;

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

size_t
ir_builder_add(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_sub(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_mul(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_div(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_mod(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_seq(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_slt(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_sle(BasicBlock *bb, size_t rs, size_t rt)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_lnot(BasicBlock *bb, size_t rs)
{
    assert_basic_block_not_end(bb);

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
    return ret;
}

size_t
ir_builder_undefined(BasicBlock *bb)
{
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
    return ret;
}

size_t
ir_builder_null(BasicBlock *bb)
{
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
