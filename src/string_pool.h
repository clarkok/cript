//
// Created by c on 4/16/16.
//

#ifndef CRIPT_CSTRING_H
#define CRIPT_CSTRING_H

#include <string.h>
#include <stdio.h>

#include "hash.h"

typedef struct CString
{
    size_t length;
    char content[0];
} CString;

#define MAX_SHORT_STRING_LENGTH (256)

typedef struct StringPool StringPool;

StringPool *string_pool_new();
void string_pool_destroy(StringPool *pool);

CString *string_pool_find_vec(const StringPool *pool, const char *vec, size_t size);

static inline CString *
string_pool_find_str(const StringPool *pool, const char *str)
{ return string_pool_find_vec(pool, str, strlen(str)); }

CString *string_pool_insert_vec(StringPool **pool_ptr, const char *vec, size_t size);

static inline CString *
string_pool_insert_str(StringPool **pool_ptr, const char *str)
{ return string_pool_insert_vec(pool_ptr, str, strlen(str)); }

void string_pool_dump(FILE *fout, StringPool *pool);

#endif //CRIPT_CSTRING_H
