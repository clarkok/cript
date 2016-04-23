//
// Created by c on 4/7/16.
//

#ifndef CRIPT_HASH_H
#define CRIPT_HASH_H

#include <stddef.h>

#include "value.h"

typedef struct HashNode
{
    uintptr_t key;
    Value value;
} HashNode;

enum HashType
{
    HT_OBJECT,
    HT_ARRAY,
    HT_REDIRECT,
    HT_LIGHTFUNC,
    HT_CLOSURE,
    HT_USERDATA,

    HT_GC_LEFT,
};

static inline const char *
hash_type_to_str(int type)
{
    static const char *LITERAL[] = {
        "object",
        "array",
        "redirect",
        "lightfunc",
        "closure",
        "userdata",
        "gc_left"
    };

    return LITERAL[type];
}

typedef struct VMState VMState;
typedef struct VMFunction VMFunction;

typedef Value (*light_function)(VMState*, Value);
typedef void (*userdata_destructor)(void *);

typedef struct Hash
{
    int type;
    union {
        light_function _func;
        VMFunction *_closure;
        struct {
            void *_data;
            userdata_destructor _destructor;
        } _userdata;
    } _info;
    size_t capacity;
    size_t size;
    HashNode content[0];
} Hash;

#define hi_func     _info._func
#define hi_closure  _info._closure
#define hi_u_data   _info._userdata._data
#define hi_u_dtor   _info._userdata._destructor

#define HASH_MIN_CAPACITY           (8)

#define hash_capacity(hash)         ((hash)->capacity)
#define hash_size(hash)             ((hash)->size)
#define hash_need_expand(hash)      (hash_size(hash) > (hash_capacity(hash) - (hash_capacity(hash) >> 2)))
#define hash_need_shrink(hash)      (hash_capacity(hash) > HASH_MIN_CAPACITY &&     \
                                     hash_size(hash) < (hash_capacity(hash) >> 1))

#define hash_set_and_update(hash, key, value)                                                               \
    do {                                                                                                    \
        hash_set((hash), (uintptr_t)key, value);                                                            \
        if (hash_need_expand(hash)) { Hash *old = hash; (hash) = hash_expand(hash); hash_destroy(old); }    \
        if (hash_need_shrink(hash)) { Hash *old = hash; (hash) = hash_shrink(hash); hash_destroy(old); }    \
    } while (0)

#define hash_for_each(hash, node)                               \
    for (                                                       \
        HashNode *node = (hash)->content,                       \
                 *limit = (hash)->content + (hash)->capacity;   \
        node != limit;                                          \
        ++node                                                  \
    )                                                           \
    if (!value_is_undefined(node->value) && !value_is_null(node->value))

Hash *hash_new(size_t capacity);
void hash_destroy(Hash *hash);

static inline size_t
hash_normalize_capacity(size_t expect_capacity)
{
    if (1 << __builtin_ctz(expect_capacity) == expect_capacity) return expect_capacity;
    else return (1 << ((sizeof(size_t) * 8) - __builtin_clz(expect_capacity)));
}

/**
 * find a value of a key in this hash table
 *
 * @param hash the hash table to find in
 * @param key the key of the value to find
 * @return that value if found, or an Undefined otherwise
 */
Value hash_find(Hash *hash, uintptr_t key);

/**
 * insert, update or remove a value with a key in this hash table
 *
 * @param hash the hash table to operate
 * @param key
 * @param value the new value, if the value is Null of Undefined, then the key is removed
 */
void hash_set(Hash *hash, uintptr_t key, Value value);

/**
 * return a newly expanded hash table, while keep the original hash table unchanged
 */
Hash *hash_expand(Hash *hash);

/**
 * return a newly shrunk hash table, while keep the original hash table unchanged
 */
Hash *hash_shrink(Hash *hash);

void hash_rehash(Hash *dst, Hash *src);

#endif //CRIPT_HASH_H
