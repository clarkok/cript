//
// Created by c on 4/4/16.
//

#include <stdlib.h>

#include "cvm.h"
#include "error.h"
#include "young_gen.h"
#include "hash_internal.h"

#define type_error(vm, expected, actual)            \
    error_f("TypeError: expected %s, but met %s\n", \
        hash_type_to_str(expected),                 \
        hash_type_to_str(actual))

#define vm_frame_top(vm)    \
    list_get(list_head(&vm->frames), VMFrame, _linked)

inline void
cvm_set_register(VMState *vm, unsigned int reg, Value value)
{
    if (reg) {
        VMFrame *frame = vm_frame_top(vm);
        frame->regs[reg] = value;
    }
}

inline Value
cvm_get_register(VMState *vm, unsigned int reg_id)
{ return vm_frame_top(vm)->regs[reg_id]; }

static inline Hash *
_cvm_allocate_new_hash(VMState *vm, size_t capacity, int type)
{
    Hash *hash = young_gen_new_hash(vm->young_gen, capacity, type);
    if (!hash) {
        cvm_young_gc(vm);
        hash = young_gen_new_hash(vm->young_gen, capacity, type);
        if (!hash) {
            error_f("Out of memory when allocating %s of capacity %d",
                    hash_type_to_str(type),
                    capacity
            );
        }
    }
    return hash;
}

static inline Hash *
_cvm_allocate_new_object(VMState *vm, size_t capacity)
{ return _cvm_allocate_new_hash(vm, capacity, HT_OBJECT); }

static inline Hash *
_cvm_allocate_new_array(VMState *vm, size_t capacity)
{ return _cvm_allocate_new_hash(vm, capacity, HT_ARRAY); }

static inline Hash *
_cvm_get_hash_in_register(VMState *vm, size_t reg)
{
    Hash *hash = value_to_ptr(cvm_get_register(vm, reg));
    if (hash->type == HT_REDIRECT) {
        while(hash->type == HT_REDIRECT) {
            hash = (Hash*)(hash->size);
        }
        cvm_set_register(vm, reg, value_from_ptr(hash));
    }
    return hash;
}

static inline void
_cvm_set_hash(VMState *vm, size_t reg, uintptr_t key, Value val)
{
    Hash *obj = _cvm_get_hash_in_register(vm, reg);
    hash_set(obj, key, val);
    if (hash_need_expand(obj)) {
        Hash *new_obj = _cvm_allocate_new_hash(vm, _hash_expand_size(obj), obj->type);

        obj = _cvm_get_hash_in_register(vm, reg);
        hash_rehash(new_obj, obj);

        obj->type = HT_REDIRECT;
        obj->capacity = 0;
        obj->size = (size_t)new_obj;
    }
}

static inline void
cvm_state_push_frame(VMState *vm, VMFunction *function)
{
    VMFrame *frame = malloc(sizeof(VMFrame) + function->register_nr * sizeof(Value));
    frame->function = function;
    frame->pc = 0;
    frame->regs[0] = value_from_int(0);
    frame->regs[1] = value_from_ptr(vm->global);
    for (size_t i = 2; i < function->register_nr; ++i) {
        frame->regs[i] = value_undefined();
    }

    list_prepend(&vm->frames, &frame->_linked);
}

static inline void
cvm_state_pop_frame(VMState *vm)
{ free(list_get(list_unlink(list_head(&vm->frames)), VMFrame, _linked)); }

VMState *
cvm_state_new_from_parse_state(ParseState *state)
{
    VMState *vm = malloc(sizeof(VMState));
    vm->string_pool = state->string_pool;   state->string_pool = NULL;
    vm->young_gen = young_gen_new();
    vm->global = young_gen_new_hash(vm->young_gen, HASH_MIN_CAPACITY, HT_OBJECT);
    list_init(&vm->functions);
    list_init(&vm->frames);

    list_move(&vm->functions, &state->functions);
    VMFunction *main_function = parse_get_main_function(state);
    inst_list_push(
        main_function->inst_list,
        cvm_inst_new_d_type(
            I_HALT,
            0, 0, 0
        )
    );

    list_prepend(&vm->functions, &main_function->_linked);

    cvm_state_push_frame(vm, main_function);

    return vm;
}

