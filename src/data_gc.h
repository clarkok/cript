//
// Created by c on 4/25/16.
//

#ifndef CRIPT_DATA_GC_H
#define CRIPT_DATA_GC_H

#include "gc.h"

typedef struct DataBlock {
    LinkedNode _linked;
    size_t level;
    size_t object_nr;
    unsigned char bitmap[0];
} DataBlock;

void data_gc_init(DataGC *data);
void data_gc_deinit(DataGC *data);

void *data_gc_alloc(DataGC *data, size_t size);

void data_gc_begin(DataGC *data);
void data_gc_mark(DataGC *data, void *target);
void data_gc_end(DataGC *data);

#endif //CRIPT_DATA_GC_H
