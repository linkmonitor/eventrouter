#ifndef EVENTROUTER_LIST_H
#define EVENTROUTER_LIST_H

#include "checked_config.h"

#if ER_IMPLEMENTATION != ER_IMPL_BAREMETAL
#error "Linked list logic is only needed in the baremetal implementation"
#endif

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// One node in a singly-linked list. Structs of this type are intended to
    /// be embedded in other structs to add linked-list functionality to them.
    typedef struct ErList_t
    {
        struct ErList_t *m_next;
    } ErList_t;

    /// Appends `a_node` to the `a_list`. This function does nothing if `a_node`
    /// is already in `a_list`. Asserts if either argument is NULL. Asserts if
    /// `a_list` equals `a_node`.
    void ErListAppend(ErList_t *a_list, ErList_t *a_node);

    /// Removes `a_node` from `a_list` if it is in the list and does nothing if
    /// not. Asserts if either argument is NULL. Asserts if `a_list` equals
    /// `a_node`.
    void ErListRemove(ErList_t *a_list, ErList_t *a_node);

    /// Returns true if `a_node` is part of `a_list` and false otherwise.
    /// Asserts if either argument is NULL.
    bool ErListContains(ErList_t *a_list, ErList_t *a_node);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_LIST_H */
