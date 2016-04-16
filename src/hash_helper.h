//
// Created by c on 4/16/16.
//

#ifndef CRIPT_HASH_HELPER_H
#define CRIPT_HASH_HELPER_H

static inline uintptr_t
_next_index(uintptr_t key, size_t capacity)
{ return ((key * 2166136261u) + 16777619u) % capacity; }

static inline size_t
_expand_size(size_t original_capacity)
{ return (original_capacity + (original_capacity >> 1)); }

#endif //CRIPT_HASH_HELPER_H
