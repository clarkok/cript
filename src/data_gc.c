//
// Created by c on 4/25/16.
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "data_gc.h"

#define _data_object_size(level)            \
    (1 << (level + DATA_MIN_SIZE_SHIFT))

#define _data_object_count_per_page(level)  \
    ((DATA_BLOCK_SIZE - sizeof(DataBlock)) * 8 / (_data_object_size(level) * 8 + 1))

#define _data_bitmap_size(level)            \
    ((_data_object_count_per_page(level) + 7) / 8)

#define _data_content_offset(level)         \
    (DATA_BLOCK_SIZE - _data_object_size(level) * _data_object_count_per_page(level))

#define _data_content(block, level)         \
    ((void*)((char*)block) + _data_content_offset(level))

#define _data_block_from_object(obj)        \
    ((DataBlock*)((uintptr_t)obj & (DATA_BLOCK_SIZE - 1)))

static inline DataBlock *
_data_block_new(int level)
{
    DataBlock *ret;
    int result = posix_memalign((void**)&ret, DATA_BLOCK_SIZE, DATA_BLOCK_SIZE);
    assert(result);
    list_node_init(&ret->_linked);
    ret->level = (size_t)level;
    ret->object_nr = 0;
    return ret;
}

void
data_gc_init(DataGC *data)
{
    list_init(&data->data_blocks);
    for (int i = 0; i < DATA_LEVEL_NR; ++i) {
        list_init(data->free_lists + i);
    }
}

void
data_gc_deinit(DataGC *data)
{
    while (list_size(&data->data_blocks)) {
        free(list_get(list_unlink(list_head(&data->data_blocks)), DataBlock, _linked));
    }
}

static inline int
_data_get_level_from_size(size_t size)
{
    assert(size < DATA_MAX_SIZE);
    if (size <= (DATA_MIN_SIZE))    return 0;
    if (1 << __builtin_ctz(size) == size) return __builtin_ctz(size) - DATA_MIN_SIZE_SHIFT;
    else return (sizeof(size_t) * 8) - __builtin_clz(size) - DATA_MIN_SIZE_SHIFT;
}

static inline void
_data_allocate_new_block(DataGC *data, int level)
{
    DataBlock *block = _data_block_new(level);
    list_append(&data->data_blocks, &block->_linked);
    for (
        char *ptr = _data_content(block, level),
             *limit = ((char*)block + DATA_BLOCK_SIZE);
        ptr != limit;
        ptr += _data_object_size(level)
    ) {
        LinkedNode *free_node = (LinkedNode*)ptr;
        list_append(data->free_lists + level, free_node);
    }
}

void *
data_gc_alloc(DataGC *data, size_t size)
{
    int level = _data_get_level_from_size(size);
    if (!list_size((data->free_lists + level))) {
        _data_allocate_new_block(data, level);
    }

    void *ret = list_unlink(list_head(data->free_lists + level));
    DataBlock *block = _data_block_from_object(ret);
    block->object_nr ++;

    return ret;
}

static inline void
_data_set_bitmap(unsigned char *bitmap, size_t index)
{
    size_t offset = index / 8;
    size_t bit = index % 8;

    bitmap[offset] |= (1 << bit);
}

static inline void
_data_unset_bitmap(unsigned char *bitmap, size_t index)
{
    size_t offset = index / 8;
    size_t bit = index % 8;

    bitmap[offset] &= ~(1 << bit);
}

static inline void
_data_unset_for_object(void *object)
{
    DataBlock *block = _data_block_from_object(object);
    _data_unset_bitmap(
        block->bitmap,
        ((size_t)object - (size_t)_data_content(block, block->level)) /
            _data_object_size(block->level)
    );
}

void
data_gc_begin(DataGC *data)
{
    list_for_each(&data->data_blocks, node) {
        DataBlock *block = list_get(node, DataBlock, _linked);
        memset(block->bitmap, 0xFF, _data_bitmap_size(block->level));
    }

    for (int i = 0; i < DATA_LEVEL_NR; ++i) {
        list_for_each(data->free_lists + i, node) {
            _data_unset_for_object(node);
        }
    }
}

void
data_gc_mark(DataGC *data, void *target)
{ _data_unset_for_object(target); }

static inline void *
_data_next_object_from_bitmap(DataBlock *block, size_t index)
{
    assert(block->bitmap[index]);
    size_t offset = __builtin_ctz((uint32_t)block->bitmap[index]);
    block->bitmap[index] &= ~(1 << offset);
    return (void*)((size_t)(_data_content(block, block->level)) +
        _data_object_size(block->level) * (index * 8 + offset));
}

void
data_gc_end(DataGC *data)
{
    list_for_each(&data->data_blocks, node) {
        DataBlock *block = list_get(node, DataBlock, _linked);
        for (size_t i = 0; i < _data_bitmap_size(block->level); ++i) {
            while (block->bitmap[i]) {
                LinkedNode *freed_object = _data_next_object_from_bitmap(block, i);
                list_prepend(data->free_lists + block->level, freed_object);
            }
        }
    }
}

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(0) +
    _data_object_count_per_page(0) * _data_object_size(0) <= DATA_BLOCK_SIZE, "level 0");

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(1) +
    _data_object_count_per_page(1) * _data_object_size(1) <= DATA_BLOCK_SIZE, "level 1");

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(2) +
    _data_object_count_per_page(2) * _data_object_size(2) <= DATA_BLOCK_SIZE, "level 2");

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(3) +
    _data_object_count_per_page(3) * _data_object_size(3) <= DATA_BLOCK_SIZE, "level 3");

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(4) +
    _data_object_count_per_page(4) * _data_object_size(4) <= DATA_BLOCK_SIZE, "level 4");

_Static_assert(
    sizeof(DataBlock) +
    _data_bitmap_size(5) +
    _data_object_count_per_page(5) * _data_object_size(5) <= DATA_BLOCK_SIZE, "level 5");

