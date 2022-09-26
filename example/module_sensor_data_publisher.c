#include "module_sensor_data_publisher.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "eventrouter/event.h"
#include "eventrouter/event_handler.h"
#include "eventrouter/eventrouter.h"

static SensorDataEvent_t s_event;

static void TimerCallback(TimerHandle_t a_timer)
{
    if (!ErEventIsInFlight(&s_event.m_event))
    {
        s_event.m_temperature_c = rand() % 100;
        s_event.m_lux           = rand() % 50;

        printf("\nPublishing sensor data\n");
        ErSend(TO_ER_EVENT(s_event));
    }
}

void SensorDataPublisher_Init(ErModuleId_t a_id)
{
    s_event.m_event.m_type           = ER_EVENT_TYPE__SENSOR_DATA;
    s_event.m_event.m_sending_module = a_id;

    srand(time(NULL));

    const int kPublishPeriodSeconds = 2;
    TimerHandle_t timer =
        xTimerCreate("Sensor Data Publish Timer",
                     (configTICK_RATE_HZ * kPublishPeriodSeconds), pdTRUE, NULL,
                     TimerCallback);
    xTimerStart(timer, 0);
}

ErEventHandlerRet_t SensorDataPublisher_EventHandler(ErEvent_t *a_event)
{
    ErEventHandlerRet_t result = ER_EVENT_HANDLER_RET__HANDLED;

    switch (a_event->m_type)
    {
        case ER_EVENT_TYPE__SENSOR_DATA:
            printf("Event returned to sender after delivery to subscribers\n");
            break;

        default:
            assert(!"Modules do not receive events they don't subscribe to or send.");
            break;
    }

    return result;
}
