//
// Created by c on 4/18/16.
//

#include <stdlib.h>
#include <stdio.h>
#include <error.h>

#include "young_gen.h"
#include "hash_internal.h"
#include "error.h"

struct YoungGenBlock
{
    size_t allocated;
    size_t object_count;
    char content[0];
};

static inline YoungGenBlock *
_young_gen_block_new()
{
    YoungGenBlock *ret = (YoungGenBlock*)aligned_alloc(YOUNG_GEN_BLOCK_SIZE, YOUNG_GEN_BLOCK_SIZE);
    ret->allocated = sizeof(YoungGenBlock);
    ret->object_count = 0;
    return ret;
}

static inline void
_young_gen_block_destroy(YoungGenBlock *block)
{ free(block); }

YoungGen *
young_gen_new()
{
    YoungGen *ret = (YoungGen*)malloc(sizeof(YoungGen));

    ret->current = _young_gen_block_new();
    ret->replacement = _young_gen_block_new();

    return ret;
}

void
young_gen_destroy(YoungGen *young_gen)
{
    _young_gen_block_destroy(young_gen->current);
    _young_gen_block_destroy(young_gen->replacement);
    free(young_gen);
}

static inline void *
_young_gen_allocate(YoungGenBlock *block, size_t size)
{
    if (block->allocated + size > YOUNG_GEN_BLOCK_SIZE) {
        return NULL;
    }
    void *ret = block->content + block->allocated;
    block->allocated += size;
    block->object_count ++;
    return ret;
}

Hash *
young_gen_new_hash(YoungGen *young_gen, size_t capacity, int type)
{
    size_t hash_total_size = _hash_total_size(capacity);
    return hash_init(
            _young_gen_allocate(young_gen->current, hash_total_size),
            capacity, type
        );
}

void
young_gen_gc_start(YoungGen *young_gen)
{
    info_f("start young gen gc, current heap size %d, object count %d",
            young_gen_heap_size(young_gen),
            young_gen_object_nr(young_gen)
    );
    young_gen->gc_start_time = clock();
}

void
young_gen_gc_mark(YoungGen *young_gen, Hash **target)
{
    Hash *hash = *target;

    if ((YoungGenBlock*)((uintptr_t)hash & -YOUNG_GEN_BLOCK_SIZE) != young_gen->current) {
        return;
    }

    if (hash_need_shrink(hash)) {
        size_t new_size = _hash_total_size(_hash_shrink_size(hash));
        *target = _young_gen_allocate(young_gen->replacement, new_size);
        hash_init(*target, _hash_shrink_size(hash), hash->type);
        hash_rehash(*target, hash);
        (*target)->_info = hash->_info;
    }
    else {
        size_t total_size = _hash_total_size(hash->capacity);
        memcpy(
            (*target = _young_gen_allocate(young_gen->replacement, total_size)),
            hash,
            total_size
        );
    }

    hash->type = HT_GC_LEFT;
    hash->capacity = 0;
    hash->size = (size_t)*target;

    hash_for_each(*target, node) {
        if (value_is_ptr(node->value)) {
            Hash *child_hash = value_to_ptr(node->value);

            while (child_hash->type == HT_REDIRECT) {
                child_hash = (Hash*)child_hash->size;
            }

            if (child_hash->type != HT_GC_LEFT) {
                switch (child_hash->type) {
                    case HT_OBJECT:
                    case HT_ARRAY:
                    case HT_LIGHTFUNC:
                        young_gen_gc_mark(young_gen, (Hash**)(&node->value));
                        break;
                    default:
                        assert(0);
                }
            }
        }
    }
}

void
young_gen_gc_end(YoungGen *young_gen)
{
    YoungGenBlock *block = young_gen->current;
    young_gen->current = young_gen->replacement;
    young_gen->replacement = block;

    young_gen->replacement->allocated = sizeof(YoungGenBlock);
    young_gen->replacement->object_count = 0;

    info_f("completed young gen gc in %dms, current heap size %d, object count %d",
            (clock() - young_gen->gc_start_time) / (CLOCKS_PER_SEC / 1000),
            young_gen_heap_size(young_gen),
            young_gen_object_nr(young_gen)
    );
}

size_t
young_gen_heap_size(YoungGen *young_gen)
{ return young_gen->current->allocated; }

size_t
young_gen_object_nr(YoungGen *young_gen)
{ return young_gen->current->object_count; }
