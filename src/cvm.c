//
// Created by c on 4/4/16.
//

#include <stdlib.h>

#include "cript.h"
#include "cvm.h"
#include "error.h"
#include "young_gen.h"
#include "hash_internal.h"

#include "inst_output.h"

#define type_error(vm, expected)            \
    error_f("%s", "TypeError: expected " expected)

#define hash_type_error(vm, expected, actual)       \
    error_f("TypeError: expected %s, but met %s\n", \
        hash_type_to_str(expected),                 \
        hash_type_to_str(actual))

#define vm_scene_top(vm)    \
    list_get(list_head(&vm->scenes), VMScene, _linked)

#define vm_frame_top(vm)    \
    list_get(list_head(&(vm_scene_top(vm)->frames)), VMFrame, _linked)

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

#define _cvm_try_alloc(vm, ret, expr)                                       \
    do {                                                                    \
        ret = expr;                                                         \
        if (!ret) {                                                         \
            cvm_young_gc(vm);                                               \
            ret = expr;                                                     \
            if (!ret) {                                                     \
                error_f("Out of memory when trying to exec %s", #expr);     \
            }                                                               \
        }                                                                   \
    } while (0)

static inline Hash *
_cvm_allocate_new_hash(VMState *vm, size_t capacity, int type)
{
    Hash *hash;
    _cvm_try_alloc(vm, hash, young_gen_new_hash(vm->young_gen, capacity, type));
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
    Value val = cvm_get_register(vm, reg);
    if (!value_is_ptr(val) || value_is_null(val)) {
        type_error(vm, "Object");
    }
    Hash *hash = value_to_ptr(val);
    if (hash->type == HT_REDIRECT) {
        while(hash->type == HT_REDIRECT) {
            hash = (Hash*)(hash->size);
        }
        cvm_set_register(vm, reg, value_from_ptr(hash));
    }
    return hash;
}

static inline Hash *
_cvm_set_hash(VMState *vm, Hash *hash, uintptr_t key, Value val)
{
    hash_set(hash, key, val);
    if (hash_need_expand(hash)) {
        Hash *new_hash = _cvm_allocate_new_hash(vm, _hash_expand_size(hash), hash->type);
        if (hash->type == HT_GC_LEFT) {
            hash = (Hash*)hash->size;
        }
        hash_rehash(new_hash, hash);
        hash->type = HT_REDIRECT;
        hash->capacity = 0;
        hash->size = (size_t)new_hash;

        hash = new_hash;
    }
    return hash;
}

static inline void
_cvm_set_hash_in_register(VMState *vm, size_t reg, uintptr_t key, Value val)
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
    VMFrame *frame = malloc(sizeof(VMFrame) + (function->register_nr + 1) * sizeof(Value));
    frame->function = function;
    frame->pc = 0;
    frame->regs[0] = value_from_int(0);
    frame->regs[1] = value_from_ptr(vm->global);
    for (size_t i = 2; i <= function->register_nr; ++i) {
        frame->regs[i] = value_undefined();
    }

    list_prepend(&vm_scene_top(vm)->frames, &frame->_linked);
}

static inline void
cvm_state_pop_frame(VMState *vm)
{ free(list_get(list_unlink(list_head(&vm_scene_top(vm)->frames)), VMFrame, _linked)); }

static inline void
_cvm_push_scene(VMState *vm)
{
    VMScene *scene = malloc(sizeof(VMScene));
    list_init(&scene->frames);
    scene->external = _cvm_allocate_new_array(vm, HASH_MIN_CAPACITY);
    list_prepend(&vm->scenes, &scene->_linked);
}

static inline void
_cvm_pop_scene(VMState *vm)
{
    VMScene *scene = vm_scene_top(vm);
    list_unlink(&scene->_linked);
    while (list_size(&scene->frames)) {
        free(list_get(list_unlink(list_head(&scene->frames)), VMFrame, _linked));
    }
    free(scene);
}