VMState *
cvm_state_new(InstList *main_inst_list, StringPool *string_pool)
{
    VMState *vm = malloc(sizeof(VMState));
    vm->string_pool = string_pool;
    vm->young_gen = young_gen_new();
    vm->global = young_gen_new_hash(vm->young_gen, HASH_MIN_CAPACITY, HT_OBJECT);
    list_init(&vm->functions);
    list_init(&vm->frames);

    VMFunction *main_function = malloc(sizeof(VMFunction));
    list_node_init(&main_function->_linked);
    main_function->arguments_nr = 0;
    main_function->register_nr = 65536;
    main_function->capture_list = hash_new(HASH_MIN_CAPACITY);
    hash_set_and_update(main_function->capture_list, 0, value_from_int(0));

    main_function->inst_list = main_inst_list;
    if (!main_inst_list)    main_function->inst_list = inst_list_new(16);
    inst_list_push(
        main_function->inst_list,
        cvm_inst_new_d_type(
            I_HALT,
            0, 0, 0
        )
    );

    list_prepend(&vm->functions, &main_function->_linked);

    cvm_state_push_frame(vm, main_function);

    return vm;
}

void
cvm_state_destroy(VMState *vm)
{
    if (vm->string_pool) {
        string_pool_destroy(vm->string_pool);
    }
    if (vm->young_gen) {
        young_gen_destroy(vm->young_gen);
    }
    while (list_size(&vm->functions)) {
        VMFunction *function = list_get(list_unlink(list_head(&vm->functions)), VMFunction, _linked);
        if (function->capture_list) {
            hash_destroy(function->capture_list);
        }
        if (function->inst_list) {
            inst_list_destroy(function->inst_list);
        }
        free(function);
    }
    while (list_size(&vm->frames)) {
        free(list_get(list_unlink(list_head(&vm->frames)), VMFrame, _linked));
    }
    free(vm);
}

void
cvm_young_gc(VMState *vm)
{
    young_gen_gc_start(vm->young_gen);

    list_for_each(&vm->frames, frame_node) {
        VMFrame *frame = list_get(frame_node, VMFrame, _linked);
        for (size_t i = 0; i <= frame->function->register_nr; ++i) {
            if (value_is_ptr(frame->regs[i])) {
                Hash *hash = value_to_ptr(frame->regs[i]);
                while (hash->type == HT_REDIRECT) {
                    hash = (Hash*)hash->size;
                }
                young_gen_gc_mark(vm->young_gen, &hash);
                frame->regs[i] = value_from_ptr(hash);
            }
        }
    }

    young_gen_gc_end(vm->young_gen);
}

Value
cvm_create_light_function(VMState *vm, light_function func)
{
    Hash *ret = _cvm_allocate_new_hash(vm, HASH_MIN_CAPACITY, HT_LIGHTFUNC);
    ret->hi_func = func;
    return value_from_ptr(ret);
}

void
cvm_register_in_global(VMState *vm, Value value, const char *name)
{
    CString *key = string_pool_insert_str(&vm->string_pool, name);
    _cvm_set_hash(vm, 1, (uintptr_t)key, value);
}

