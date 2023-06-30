#include "task_generic.h"

#include <assert.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "eventrouter.h"

void GenericTask_Run(void *a_parameters)
{
    const GenericTaskOptions_t *options = (GenericTaskOptions_t *)a_parameters;

    assert(options != NULL);
    assert(options->m_input_queue != NULL);

    while (true)
    {
        /// Wait to receive events and then call the appropriate handlers; this
        /// involves both delivering events to subscribers and returning events
        /// to the modules that sent them.
        ErEvent_t *event = ErReceive();
        ErCallHandlers(event);
    }

    assert(!"The event loop should never exit");
}