VMState *
cvm_state_new_from_parse_state(ParseState *state)
{
    VMState *vm = malloc(sizeof(VMState));
    vm->string_pool = state->string_pool;   state->string_pool = NULL;
    vm->young_gen = young_gen_new();
    vm->global = young_gen_new_hash(vm->young_gen, HASH_MIN_CAPACITY, HT_OBJECT);
    list_init(&vm->functions);
    list_init(&vm->scenes);

    _cvm_push_scene(vm);

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

Value
cvm_state_import_from_parse_state(VMState *vm, ParseState *state)
{
    vm->string_pool = state->string_pool;   state->string_pool = NULL;

    _cvm_push_scene(vm);
    while (list_size(&state->functions)) {
        list_append(&vm->functions, list_unlink(list_head(&state->functions)));
    }
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
    Value val = cvm_state_run(vm);
    _cvm_pop_scene(vm);

    return val;
}

VMState *
cvm_state_new(InstList *main_inst_list, StringPool *string_pool)
{
    VMState *vm = malloc(sizeof(VMState));
    vm->string_pool = string_pool;
    vm->young_gen = young_gen_new();
    vm->global = young_gen_new_hash(vm->young_gen, HASH_MIN_CAPACITY, HT_OBJECT);
    list_init(&vm->functions);
    list_init(&vm->scenes);

    _cvm_push_scene(vm);

    VMFunction *main_function = malloc(sizeof(VMFunction));
    list_node_init(&main_function->_linked);
    main_function->arguments_nr = 0;
    main_function->register_nr = 65535;
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
    while (list_size(&vm->scenes)) {
        VMScene *scene = vm_scene_top(vm);
        list_unlink(&scene->_linked);
        while (list_size(&scene->frames)) {
            free(list_get(list_unlink(list_head(&scene->frames)), VMFrame, _linked));
        }
        free(scene);
    }
    free(vm);
}

void
cvm_young_gc(VMState *vm)
{
    young_gen_gc_start(vm->young_gen);

    young_gen_gc_mark(vm->young_gen, &vm->global);

    list_for_each(&vm->scenes, scene_node) {
        VMScene *scene = list_get(scene_node, VMScene, _linked);
        young_gen_gc_mark(vm->young_gen, &scene->external);
        list_for_each(&scene->frames, frame_node) {
            VMFrame *frame = list_get(frame_node, VMFrame, _linked);
            for (size_t i = 0; i <= frame->function->register_nr; ++i) {
                if (value_is_ptr(frame->regs[i]) && !value_is_null(frame->regs[i])) {
                    Hash *hash = value_to_ptr(frame->regs[i]);
                    while (hash->type == HT_REDIRECT) {
                        hash = (Hash*)hash->size;
                    }
                    young_gen_gc_mark(vm->young_gen, &hash);
                    frame->regs[i] = value_from_ptr(hash);
                }
            }
        }
    }

    young_gen_gc_end(vm->young_gen);
}

Value
cvm_create_light_function(VMState *vm, light_function func)
{
    Hash *ret = _cvm_allocate_new_hash(vm, 0, HT_LIGHTFUNC);
    ret->hi_func = func;
    return value_from_ptr(ret);
}

Value
cvm_create_userdata(VMState *vm, void *data, userdata_destructor destructor)
{
    Hash *ret;
    _cvm_try_alloc(vm, ret, young_gen_new_userdata(vm->young_gen, data, destructor));
    return value_from_ptr(ret);
}

void
cvm_register_in_global(VMState *vm, Value value, const char *name)
{
    CString *key = string_pool_insert_str(&vm->string_pool, name);
    _cvm_set_hash_in_register(vm, 1, (uintptr_t) key, value);
}

static inline void
_cvm_setup_function_frame(VMState *vm, Hash *closure, Hash *args)
{
    for (size_t i = 0; i <= closure->hi_closure->arguments_nr; ++i) {
        cvm_set_register(vm, i + 1, hash_find(args, i));
    }

    hash_for_each(closure, captured) {
        cvm_set_register(vm, captured->key, captured->value);
    }
}

Value
cvm_state_call_function(VMState *vm, Value func_val, Value args_val)
{
    if (!value_is_ptr(func_val) || !value_to_ptr(args_val)) {
        type_error(vm, "function");
        return value_undefined();
    }

    Hash *func = value_to_ptr(func_val);

    if (func->type == HT_LIGHTFUNC) {
        return func->hi_func(vm, args_val);
    }
    else {
        Hash *args = value_to_ptr(args_val);

        _cvm_push_scene(vm);

        cvm_state_push_frame(vm, func->hi_closure);
        _cvm_setup_function_frame(vm, func, args);

        Value ret = cvm_state_run(vm);

        _cvm_pop_scene(vm);

        return ret;
    }
}

Hash *
cvm_get_global(VMState *vm)
{ return vm->global; }

Hash *
cvm_create_object(VMState *vm, size_t capacity)
{
    Hash *hash = _cvm_allocate_new_hash(vm, capacity, HT_OBJECT);
    vm_scene_top(vm)->external = _cvm_set_hash(
        vm,
        vm_scene_top(vm)->external,
        hash_size(vm_scene_top(vm)->external),
        value_from_ptr(hash)
    );
    return hash;
}

Hash *
cvm_create_array(VMState *vm, size_t capacity)
{
    Hash *hash = _cvm_allocate_new_hash(vm, capacity, HT_ARRAY);
    vm_scene_top(vm)->external = _cvm_set_hash(
        vm,
        vm_scene_top(vm)->external,
        hash_size(vm_scene_top(vm)->external),
        value_from_ptr(hash)
    );
    return hash;
}

Hash *
cvm_set_hash(VMState *vm, Hash *hash, uintptr_t key, Value val)
{ return _cvm_set_hash(vm, hash, key, val); }

Value
cvm_state_run(VMState *vm)
{
#if USE_COMPUTED_GOTO
    static void *LABEL_CONSTANT[] = {
        &&i_unknown,
        &&i_halt,
        &&i_li,
        &&i_add,
        &&i_sub,
        &&i_mul,
        &&i_div,
        &&i_mod,
        &&i_seq,
        &&i_slt,
        &&i_sle,
        &&i_lnot,
        &&i_mov,
        &&i_bnr,
        &&i_j,
        &&i_lstr,
        &&i_new_obj,
        &&i_new_arr,
        &&i_set_obj,
        &&i_get_obj,
        &&i_call,
        &&i_ret,
        &&i_new_cls,
        &&i_undefined,
        &&i_null
    };
#endif

#if USE_COMPUTED_GOTO
    #define DISPATCH()                                              \
    frame = vm_frame_top(vm);                                       \
    inst = frame->function->inst_list->insts[frame->pc++];          \
    goto *LABEL_CONSTANT[inst.type]

        VMFrame *frame;
        Inst inst;
        DISPATCH();
#else
    for (;;) {
        VMFrame *frame = vm_frame_top(vm);
        Inst inst = frame->function->inst_list->insts[frame->pc++];
        switch (inst.type) {
#endif

#if USE_COMPUTED_GOTO
            i_halt:
#else
            case I_HALT:
#endif
                return value_undefined();

#if USE_COMPUTED_GOTO
            i_li:
#else
            case I_LI:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(inst.i_imm)
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_add:
#else
                break;
            case I_ADD:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) +
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_sub:
#else
                break;
            case I_SUB:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) -
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_mul:
#else
                break;
            case I_MUL:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) *
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_div:
#else
                break;
            case I_DIV:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) /
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_mod:
#else
                break;
            case I_MOD:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) %
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_seq:
#else
                break;
            case I_SEQ:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        cvm_get_register(vm, inst.i_rs)._int ==
                        cvm_get_register(vm, inst.i_rt)._int
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_slt:
#else
                break;
            case I_SLT:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_sle:
