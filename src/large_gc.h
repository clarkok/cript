//
// Created by c on 4/26/16.
//

#ifndef CRIPT_LARGE_GC_H
#define CRIPT_LARGE_GC_H

#include "gc.h"
#include "hash.h"
#include "string_pool.h"

typedef struct LargeBlock {
    int is_string;
    char content[0];
} LargeBlock;

void large_gc_init(LargeGC *large);
void large_gc_deinit(LargeGC *large);

Hash *large_gc_alloc_hash(LargeGC *large, size_t capacity, int type);
CString *large_gc_alloc_string(LargeGC *large, size_t length);

static inline int
object_is_large(LargeGC *large, void *target)
{
    LargeBlock *block = container_of(target, LargeBlock, content);
    Value result = hash_find(large->referenced, (uintptr_t)block);
    if (!value_is_undefined(result)) { return 1; }
    result = hash_find(large->garbage, (uintptr_t)block);
    return !value_is_undefined(result);
}

void large_gc_begin(LargeGC *large);
void large_gc_mark(LargeGC *large, void *target);
void large_gc_end(LargeGC *large);

#endif //CRIPT_LARGE_GC_H