Value
cvm_state_run(VMState *vm)
{
    for (;;) {
        VMFrame *frame = vm_frame_top(vm);
        Inst inst = frame->function->inst_list->insts[frame->pc++];
        switch (inst.type) {
            case I_HALT:
                return value_undefined();
            case I_LI:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(inst.i_imm)
                );
                break;
            case I_ADD:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) +
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SUB:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) -
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_MUL:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) *
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_DIV:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) /
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_MOD:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) %
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SEQ:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) ==
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SLT:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_SLE:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <=
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
                break;
            case I_LNOT:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        !value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
                break;
            case I_BNR:
                if (!value_to_int(cvm_get_register(vm, inst.i_rd))) {
                    frame->pc += inst.i_imm;
                }
                break;
            case I_J:
                frame->pc = (size_t)inst.i_imm;
                break;
            case I_JAL:
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(frame->pc)
                );
                frame->pc = (size_t)inst.i_imm;
                break;
            case I_LSTR:
            {
                CString *string = (CString*)inst.i_imm;
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_string(string)
                );
                break;
            }
            case I_NEW_OBJ:
            {
                Hash *new_obj = _cvm_allocate_new_object(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_obj)
                );
                break;
            }
            case I_NEW_ARR:
            {
                Hash *new_arr = _cvm_allocate_new_array(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_arr)
                );
                break;
            }
            case I_SET_OBJ:
            {
                Hash *obj = _cvm_get_hash_in_register(vm, inst.i_rd);
                uintptr_t key;
                if (obj->type == HT_OBJECT) {
                    key = (uintptr_t)value_to_string(cvm_get_register(vm, inst.i_rt));
                }
                else {
                    key = (uintptr_t)value_to_int(cvm_get_register(vm, inst.i_rt));
                }
                _cvm_set_hash(vm, inst.i_rd, key, cvm_get_register(vm, inst.i_rs));
                break;
            }
            case I_GET_OBJ:
            {
                Hash *obj = _cvm_get_hash_in_register(vm, inst.i_rs);
                uintptr_t key;
                if (obj->type == HT_OBJECT) {
                    key = (uintptr_t)value_to_string(cvm_get_register(vm, inst.i_rt));
                }
                else {
                    key = (uintptr_t)value_to_int(cvm_get_register(vm, inst.i_rt));
                }
                cvm_set_register(
                    vm, inst.i_rd,
                    hash_find(obj, key)
                );
                break;
            }
            case I_CALL:
            {
                Hash *func = _cvm_get_hash_in_register(vm, inst.i_rs);
                if (func->type == HT_LIGHTFUNC) {
                    cvm_set_register(
                        vm, inst.i_rd,
                        func->hi_func(vm, cvm_get_register(vm, inst.i_rt))
                    );
                }
                else if (func->type == HT_CLOSURE) {
                    Hash *args = _cvm_get_hash_in_register(vm, inst.i_rt);

                    cvm_state_push_frame(vm, func->hi_closure);

                    for (size_t i = 0; i <= func->hi_closure->arguments_nr; ++i) {
                        cvm_set_register(vm, i + 1, hash_find(args, i));
                    }

                    hash_for_each(func, captured) {
                        cvm_set_register(vm, captured->key, captured->value);
                    }
                }
                else {
                    type_error(vm, HT_LIGHTFUNC, func->type);
                }
                break;
            }
            case I_RET:
            {
                Value ret_val = cvm_get_register(vm, inst.i_rd);
                cvm_state_pop_frame(vm);
                if (!list_size(&vm->frames)) { return ret_val; }
                Inst original_inst = vm_frame_top(vm)->function->inst_list->insts[vm_frame_top(vm)->pc - 1];
                cvm_set_register(vm, original_inst.i_rd, ret_val);
                break;
            }
            case I_NEW_CLS:
            {
                VMFunction *func = (VMFunction*)inst.i_imm;
                Hash *closure = _cvm_allocate_new_hash(vm, 2 * hash_size(func->capture_list), HT_CLOSURE);
                closure->hi_closure = func;
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(closure)
                );
                // should not trigger gc
                hash_for_each(func->capture_list, capture) {
                    hash_set(
                        closure,
                        (uintptr_t)value_to_int(capture->value),
                        cvm_get_register(vm, capture->key)
                    );
                }
                break;
            }
            case I_UNDEFINED:
                cvm_set_register(vm, inst.i_rd, value_undefined());
                break;
            case I_NULL:
                cvm_set_register(vm, inst.i_rd, value_null());
                break;
            default:
                error_f("Unknown VM instrument (offset %d)", frame->pc - 1);
                break;
        }
    }
}
