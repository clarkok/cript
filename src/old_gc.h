//
// Created by c on 4/26/16.
//

#ifndef CRIPT_OLD_GC_H
#define CRIPT_OLD_GC_H

#include "gc.h"

#define OLD_BITMAP_SIZE         ((OLD_BLOCK_SIZE / sizeof(void*) + 7) / 8)

typedef struct OldBlock {
    LinkedNode _linked;
    size_t allocated;
    size_t object_nr;
    unsigned char bitmap[OLD_BITMAP_SIZE];
    unsigned char content[0];
} OldBlock;

void old_gc_init(OldGC *old);
void old_gc_deinit(OldGC *old);

Hash *old_gc_alloc_hash(OldGC *old, size_t capacity, int type);

void old_gc_begin(OldGC *old);
void old_gc_mark(OldGC *old, Hash *target);
void old_gc_end(OldGC *old);

#endif //CRIPT_OLD_GC_H
