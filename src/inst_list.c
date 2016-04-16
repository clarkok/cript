//
// Created by c on 4/16/16.
//

#include <string.h>

#include "inst_list.h"

#define CVM_LIST_DEFAULT_CAPACITY 16

static inline size_t
cvm_list_insts_size(size_t capacity)
{ return capacity * sizeof(Inst); }

static inline size_t
cvm_list_resize_capacity(size_t original)
{ return original ? (original + (original >> 1)) : CVM_LIST_DEFAULT_CAPACITY; }

InstList *
cvm_list_new(size_t capacity)
{
    size_t insts_size = cvm_list_insts_size(capacity);

    InstList *ret = malloc(sizeof(InstList) + insts_size);
    if (!ret) return NULL;

    memset((char *)(ret) + sizeof(InstList), 0, insts_size);

    ret->capacity = capacity;
    ret->count = 0;

    return ret;
}

InstList *
cvm_list_resize(InstList *list, size_t capacity)
{
    size_t insts_size = cvm_list_insts_size(capacity);
    size_t original_size = cvm_list_insts_size(list->capacity);

    list = realloc(list, sizeof(InstList) + insts_size);
    if (capacity > list->capacity) {
        memset(
            (char*)(list) + original_size + sizeof(InstList),
            0,
            insts_size - original_size
        );
    }
    list->capacity = capacity;

    return list;
}

InstList *
cvm_list_append(InstList *list, Inst inst)
{
    if (list->count == list->capacity) {
        list = cvm_list_resize(list, cvm_list_resize_capacity(list->capacity));
    }

    list->insts[list->count++] = inst;
    return list;
}

void
cvm_list_destroy(InstList *list)
{ free(list); }

