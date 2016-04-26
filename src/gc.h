//
// Created by c on 4/25/16.
//

#ifndef CRIPT_GC_H
#define CRIPT_GC_H

#include <stddef.h>
#include <time.h>

#include "hash.h"
#include "list.h"

typedef struct VMState VMState;

#define YOUNG_BLOCK_SIZE    (1024 * 1024)
#define YOUNG_SIZE_LIMIT    (YOUNG_BLOCK_SIZE >> 2)
typedef struct YoungBlock YoungBlock;
typedef struct YoungGC
{
    YoungBlock *current;
    YoungBlock *replace;
} YoungGC;

#define DATA_MAX_SIZE_SHIFT (10)
#define DATA_MAX_SIZE       (1 << DATA_MAX_SIZE_SHIFT)
#define DATA_MIN_SIZE_SHIFT (5)
#define DATA_MIN_SIZE       (1 << DATA_MIN_SIZE_SHIFT)
#define DATA_LEVEL_NR       (DATA_MAX_SIZE_SHIFT - DATA_MIN_SIZE_SHIFT + 1)
#define DATA_BLOCK_SIZE     (1024 * 1024)
typedef struct DataBlock DataBlock;
typedef struct DataGC
{
    LinkedList data_blocks;
    LinkedList free_lists[DATA_LEVEL_NR];
} DataGC;

typedef struct LargeBlock   LargeBlock;
typedef struct LargeGC
{
    Hash *referenced;
    Hash *garbage;
} LargeGC;

#define OLD_BLOCK_SIZE      (32 * 1024 * 1024)
typedef struct OldBlock     OldBlock;
typedef struct OldGC
{
    LinkedList old_blocks;
    size_t object_nr;
} OldGC;

typedef struct GCPool {
    YoungGC young_gen;
    DataGC data_gen;
    LargeGC large_gen;
    OldGC old_gen;
    Hash *to_young_gen;
} GCPool;

GCPool *gc_pool_new();
void gc_pool_destroy(GCPool *pool);

Hash *gc_alloc_hash(GCPool *pool, size_t capacity, int type);
CString *gc_alloc_string(GCPool *pool, size_t size);

void gc_minor_begin(GCPool *pool);
void gc_minor_mark(GCPool *pool, Hash **target);
void gc_minor_end(GCPool *pool);

void gc_major_begin(GCPool *pool);
void gc_major_mark(GCPool *pool, Hash *target);
void gc_major_mark_data(GCPool *pool, void *data);
void gc_major_end(GCPool *pool);

extern inline void gc_write_barriar(GCPool *pool, Hash **dst, Value value);
extern inline void gc_after_expand(GCPool *pool, Hash *old, Hash *new);

#endif //CRIPT_GC_H
