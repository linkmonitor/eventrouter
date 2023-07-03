#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

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

int main(void)
{
    //==========================================================================
    // Create Tasks.
    //==========================================================================

    enum
    {
        SENSOR_TASK__STACK_SIZE         = 1024 * 8,
        SENSOR_TASK__QUEUE_LENGTH       = 10 /*elements*/,
        SENSOR_TASK__QUEUE_ELEMENT_SIZE = sizeof(ErEvent_t*),
    };
    TaskHandle_t sensor_task_handle;
    GenericTaskOptions_t sensor_task_options = {
        .m_input_queue = ErQueueNew(SENSOR_TASK__QUEUE_LENGTH),
    };
    assert(pdPASS == xTaskCreate(GenericTask_Run, "Sensor Task",
                                 SENSOR_TASK__STACK_SIZE, &sensor_task_options,
                                 TASK_PRI__SENSOR, &sensor_task_handle));
    s_er_tasks[TASK__SENSOR].m_event_queue = sensor_task_options.m_input_queue;
    s_er_tasks[TASK__SENSOR].m_task_handle = sensor_task_handle;

    enum
    {
        APP_TASK__STACK_SIZE         = 1024 * 8,
        APP_TASK__QUEUE_LENGTH       = 10 /*elements*/,
        APP_TASK__QUEUE_ELEMENT_SIZE = sizeof(ErEvent_t*),
    };
    TaskHandle_t app_task_handle;
    GenericTaskOptions_t app_task_options = {
        .m_input_queue = ErQueueNew(APP_TASK__QUEUE_LENGTH),
    };
    assert(pdPASS == xTaskCreate(GenericTask_Run, "App Task",
                                 APP_TASK__STACK_SIZE, &app_task_options,
                                 TASK_PRI__APP, &app_task_handle));
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

    const int kPublishPeriodSeconds = 2;
    TimerHandle_t timer =
        xTimerCreate("Sensor Data Publish Timer",
                     (configTICK_RATE_HZ * kPublishPeriodSeconds), pdTRUE, NULL,
                     SensorDataPublisher_GenerateData);
    xTimerStart(timer, 0);

    //==========================================================================
    // Start Scheduler
    //==========================================================================

    vTaskStartScheduler();
    assert(!"The scheduler should never return");

    return 0;
}
