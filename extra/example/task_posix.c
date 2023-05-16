#include "task_posix.h"
#include <assert.h>

#include <mqueue.h>
#include <stdio.h>

#include "eventrouter.h"

void* GenericTask_Run(void *a_parameters)
{
    const GenericTaskOptions_t *options = (GenericTaskOptions_t *)a_parameters;

    assert(options != NULL);
    assert(options->m_input_queue != 0);

    while (true)
    {
        /// Wait to receive events and then call the appropriate handlers; this
        /// involves both delivering events to subscribers and returning events
        /// to the modules that sent them.
        ErEvent_t *event = NULL;
        unsigned int prio;
        int res = mq_receive(options->m_input_queue, (char*)&event, sizeof(ErEvent_t*), &prio);
        if (res > 0)
        {
            ErCallHandlers(event);
        }
        else
        {
            perror("mq_receive: ");
        }
    }

    assert(!"The event loop should never exit");
    return NULL;
}
