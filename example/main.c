#include <assert.h>

#include "FreeRTOS.h"
#include "eventrouter/module_id.h"
#include "queue.h"
#include "task.h"

#include "eventrouter/eventrouter.h"
#include "module_data_logger.h"
#include "module_data_uploader.h"
#include "module_sensor_data_publisher.h"
#include "task_generic.h"

typedef enum
{
    // Low-priority.
    TASK__APP,
    TASK__SENSOR,
    // High-priority.

    TASK__COUNT,
} Task_t;

static ErTaskDesc_t s_er_task_descriptions[TASK__COUNT];

/// Defines all modules that participate in Event Routing.
#define EVENT_ROUTING_MODULE_LIST()                              \
    X(DataLogger_EventHandler, kDataLoggerModule, TASK__APP)     \
    X(DataUploader_EventHandler, kDataUploaderModule, TASK__APP) \
    X(SensorDataPublisher_EventHandler, kSensorDataModule, TASK__SENSOR)

typedef enum
{
#define X(eventhandler, enum_entry, owning_task) enum_entry,
    EVENT_ROUTING_MODULE_LIST()
#undef X
        kModuleCount
} EventRoutingModules_t;

static const ErEventHandler_t s_er_modules[] = {
#define X(eventhandler, enum_entry, owning_task) eventhandler,
    EVENT_ROUTING_MODULE_LIST()
#undef X
};

static bool IsInIsr(void)
{
    return false;
}

static const ErOptions_t s_er_options = {
    .m_tasks              = &s_er_task_descriptions[0],
    .m_num_tasks          = TASK__COUNT,
    .m_event_handlers     = &s_er_modules[0],
    .m_num_event_handlers = kModuleCount,
    .m_first_event_type   = ER_EVENT_TYPE__FIRST,
    .m_num_event_types    = ER_EVENT_TYPE__COUNT,
    .m_system_funcs       = {.m_IsInIsr = IsInIsr},
};

int main(void)
{
    //==========================================================================
    // Create Tasks.
    //==========================================================================

    enum
    {
        APP_TASK__STACK_SIZE         = 1024 * 8,
        APP_TASK__QUEUE_LENGTH       = 10 /*elements*/,
        APP_TASK__QUEUE_ELEMENT_SIZE = sizeof(ErEvent_t*),
    };
    TaskHandle_t app_task_handle;
    GenericTaskOptions_t app_task_options = {
        .m_input_queue =
            xQueueCreate(APP_TASK__QUEUE_LENGTH, APP_TASK__QUEUE_ELEMENT_SIZE),
    };
    assert(pdPASS == xTaskCreate(GenericTask_Run, "App Task",
                                 APP_TASK__STACK_SIZE, &app_task_options,
                                 TASK__APP, &app_task_handle));

    s_er_task_descriptions[TASK__APP] = (ErTaskDesc_t){
        .m_first_module = (ErModuleId_t)&s_er_modules[kDataLoggerModule],
        .m_last_module  = (ErModuleId_t)&s_er_modules[kDataUploaderModule],
        .m_event_queue  = app_task_options.m_input_queue,
        .m_task_handle  = app_task_handle,
    };

    enum
    {
        SENSOR_TASK__STACK_SIZE         = 1024 * 8,
        SENSOR_TASK__QUEUE_LENGTH       = 10 /*elements*/,
        SENSOR_TASK__QUEUE_ELEMENT_SIZE = sizeof(ErEvent_t*),
    };
    TaskHandle_t sensor_task_handle;
    GenericTaskOptions_t sensor_task_options = {
        .m_input_queue = xQueueCreate(SENSOR_TASK__QUEUE_LENGTH,
                                      SENSOR_TASK__QUEUE_ELEMENT_SIZE),
    };
    assert(pdPASS == xTaskCreate(GenericTask_Run, "Sensor Task",
                                 SENSOR_TASK__STACK_SIZE, &sensor_task_options,
                                 TASK__SENSOR, &sensor_task_handle));

    s_er_task_descriptions[TASK__SENSOR] = (ErTaskDesc_t){
        .m_first_module = (ErModuleId_t)&s_er_modules[kSensorDataModule],
        .m_last_module  = (ErModuleId_t)&s_er_modules[kSensorDataModule],
        .m_event_queue  = sensor_task_options.m_input_queue,
        .m_task_handle  = sensor_task_handle,
    };

    ErInit(&s_er_options);

    //==========================================================================
    // Initialize Modules
    //==========================================================================

    DataLogger_Init((ErModuleId_t)&s_er_modules[kDataLoggerModule]);
    DataUploader_Init((ErModuleId_t)&s_er_modules[kDataUploaderModule]);
    SensorDataPublisher_Init((ErModuleId_t)&s_er_modules[kSensorDataModule]);

    //==========================================================================
    // Start Scheduler
    //==========================================================================

    vTaskStartScheduler();
    assert(!"The scheduler should never return");

    return 0;
}
