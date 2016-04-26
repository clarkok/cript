//
// Created by c on 4/25/16.
//

#include <stdlib.h>

#include "gc.h"
#include "young_gc.h"
#include "data_gc.h"
#include "large_gc.h"
#include "hash_internal.h"
#include "old_gc.h"

GCPool *
gc_pool_new()
{
    GCPool *ret = malloc(sizeof(GCPool));

    young_gc_init(&ret->young_gen);
    data_gc_init(&ret->data_gen);
    large_gc_init(&ret->large_gen);
    old_gc_init(&ret->old_gen);
    ret->to_young_gen = hash_new(HASH_MIN_CAPACITY);

    return ret;
}

void
gc_pool_destroy(GCPool *pool)
{
    young_gc_deinit(&pool->young_gen);
    data_gc_deinit(&pool->data_gen);
    large_gc_deinit(&pool->large_gen);
    old_gc_deinit(&pool->old_gen);
    hash_destroy(pool->to_young_gen);

    free(pool);
}

Hash *
gc_alloc_hash(GCPool *pool, size_t capacity, int type)
{
    size_t total_size = _hash_total_size(capacity);
    if (total_size > YOUNG_SIZE_LIMIT) {
        return large_gc_alloc_hash(&pool->large_gen, capacity, type);
    }
    else {
        return young_gc_alloc_hash(&pool->young_gen, capacity, type);
    }
}

void
gc_minor_begin(GCPool *pool)
{
    young_gc_begin(&pool->young_gen);
    hash_for_each(pool->to_young_gen, node) {
        gc_minor_mark(pool, (Hash**)&node->value);
    }
}

void
gc_minor_mark(GCPool *pool, Hash **target)
{
    if (hash_is_young(&pool->young_gen, *target)) {
        young_gc_mark(&pool->young_gen, target);
    }
}

void
gc_minor_end(GCPool *pool)
{
    young_gc_end(&pool->young_gen);
    hash_for_each(pool->to_young_gen, node) {
        Hash *hash = value_to_ptr(node->value);
        while (hash->type == HT_GC_LEFT || hash->type == HT_REDIRECT) {
            hash = (Hash*)hash->size;
        }
        node->value = value_from_ptr(hash);
        *(Hash**)(node->key) = hash;
    }
}

void
gc_major_begin(GCPool *pool)
{
    data_gc_begin(&pool->data_gen);
    large_gc_begin(&pool->large_gen);
    old_gc_begin(&pool->old_gen);

    young_gc_mark_other(&pool->young_gen);
}

void
gc_major_mark(GCPool *pool, Hash *target)
{
    if (object_is_large(&pool->large_gen, target)) {
        large_gc_mark(&pool->large_gen, target);
    }
    else if (!hash_is_young(&pool->young_gen, target)) {
        old_gc_mark(&pool->old_gen, target);
    }
}

void
gc_major_mark_data(GCPool *pool, void *data)
{
    if (object_is_large(&pool->large_gen, data)) {
        large_gc_mark(&pool->large_gen, data);
    }
    else {
        data_gc_mark(&pool->data_gen, data);
    }
}

void
gc_major_end(GCPool *pool)
{
    data_gc_end(&pool->data_gen);
    large_gc_end(&pool->large_gen);
    old_gc_end(&pool->old_gen);
}

extern inline void
gc_write_barriar(GCPool *pool, Hash **dst, Value value)
{
    if (!value_is_ptr(value)) { return; }
    if (value_is_null(value)) { return; }
    Hash *obj = value_to_ptr(value);
    if (hash_is_young(&pool->young_gen, obj)) {
        hash_set_and_update(pool->to_young_gen, (uintptr_t)dst, value_from_ptr(obj));
    }
    else {
        Value result = hash_find(pool->to_young_gen, (uintptr_t)dst);
        if (!value_is_undefined(result)) {
            hash_set(pool->to_young_gen, (uintptr_t)dst, value_undefined());
        }
    }
}

extern inline void
gc_after_expand(GCPool *pool, Hash *old, Hash *new)
{
    if (!hash_is_young(&pool->young_gen, old)) {
        hash_for_each(old, node) {
            if (value_is_ptr(node->value)) {
                if (hash_is_young(&pool->young_gen, value_to_ptr(node->value))) {
                    hash_set(pool->to_young_gen, (uintptr_t)&(node->value), value_undefined());
                }
            }
        }
    }
    if (!hash_is_young(&pool->young_gen, new)) {
        hash_for_each(new, node) {
            if (value_is_ptr(node->value)) {
                if (hash_is_young(&pool->young_gen, value_to_ptr(node->value))) {
                    hash_set(pool->to_young_gen, (uintptr_t)&(node->value), node->value);
                }
            }
        }
    }
}
