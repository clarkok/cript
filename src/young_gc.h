//
// Created by c on 4/25/16.
//

#ifndef CRIPT_YOUNG_GC_H
#define CRIPT_YOUNG_GC_H

#include "hash.h"
#include "gc.h"

typedef struct YoungBlock
{
    size_t allocated;
    size_t content[0];
} YoungBlock;

static inline int
hash_is_young(YoungGC *young, Hash *hash)
{ return (YoungBlock*)((size_t)hash & (YOUNG_BLOCK_SIZE - 1)) == young->current; }

void young_gc_init(YoungGC *young);
void young_gc_deinit(YoungGC *young);

Hash *young_gc_alloc_hash(YoungGC *young, size_t capacity, int type);
void young_gc_begin(YoungGC *young);
void young_gc_mark(YoungGC *young, Hash **target);
void young_gc_end(YoungGC *young);

void young_gc_mark_other(YoungGC *young);

#endif //CRIPT_YOUNG_GC_H
