//
// Created by c on 4/4/16.
//

#ifndef CRIPT_VALUE_H
#define CRIPT_VALUE_H

#include <stdint.h>

typedef union Value
{
    intptr_t _int;
} Value;

static inline intptr_t
value_to_int(Value val)
{ return val._int >> 2; }

static inline Value
value_from_int(intptr_t value)
{ Value ret; ret._int = value << 2; return ret; }

#endif //CRIPT_VALUE_H
