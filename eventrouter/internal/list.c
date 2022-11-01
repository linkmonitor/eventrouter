#include "list.h"

#include <stddef.h>

#include "checked_config.h"

void ErListAppend(ErList_t *a_list, ErList_t *a_node)
{
    ER_ASSERT(a_list != NULL);
    ER_ASSERT(a_node != NULL);
    ER_ASSERT(a_list != a_node);

    while (a_list->m_next != NULL)
    {
        if (a_list->m_next == a_node)
        {
            return;
        }
        a_list = a_list->m_next;
    }
    a_list->m_next = a_node;
}

void ErListRemove(ErList_t *a_list, ErList_t *a_node)
{
    ER_ASSERT(a_list != NULL);
    ER_ASSERT(a_node != NULL);
    ER_ASSERT(a_list != a_node);

    while (a_list->m_next != NULL)
    {
        if (a_list->m_next == a_node)
        {
            a_list->m_next = a_node->m_next;
            a_node->m_next = NULL;
            break;
        }
        a_list = a_list->m_next;
    }
}

bool ErListContains(ErList_t *a_list, ErList_t *a_node)
{
    ER_ASSERT(a_list != NULL);
    ER_ASSERT(a_node != NULL);

    bool ret = false;
    while (a_list != NULL)
    {
        if (a_list == a_node)
        {
            ret = true;
            break;
        }
        a_list = a_list->m_next;
    }
    return ret;
}
