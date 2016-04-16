//
// Created by c on 4/4/16.
//

#ifndef CRIPT_LIST_H
#define CRIPT_LIST_H

#include <stddef.h>

#ifndef container_of
#define container_of(ptr, type, mem)                        \
        ({ const typeof(((type*)0)->mem) *__mptr = (ptr);   \
           (type*)((char*)__mptr - offsetof(type, mem)); })
#endif

typedef struct LinkedNode
{
    struct LinkedNode **prev;
    struct LinkedNode *next;
    struct LinkedList *list;
} LinkedNode;

typedef struct LinkedList
{
    struct LinkedNode *head;
    struct LinkedNode **tail;
    size_t size;
} LinkedList;

#define list_get(ptr, type, list)   container_of(ptr, type, list)
#define list_from_node(node)        ((node)->list)
#define list_node_linked(node)      (list_from_node(node))

#define list_head(list) ((list)->head)
#define list_tail(list) container_of((list)->tail, LinkedNode, next)
#define list_size(list) ((list)->size)
#define list_next(node) ((node)->next)
#define list_prev(node)                         \
    (((node)->prev == &((node)->list->head))    \
        ? NULL                                  \
        : container_of((node)->prev, LinkedNode, next))

#define list_for_each(list, n)      \
    for (LinkedNode *n = list_head(list); n; n = list_next(n))

#define list_for_each_r(list, n)    \
    for (LinkedNode *n = list_tail(list); n; n = list_prev(n))

static inline
void list_node_init(LinkedNode *node)
{ node->list = NULL; }

static inline
void list_init(LinkedList *list)
{
    list->head = NULL;
    list->tail = &(list->head);
    list->size = 0;
}

static inline LinkedNode *
list_append(LinkedList *list, LinkedNode *node)
{
    node->prev      = list->tail;
    node->next      = NULL;
    node->list      = list;
    *(list->tail)   = node;
    list->tail      = &(node->next);

    ++list->size;

    return node;
}

static inline LinkedNode *
list_prepend(LinkedList *list, LinkedNode *node)
{
    node->next = list->head;
    node->prev = &(list->head);
    node->list = list;
    list->head = node;
    if (node->next) {
        node->next->prev = &(node->next);
    }
    else {
        list->tail = &(node->next);
    }

    ++list->size;

    return node;
}

static inline LinkedNode *
list_before(LinkedNode *old_node, LinkedNode *new_node)
{
    new_node->prev = old_node->prev;
    *(new_node->prev) = new_node;
    new_node->next = old_node;
    old_node->prev = &(new_node->next);
    new_node->list = old_node->list;

    ++old_node->list->size;

    return new_node;
}

static inline LinkedNode *
list_after(LinkedNode *old_node, LinkedNode *new_node)
{
    new_node->next = old_node->next;
    new_node->prev = &(old_node->next);
    old_node->next = new_node;
    new_node->list = old_node->list;
    if (new_node->next) {
        new_node->next->prev = &(new_node->next);
    }
    else {
        old_node->list->tail = &(new_node->next);
    }

    ++old_node->list->size;

    return new_node;
}

static inline LinkedNode *
list_unlink(LinkedNode *node)
{
    *(node->prev) = node->next;
    if (node->next) {
        node->next->prev = node->prev;
    }
    else {
        node->list->tail = node->prev;
    }
    --node->list->size;
    node->list = NULL;
    return node;
}

static inline void
list_move(LinkedList *dst, LinkedList *src)
{
    list_init(dst);
    while (list_size(src)) {
        list_append(dst, list_unlink(list_head(src)));
    }
}

#endif //CRIPT_LIST_H
