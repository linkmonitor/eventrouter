#ifndef TASK_POSIX_H
#define TASK_POSIX_H

#include <mqueue.h>

typedef struct
{
    mqd_t m_input_queue;
} GenericTaskOptions_t;

void* GenericTask_Run(void *a_parameters);

#endif /* TASK_POSIX_H */
