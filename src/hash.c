//
// Created by c on 4/7/16.
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash.h"
#include "hash_helper.h"

/**
 * Hash
 *
 * if value in a slot is null, means open hash chain ends here
 * if value in a slot is undefined, means this slot is deleted
 */

static inline size_t
_hash_content_size(size_t capacity)
{ return capacity * sizeof(HashNode); }

static inline uintptr_t
_hash_next_index(uintptr_t key, size_t capacity)
{ return _next_index(key, capacity); }

static inline size_t
_hash_expand_size(Hash *hash)
{ return _expand_size(hash->capacity); }

static inline size_t
_hash_shrink_size(Hash *hash)
{ return (hash->size + (hash->size >> 1)); }

Hash *
hash_new(size_t capacity)
{
    if (capacity < HASH_MIN_CAPACITY)   capacity = HASH_MIN_CAPACITY;

    size_t content_size = _hash_content_size(capacity);
    Hash *ret = malloc(sizeof(Hash) + content_size);

    ret->capacity = capacity;
    ret->size = 0;

    memset((char*)ret + sizeof(Hash), 0, content_size);

    return ret;
}

void
hash_destroy(Hash *hash)
{ free(hash); }

Value
hash_find(Hash *hash, uintptr_t key)
{
    uintptr_t index = key % hash->capacity;

    while (hash->content[index].key != key && !value_is_null(hash->content[index].value)) {
        index = _hash_next_index(index, hash->capacity);
    }

    if (value_is_null(hash->content[index].value)) {
        return value_undefined();
    }
    else {
        return hash->content[index].value;
    }
}

void
hash_set(Hash *hash, uintptr_t key, Value value)
{
    uintptr_t index = key % hash->capacity;
    uintptr_t slot = hash->capacity;

    if (value_is_null(value)) {
        value = value_undefined();
    }

    while (hash->content[index].key != key && !value_is_null(hash->content[index].value)) {
        if (slot == hash->capacity && value_is_undefined(hash->content[index].value)) {
            slot = index;
        }
        index = _hash_next_index(index, hash->capacity);
    }

    if (hash->content[index].key == key)    { slot = index; }
    if (slot == hash->capacity)             { slot = index; }

    if (
        !value_is_undefined(value) &&
        (value_is_null(hash->content[slot].value) ||
         value_is_undefined(hash->content[slot].value))
    ) {
        assert(hash->size != hash->capacity);
        hash->size++;
    }

    if (
        value_is_undefined(value) &&
        !value_is_undefined(hash->content[slot].value) &&
        !value_is_null(hash->content[slot].value)
    ) {
        assert(hash->size);
        hash->size--;
    }

    hash->content[slot].value = value;
    hash->content[slot].key = key;
}

static inline void
_hash_rehash(Hash *dst, Hash *src)
{
    HashNode *ptr, *limit;
    for (
        ptr = src->content, limit = src->content + src->capacity;
        ptr != limit;
        ++ptr
    ) {
        if (!value_is_undefined(ptr->value) && !value_is_null(ptr->value))
            hash_set(dst, ptr->key, ptr->value);
    }
}

Hash *
hash_expand(Hash *hash)
{
    Hash *ret = hash_new(_hash_expand_size(hash));
    _hash_rehash(ret, hash);
    return ret;
}

Hash *
hash_shrink(Hash *hash)
{
    Hash *ret = hash_new(_hash_shrink_size(hash));
    _hash_rehash(ret, hash);
    return ret;
}

