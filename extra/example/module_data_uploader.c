#include "module_data_uploader.h"

#include <stdio.h>

#include "eventrouter.h"
#include "module_sensor_data_publisher.h"

static ErEventHandlerRet_t DataUploader_EventHandler(ErEvent_t* a_event,
                                                     void* a_context);

ErModule_t g_data_uploader_module =
    ER_CREATE_MODULE(DataUploader_EventHandler, NULL);

void DataUploader_Init(void)
{
    ErSubscribe(&g_data_uploader_module, ER_EVENT_TYPE__SENSOR_DATA);
}

ErEventHandlerRet_t DataUploader_EventHandler(ErEvent_t* a_event,
                                              void* a_context)
{
    ER_UNUSED(a_context);
    ErEventHandlerRet_t result = ER_EVENT_HANDLER_RET__HANDLED;

    switch (a_event->m_type)
    {
        case ER_EVENT_TYPE__SENSOR_DATA:
        {
            SensorDataEvent_t event = FROM_ER_EVENT(a_event, SensorDataEvent_t);
            printf(
                "Uploading Sensor Data:\n"
                "   Temperature C  = %d\n"
                "   Brightness Lux = %d\n",
                event.m_temperature_c, event.m_lux);
            break;
        }

        default:
            assert(!"Modules do not receive events they don't subscribe to or send.");
            break;
    }

    return result;
}
