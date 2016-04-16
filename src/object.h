//
// Created by c on 4/4/16.
//

#ifndef CRIPT_OBJECT_H
#define CRIPT_OBJECT_H

#include "hash.h"

typedef struct GCHeader
{
} GCHeader;

typedef struct Object
{
    GCHeader header;
    Hash array_part;
    Hash map_part;
} Object;

#endif //CRIPT_OBJECT_H
