//
// Created by c on 4/18/16.
//

#ifndef CRIPT_YOUNG_GC_H
#define CRIPT_YOUNG_GC_H

#include <stddef.h>
#include <time.h>

#include "hash.h"
#include "cvm.h"

#define YOUNG_GEN_BLOCK_SIZE    (1024 * 1024)

typedef struct YoungGenBlock YoungGenBlock;

typedef struct YoungGen
{
    YoungGenBlock *current;
    YoungGenBlock *replacement;
    clock_t gc_start_time;
} YoungGen;

YoungGen *young_gen_new();
void young_gen_destroy(YoungGen *young_gen);

Hash *young_gen_new_hash(YoungGen *young_gen, size_t capacity);

void young_gen_gc_start(YoungGen *young_gen);
void young_gen_gc_mark(YoungGen *young_gen, Hash **target);
void young_gen_gc_end(YoungGen *young_gen);

size_t young_gen_heap_size(YoungGen *young_gen);

#endif // CRIPT_YOUNG_GC_H
