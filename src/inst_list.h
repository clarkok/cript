//
// Created by c on 4/16/16.
//

#ifndef CRIPT_INST_LIST_H
#define CRIPT_INST_LIST_H

#include <stdlib.h>

#include "inst.h"

typedef struct InstList
{
    size_t capacity;
    size_t count;
    Inst insts[0];
} InstList;

InstList *inst_list_new(size_t capacity);
InstList *inst_list_resize(InstList *list, size_t capacity);
InstList *inst_list_append(InstList *list, Inst inst);
void inst_list_destroy(InstList *list);
#define inst_list_push(list, inst)  list = inst_list_append(list, inst)

#endif //CRIPT_INST_LIST_H
