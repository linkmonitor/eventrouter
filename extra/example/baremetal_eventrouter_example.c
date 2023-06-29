#include <stdio.h>
#include <unistd.h>

#include "eventrouter.h"
#include "module_data_logger.h"
#include "module_data_uploader.h"
#include "module_sensor_data_publisher.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static ErModule_t *s_er_modules[] = {
    &g_sensor_data_publisher_module,
    &g_data_logger_module,
    &g_data_uploader_module,
};

static ErTask_t s_er_task = {
    .m_modules     = s_er_modules,
    .m_num_modules = ARRAY_SIZE(s_er_modules),
};

static ErOptions_t s_er_options = {
    .m_tasks     = &s_er_task,
    .m_num_tasks = 1,
    .m_IsInIsr   = NULL,
};

int main(void)
{
    ErInit(&s_er_options);

    SensorDataPublisher_Init();
    DataLogger_Init();
    DataUploader_Init();

    while (1)
    {
        ErNewLoop();

        ErEvent_t *event = NULL;
        while ((event = ErGetEventToDeliver()))
        {
            ErCallHandlers(event);
        }

        sleep(1);
        SensorDataPublisher_GenerateData();
    }

    return 0;
}
