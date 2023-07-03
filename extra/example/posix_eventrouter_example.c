#include <assert.h>

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "eventrouter.h"
#include "module_data_logger.h"
#include "module_data_uploader.h"
#include "module_sensor_data_publisher.h"
#include "task_generic.h"

typedef enum
{
    // Low-priority.
    TASK_PRI__APP,
    TASK_PRI__SENSOR,
    // High-priority.

    TASK_PRI__COUNT,
} TaskPriorty_t;

typedef enum
{
    TASK__SENSOR,
    TASK__APP,

    TASK__COUNT,
} TaskId_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

static ErModule_t* s_sensor_modules[] = {&g_sensor_data_publisher_module};
static ErModule_t* s_app_modules[]    = {&g_data_logger_module,
                                         &g_data_uploader_module};

static ErTask_t s_er_tasks[TASK__COUNT] = {
    [TASK__SENSOR] = {.m_modules     = s_sensor_modules,
                      .m_num_modules = ARRAY_SIZE(s_sensor_modules)},
    [TASK__APP]    = {.m_modules     = s_app_modules,
                      .m_num_modules = ARRAY_SIZE(s_app_modules)},
};

static bool IsInIsr(void)
{
    return false;
}

static const ErOptions_t s_er_options = {
    .m_tasks     = s_er_tasks,
    .m_num_tasks = TASK__COUNT,
    .m_IsInIsr   = IsInIsr,
};

static void* GenericTask_Posix(void* a_parameter)
{
    GenericTask_Run(a_parameter);
    return NULL;
}

int main(void)
{
    //==========================================================================
    // Create Tasks.
    //==========================================================================

    ErQueueHandle_t sensor_q = ErQueueNew(10);
    assert(sensor_q != NULL);

    ErQueueHandle_t app_q = ErQueueNew(10);
    assert(app_q != NULL);

    pthread_t sensor_task_handle;
    GenericTaskOptions_t sensor_task_options = {
        .m_input_queue = sensor_q,
    };
    int res = pthread_create(&sensor_task_handle, NULL, GenericTask_Posix,
                             &sensor_task_options);
    if (res < 0)
    {
        perror("sensor task: ");
    }
    s_er_tasks[TASK__SENSOR].m_event_queue = sensor_task_options.m_input_queue;
    s_er_tasks[TASK__SENSOR].m_task_handle = sensor_task_handle;

    pthread_t app_task_handle;
    GenericTaskOptions_t app_task_options = {
        .m_input_queue = app_q,
    };
    res = pthread_create(&app_task_handle, NULL, GenericTask_Posix,
                         &app_task_options);
    if (res < 0)
    {
        perror("app task: ");
    }
    s_er_tasks[TASK__APP].m_event_queue = app_task_options.m_input_queue;
    s_er_tasks[TASK__APP].m_task_handle = app_task_handle;

    ErInit(&s_er_options);

    //==========================================================================
    // Initialize Modules
    //==========================================================================

    DataLogger_Init();
    DataUploader_Init();
    SensorDataPublisher_Init();

    //==========================================================================
    // Start polling timer.
    //==========================================================================

    while (true)
    {
        sleep(2);
        SensorDataPublisher_GenerateData();
    }

    //==========================================================================
    // Start Scheduler
    //==========================================================================

    pthread_join(sensor_task_handle, NULL);
    pthread_join(app_task_handle, NULL);

    return 0;
}
