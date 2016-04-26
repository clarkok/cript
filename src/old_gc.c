//
// Created by c on 4/26/16.
//

#include <stdlib.h>
#include <string.h>
#include "old_gc.h"
#include "hash_internal.h"

#define _old_block_from_object(object)      \
    (OldBlock*)((size_t)object & (OLD_BLOCK_SIZE - 1))

static inline OldBlock *
_old_block_new()
{
    OldBlock *ret;
    int result = posix_memalign((void**)&ret, OLD_BLOCK_SIZE, OLD_BLOCK_SIZE);
    assert(!result);

    list_node_init(&ret->_linked);
    ret->allocated = sizeof(OldBlock);
    ret->object_nr = 0;
    memset(ret->content, 0, OLD_BITMAP_SIZE);

    return ret;
}

void
old_gc_init(OldGC *old)
{
    list_init(&old->old_blocks);
    old->object_nr = 0;

    OldBlock *block = _old_block_new();
    list_append(&old->old_blocks, &block->_linked);
}

void
old_gc_deinit(OldGC *old)
{
    while (list_size(&old->old_blocks)) {
        free(list_get(list_unlink(list_head(&old->old_blocks)), OldBlock, _linked));
    }
}

static inline void*
_old_try_alloc_in_block(OldBlock *block, size_t size)
{
    // to be optimized
    size_t current_bits = 0;
    size_t bit_index = block->allocated / sizeof(void*);
    while (block->allocated < OLD_BLOCK_SIZE) {
        block->allocated += 4;
        if (block->bitmap[bit_index / 8] & (1 << (bit_index % 8))) {
            block->bitmap[bit_index / 8] &= ~(1 << bit_index % 8);
            current_bits = 0;
        }
        else {
            current_bits++;
            if (current_bits == (size / sizeof(void*))) {
                return (void*)((size_t)block + block->allocated - size);
            }
        }
    }
    return NULL;
}

Hash *
old_gc_alloc_hash(OldGC *old, size_t capacity, int type)
{
    capacity = hash_normalize_capacity(capacity);
    size_t total_size = _hash_total_size(capacity);

    list_for_each(&old->old_blocks, block_node) {
        OldBlock *block = list_get(block_node, OldBlock, _linked);
        Hash *ret = _old_try_alloc_in_block(block, total_size);
        if (!ret) continue;
        hash_init(ret, capacity, type);
        block->object_nr++;
        return ret;
    }

    OldBlock *block = _old_block_new();
    list_prepend(&old->old_blocks, &block->_linked);
    Hash *ret = _old_try_alloc_in_block(block, total_size);
    hash_init(ret, capacity, type);
    block->object_nr++;
    return ret;
}

void
old_gc_begin(OldGC *old)
{
    list_for_each(&old->old_blocks, block_node) {
        OldBlock *block = list_get(block_node, OldBlock, _linked);
        block->object_nr = 0;
    }
}

void
old_gc_mark(OldGC *old, Hash *target)
{
    while (target->type == HT_REDIRECT) target = (Hash*)target->size;

    OldBlock *block = _old_block_from_object(target);
    size_t bit_index = ((size_t)target - (size_t)block) / sizeof(void*);
    block->bitmap[bit_index / 8] |= (1 << (bit_index % 8));
    block->object_nr++;

    hash_for_each(target, node) {
        if (value_is_ptr(node->value)) {
            Hash *child = value_to_ptr(node->value);
            gc_major_mark(container_of(old, GCPool, old_gen), &child);
            node->value = value_from_ptr(child);
        }
        else if (value_is_string(node->value)) {
            gc_major_mark_data(container_of(old, GCPool, old_gen), value_to_string(node->value));
        }
    }
}

void
old_gc_end(OldGC *old)
{
    old->object_nr = 0;
    list_for_each(&old->old_blocks, block_node) {
        while (block_node && list_get(block_node, OldBlock, _linked)->object_nr == 0) {
            LinkedNode *node_tobe_freed = block_node;
            block_node = list_next(block_node);
            free(list_unlink(node_tobe_freed));
        }

        old->object_nr += list_get(block_node, OldBlock, _linked)->object_nr;
    }
}