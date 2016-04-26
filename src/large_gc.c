//
// Created by c on 4/26/16.
//

#include <stdlib.h>

#include "large_gc.h"
#include "hash_internal.h"

void
large_gc_init(LargeGC *large)
{
    large->referenced = hash_new(HASH_MIN_CAPACITY);
    large->garbage = NULL;
}

void
large_gc_deinit(LargeGC *large)
{
    hash_for_each(large->referenced, node) {
        free((void*)node->key);
    }

    hash_destroy(large->referenced);
}

Hash *
large_gc_alloc_hash(LargeGC *large, size_t capacity, int type)
{
    capacity = hash_normalize_capacity(capacity);
    size_t total_size = _hash_total_size(capacity) + sizeof(LargeBlock);
    LargeBlock *block = malloc(total_size);
    block->is_string = 0;
    Hash *ret = (Hash*)((size_t)total_size + sizeof(LargeBlock));
    hash_init(ret, capacity, type);
    return ret;
}

CString *
large_gc_alloc_string(LargeGC *large, size_t length)
{
    // TODO
    return NULL;
}

void
large_gc_begin(LargeGC *large)
{
    large->garbage = large->referenced;
    large->referenced = hash_new(HASH_MIN_CAPACITY);
}

static inline int
object_is_large_referenced(LargeGC *large, void *target)
{
    LargeBlock *block = container_of(target, LargeBlock, content);
    Value result = hash_find(large->referenced, (uintptr_t)block);
    return !value_is_undefined(result);
}

static inline int
object_is_large_garbage(LargeGC *large, void *target)
{
    LargeBlock *block = container_of(target, LargeBlock, content);
    Value result = hash_find(large->garbage, (uintptr_t)block);
    return !value_is_undefined(result);
}

void
large_gc_mark(LargeGC *large, void *target)
{
    if (object_is_large_referenced(large, target)) return;
    assert(object_is_large_garbage(large, target));

    LargeBlock *block = container_of(target, LargeBlock, content);

    hash_set(large->garbage, (uintptr_t)block, value_undefined());
    hash_set_and_update(large->referenced, (uintptr_t)block, value_from_ptr(block));

    if (!block->is_string) {
        Hash *hash = (Hash*)target;
        hash_for_each(hash, hash_node) {
            // TODO collect child_hash
        }
    }
}

void
large_gc_end(LargeGC *large)
{
    hash_for_each(large->garbage, hash_node) {
        free((void*)hash_node->key);
    }
    hash_destroy(large->garbage);
    large->garbage = NULL;
}