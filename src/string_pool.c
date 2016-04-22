//
// Created by c on 4/16/16.
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "string_pool.h"
#include "hash_helper.h"

#define STRING_PAGE_SIZE                4096
#define STRING_POOL_DEFAULT_CAPACITY    16

typedef struct StringPage
{
    LinkedNode _linked;
    size_t allocated;
    char content[0];
} StringPage;

typedef struct StringNode
{
    uintptr_t key;
    CString *string;
} StringNode;

struct StringPool
{
    LinkedList string_pages;
    size_t capacity;
    size_t size;
    StringNode content[0];
};

static inline uintptr_t
string_hash_vec(const char *vec, size_t size)
{
    uintptr_t ret = size;

    while (size > sizeof(size_t)) {
        ret = (ret << 7) + ret;
        ret ^= *(size_t*)(vec);

        vec  += sizeof(size_t);
        size -= sizeof(size_t);
    }

    while (size--) {
        ret = (ret << 3) + ret;
        ret ^= *vec++;
    }

    return ret;
}

static inline uintptr_t
_string_next_index(uintptr_t key, size_t capacity)
{ return _next_index(key, capacity); }

static inline size_t
_string_expand_size(size_t original_capacity)
{ return _expand_size((original_capacity)); }

static inline int
_string_hash_need_expand(size_t size, size_t capacity)
{ return size > (capacity - (capacity >> 1)); }

static inline void
_string_hash_insert(StringPool *pool, uintptr_t key, CString *string)
{
    uintptr_t index = key % pool->capacity;
    StringNode *node = NULL;

    while ((node = pool->content + index)->string) {
        index = _string_next_index(index, pool->capacity);
    }

    node->key = key;
    node->string = string;
}

static inline StringPage *
_new_string_page()
{
    StringPage *ret = (StringPage*)malloc(STRING_PAGE_SIZE);
    list_node_init(&ret->_linked);
    ret->allocated = sizeof(StringPage);
    return ret;
}

#define _string_pool_hash_size(capacity)    (sizeof(StringNode) * (capacity))
#define _string_pool_size(capacity)         (sizeof(StringPool) + _string_pool_hash_size(capacity))

static inline StringPool *
_string_pool_construct(size_t capacity)
{
    StringPool *ret = (StringPool*)malloc(_string_pool_size(capacity));

    list_init(&ret->string_pages);
    ret->capacity = capacity;
    ret->size = 0;
    memset(ret->content, 0, _string_pool_hash_size(capacity));

    StringPage *first_page = _new_string_page();
    list_append(&ret->string_pages, &first_page->_linked);

    return ret;
}

StringPool *
string_pool_new()
{ return _string_pool_construct(STRING_POOL_DEFAULT_CAPACITY); }

void
string_pool_destroy(StringPool *pool)
{
    while (list_size(&pool->string_pages)) {
        free(list_get(list_unlink(list_head(&pool->string_pages)), StringPage, _linked));
    }
    free(pool);
}

StringPool *
_string_expand_pool(StringPool *original)
{
    StringPool *ret = _string_pool_construct(_string_expand_size(original->capacity));

    list_move(&ret->string_pages, &original->string_pages);

    StringNode *node = original->content,
               *limit = original->content + original->capacity;
    for (; node != limit; ++node) {
        if (node->string) {
            _string_hash_insert(ret, node->key, node->string);
        }
    }

    free(original);
    return ret;
}

CString *
_string_pool_find_hash(const StringPool *pool, const char *vec, size_t size, uintptr_t hash)
{
    const StringNode *node = NULL;
    uintptr_t index = hash % pool->capacity;
    while ((node = pool->content + index)->string) {
        if (node->key == hash && size == node->string->length && !memcmp(vec, node->string->content, size)) {
            return node->string;
        }
        index = _string_next_index(index, pool->capacity);
    }
    return NULL;
}

CString *
string_pool_find_vec(const StringPool *pool, const char *vec, size_t size)
{
    uintptr_t hash = string_hash_vec(vec, size);
    return _string_pool_find_hash(pool, vec, size, hash);
}

static inline CString *
_string_allocate_in_pool(StringPool *pool, size_t size)
{
    StringPage *page = list_get(list_head(&pool->string_pages), StringPage, _linked);
    if (STRING_PAGE_SIZE - page->allocated < size + sizeof(CString)) {
        page = _new_string_page();
        list_prepend(&pool->string_pages, &page->_linked);
    }
    CString *ret = (CString*)((char *)page + page->allocated);
    page->allocated += (size + sizeof(CString) + sizeof(size_t)) & -sizeof(size_t); // for alignment
    return ret;
}

CString *
string_pool_insert_vec(StringPool **pool_ptr, const char *vec, size_t size)
{
    assert(size < MAX_SHORT_STRING_LENGTH);

    StringPool *pool = *pool_ptr;
    uintptr_t hash = string_hash_vec(vec, size);

    CString *find_result = _string_pool_find_hash(pool, vec, size, hash);
    if (find_result) { return find_result; }

    if (_string_hash_need_expand(pool->size + 1, pool->capacity)) {
        pool = *pool_ptr = _string_expand_pool(pool);
    }

    CString *string = _string_allocate_in_pool(pool, size + 1);
    string->length = size;
    memcpy(string->content, vec, size);
    string->content[string->length] = 0;    // tailing zero

    _string_hash_insert(pool, hash, string);

    pool->size++;
    return string;
}

void
string_pool_dump(FILE *fout, StringPool *pool)
{
    StringNode *node = pool->content,
               *limit = pool->content + pool->capacity;

    for (; node != limit; ++node) {
        if (node->string) {
            fprintf(fout, "0x%x(%d): %.*s\n",
                    (uint)node->string,
                    (uint)node->string,
                    node->string->length,
                    node->string->content
            );
        }
    }
}