#else
                break;
            case I_SLE:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        value_to_int(cvm_get_register(vm, inst.i_rs)) <=
                        value_to_int(cvm_get_register(vm, inst.i_rt))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_lnot:
#else
                break;
            case I_LNOT:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_int(
                        !value_to_int(cvm_get_register(vm, inst.i_rs))
                    )
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_mov:
#else
                break;
            case I_MOV:
#endif
                cvm_set_register(
                    vm, inst.i_rd,
                    cvm_get_register(vm, inst.i_rs)
                );
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_bnr:
#else
                break;
            case I_BNR:
#endif
                {
                    Value val = cvm_get_register(vm, inst.i_rd);
                    if (
                        value_is_undefined(val) ||
                        value_is_null(val) ||
                        (value_is_int(val) && !value_to_int(val))
                    ) {
                        frame->pc += inst.i_imm;
                    }
                }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_j:
#else
                break;
            case I_J:
#endif
                frame->pc = (size_t)inst.i_imm;
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_lstr:
#else
                break;
            case I_LSTR:
#endif
            {
                CString *string = (CString*)inst.i_imm;
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_string(string)
                );
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_new_obj:
#else
                break;
            case I_NEW_OBJ:
#endif
            {
                Hash *new_obj = _cvm_allocate_new_object(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_obj)
                );
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_new_arr:
#else
                break;
            case I_NEW_ARR:
