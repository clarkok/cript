//
// Created by c on 4/25/16.
//

#include <assert.h>
#include <stdlib.h>

#include "young_gc.h"
#include "hash_internal.h"

static inline void
_young_block_init(YoungBlock *block)
{ block->allocated = sizeof(YoungBlock); }

static inline YoungBlock *
_young_block_new()
{
    YoungBlock *ret;
    int result = posix_memalign((void**)&ret, YOUNG_BLOCK_SIZE, YOUNG_BLOCK_SIZE);
    assert(!result);
    _young_block_init(ret);
    return ret;
}

void
young_gc_init(YoungGC *young)
{
    young->current = _young_block_new();
    young->replace = _young_block_new();
}

void
young_gc_deinit(YoungGC *young)
{
    free(young->current);   young->current = NULL;
    free(young->replace);   young->replace = NULL;
}

static inline Hash *
_young_gc_alloc_in_block(YoungBlock *block, size_t size)
{
    Hash *ret = (Hash *)((char*)block->content + block->allocated);
    block->allocated += size;
    return ret;
}

static inline size_t
_young_gc_total_size_by_capacity(size_t capacity)
{
    size_t ret = _hash_total_size(capacity);
    return (size_t)((ret + sizeof(void*) - 1) & -sizeof(void*));
}

Hash *
young_gc_alloc_hash(YoungGC *young, size_t capacity, int type)
{
    capacity = hash_normalize_capacity(capacity);
    size_t total_size = _young_gc_total_size_by_capacity(capacity);

    if (total_size + young->current->allocated > YOUNG_BLOCK_SIZE) { return NULL; }
    Hash *hash = _young_gc_alloc_in_block(young->current, total_size);
    hash_init(hash, capacity, type);
    return hash;
}

void
young_gc_begin(YoungGC *young)
{ _young_block_init(young->replace); }

static inline Hash *
_young_gc_resolve_redirect(Hash *hash)
{
    while (
        (hash->type == HT_GC_LEFT) ||
        (hash->type == HT_REDIRECT)
    ) {
        hash = (Hash*)(hash->size);
    }
    return hash;
}

static inline Hash *
_young_gc_copy_to_replace(YoungGC *young, Hash *hash)
{
    hash = _young_gc_resolve_redirect(hash);
    if (!hash_is_young(young, hash)) { return hash; }
    size_t new_capacity = hash_normalize_capacity(
        hash_need_shrink(hash) ? _hash_shrink_size(hash) : hash_capacity(hash)
    );
    Hash *new_hash = _young_gc_alloc_in_block(
        young->replace,
        _young_gc_total_size_by_capacity(new_capacity)
    );
    hash_init(new_hash, new_capacity, hash->type);
    hash_rehash(new_hash, hash);
    hash->type = HT_GC_LEFT;
    hash->capacity = 0;
    hash->size = (size_t)new_hash;
    return new_hash;
}

void
young_gc_mark(YoungGC *young, Hash **target)
{
    Hash *new_hash = _young_gc_copy_to_replace(young, *target);
    *target = new_hash;

    hash_for_each(new_hash, node) {
        if (!value_is_ptr(node->value)) { continue; }
        Hash *child_hash = _young_gc_resolve_redirect(value_to_ptr(node->value));
        if (hash_is_young(young, child_hash)) {
            young_gc_mark(young, &child_hash);
            node->value = value_from_ptr(child_hash);
        }
    }
}

void
young_gc_end(YoungGC *young)
{
    YoungBlock *block = young->replace;
    young->replace = young->current;
    young->current = block;
}

void
young_gc_mark_other(YoungGC *young)
{
    Hash *hash = (Hash*)young->current->content;
    while ((size_t)hash - (size_t)young->current->content < young->current->allocated) {
        hash_for_each(hash, node) {
            if (value_is_ptr(node->value) && !hash_is_young(young, value_to_ptr(node->value))) {
                gc_major_mark(container_of(young, GCPool, young_gen), value_to_ptr(node->value));
            }
        }
        hash = (Hash*)((size_t)hash + _hash_total_size(hash->capacity));
    }
}
