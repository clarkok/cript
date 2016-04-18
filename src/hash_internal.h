//
// Created by c on 4/18/16.
//

#ifndef CRIPT_HASH_INTERNAL_H
#define CRIPT_HASH_INTERNAL_H

#include "hash.h"

Hash *hash_init(void *hash, size_t capacity);

static inline size_t
_hash_content_size(size_t capacity)
{ return capacity * sizeof(HashNode); }

static inline size_t
_hash_total_size(size_t capacity)
{ return sizeof(Hash) + _hash_content_size(capacity); }

static inline uintptr_t
_hash_next_index(uintptr_t key, size_t capacity)
{ return _next_index(key, capacity); }

static inline size_t
_hash_expand_size(Hash *hash)
{ return _expand_size(hash->capacity); }

static inline size_t
_hash_shrink_size(Hash *hash)
{ return (hash->size + (hash->size >> 1)); }

#endif //CRIPT_HASH_INTERNAL_H