#endif
            {
                Hash *new_arr = _cvm_allocate_new_array(vm, HASH_MIN_CAPACITY);
                cvm_set_register(
                    vm, inst.i_rd,
                    value_from_ptr(new_arr)
                );
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_set_obj:
#else
                break;
            case I_SET_OBJ:
#endif
            {
                Hash *obj = _cvm_get_hash_in_register(vm, inst.i_rd);
                uintptr_t key;
                if (obj->type == HT_OBJECT) {
                    key = (uintptr_t)value_to_string(cvm_get_register(vm, inst.i_rt));
                }
                else {
                    key = (uintptr_t)value_to_int(cvm_get_register(vm, inst.i_rt));
                }
                _cvm_set_hash_in_register(vm, inst.i_rd, key, cvm_get_register(vm, inst.i_rs));
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_get_obj:
#else
                break;
            case I_GET_OBJ:
#endif
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
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_call:
#else
                break;
            case I_CALL:
#endif
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
                    _cvm_setup_function_frame(vm, func, args);
                }
                else {
                    hash_type_error(vm, HT_LIGHTFUNC, func->type);
                }
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_ret:
#else
                break;
            case I_RET:
#endif
            {
                Value ret_val = cvm_get_register(vm, inst.i_rd);
                cvm_state_pop_frame(vm);
                if (!list_size(&vm_scene_top(vm)->frames)) { return ret_val; }
                Inst original_inst = vm_frame_top(vm)->function->inst_list->insts[vm_frame_top(vm)->pc - 1];
                cvm_set_register(vm, original_inst.i_rd, ret_val);
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_new_cls:
#else
                break;
            case I_NEW_CLS:
#endif
            {
                VMFunction *func = (VMFunction*)inst.i_imm;
                Hash *closure = _cvm_allocate_new_hash(vm, hash_size(func->capture_list), HT_CLOSURE);
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
            }
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_undefined:
#else
                break;
            case I_UNDEFINED:
#endif
                cvm_set_register(vm, inst.i_rd, value_undefined());
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_null:
#else
                break;
            case I_NULL:
#endif
                cvm_set_register(vm, inst.i_rd, value_null());
#if USE_COMPUTED_GOTO
                DISPATCH();
            i_unknown:
#else
                break;
            default:
#endif
                error_f("Unknown VM instrument (offset %d)", frame->pc - 1);
                return value_undefined();
#if USE_COMPUTED_GOTO
#undef DISPATCH
#else
        }
    }
#endif
}
