//
// Created by c on 4/4/16.
//

#ifndef CRIPT_VALUE_H
#define CRIPT_VALUE_H

#include <stddef.h>
#include <stdint.h>

typedef union Value
{
    intptr_t _int;
    void *_ptr;
} Value;

enum ValueTag
{
    T_POINTER = 0,
    T_NUMBER = 1,
    T_UNDEFINED = 3
};

static inline int
value_is_int(Value val)
{ return (val._int & 3) != 0; }

static inline intptr_t
value_to_int(Value val)
{ return val._int >> 2; }

static inline Value
value_from_int(intptr_t value)
{ Value ret; ret._int = (value << 2) + T_NUMBER; return ret; }

static inline void *
value_to_ptr(Value val)
{ return (void*)((intptr_t)val._ptr & ~T_POINTER); }

static inline Value
value_from_ptr(void *ptr)
{ Value ret; ret._ptr = (void*)((intptr_t)ptr + T_POINTER); return ret; }

static inline int
value_is_null(Value val)
{ return (!value_is_int(val) && !value_to_ptr(val)); }

static inline Value
value_null()
{ return value_from_ptr(NULL); }

static inline int
value_is_undefined(Value value)
{ return (value._int & T_UNDEFINED) == T_UNDEFINED; }

static inline Value
value_undefined()
{ Value ret; ret._int = T_UNDEFINED; return ret; }

#endif //CRIPT_VALUE_H
