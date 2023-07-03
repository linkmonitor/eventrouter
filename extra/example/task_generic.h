#ifndef TASK_GENERIC_H
#define TASK_GENERIC_H

#include "eventrouter.h"

typedef struct
{
    ErQueueHandle_t m_input_queue;
} GenericTaskOptions_t;

void GenericTask_Run(void *a_parameters);

#endif /* TASK_GENERIC_H */
