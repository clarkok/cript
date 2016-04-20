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
    unsigned char bitmap[(YOUNG_GEN_BLOCK_SIZE / sizeof(void*) + 7) / 8];
    unsigned char content[0];
};

static inline void
_young_gen_set_bitmap(YoungGenBlock *block, size_t offset)
{
    size_t index = (offset / sizeof(void*) / 8),
           bit = (offset / sizeof(void*)) & 7;
    block->bitmap[index] |= 1 << bit;
}

static inline void
_young_gen_unset_bitmap(YoungGenBlock *block, size_t offset)
{
    size_t index = (offset / sizeof(void*) / 8),
           bit = offset & 7;
    block->bitmap[index] &= ~(1 << bit);
}

static inline Hash *
_young_gen_get_last_hash_from_bitmap(YoungGenBlock *block, size_t index, unsigned char *bitmap)
{
    if (!*bitmap) { return NULL; }

    size_t last_bit = __builtin_ctz(*bitmap);
    *bitmap &= ~(1 << last_bit);

    return (Hash*)((size_t)block + (index * 8 + last_bit) * sizeof(void*));
}

static inline YoungGenBlock *
_young_gen_block_new()
{
    YoungGenBlock *ret = (YoungGenBlock*)aligned_alloc(YOUNG_GEN_BLOCK_SIZE, YOUNG_GEN_BLOCK_SIZE);
    ret->allocated = sizeof(YoungGenBlock);
    ret->object_count = 0;
    memset(ret->bitmap, 0, sizeof(ret->bitmap));
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

Hash *
young_gen_new_userdata(YoungGen *young_gen, void *data, userdata_destructor destructor)
{
    size_t total_size = _hash_total_size(0);
    Hash *ret = _young_gen_allocate(young_gen->current, total_size);
    if (!ret) { return NULL; }
    ret->type = HT_USERDATA;
    ret->capacity = 0;
    ret->size = 0;
    ret->hi_u_data = data;
    ret->hi_u_dtor = destructor;
    _young_gen_set_bitmap(young_gen->current, (size_t)ret - (size_t)young_gen->current);
    return ret;
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
    if (hash->type == HT_GC_LEFT) {
        *target = (Hash*)hash->size;
        return;
    }

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

        if (hash->type == HT_USERDATA) {
            _young_gen_unset_bitmap(young_gen->current, (size_t)hash - (size_t)young_gen->current);
            _young_gen_set_bitmap(young_gen->replacement, (size_t)*target - (size_t)young_gen->replacement);
        }
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
                    case HT_CLOSURE:
                    case HT_LIGHTFUNC:
                    case HT_USERDATA:
                        young_gen_gc_mark(young_gen, (Hash**)(&node->value));
                        break;
                    default:
                        assert(0);
                }
            }
            else {
                node->value = value_from_ptr((Hash*)child_hash->size);
            }
        }
    }
}

static inline void
_young_gen_gc_destruct_userdata(YoungGenBlock *block)
{
    for (size_t i = 0; i < sizeof(block->bitmap) / sizeof(block->bitmap[0]); ++i) {
        Hash *hash = NULL;
        while ((hash = _young_gen_get_last_hash_from_bitmap(block, i, block->bitmap + i))) {
            if (hash->type == HT_USERDATA) {
                hash->hi_u_dtor(hash->hi_u_data);
            }
        }
    }
}

void
young_gen_gc_end(YoungGen *young_gen)
{
    _young_gen_gc_destruct_userdata(young_gen->current);

    YoungGenBlock *block = young_gen->current;
    young_gen->current = young_gen->replacement;
    young_gen->replacement = block;

    young_gen->replacement->allocated = sizeof(YoungGenBlock);
    young_gen->replacement->object_count = 0;
    memset(young_gen->replacement->bitmap, 0, sizeof(young_gen->replacement->bitmap));

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
