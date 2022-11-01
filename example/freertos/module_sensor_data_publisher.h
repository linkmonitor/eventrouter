#ifndef MODULE_SENSOR_DATA_PUBLISHER_H
#define MODULE_SENSOR_DATA_PUBLISHER_H

/// This module periodically publishes instances of SensorDataEvent_t. Subscribe
/// to ER_EVENT_TYPE__SENSOR_DATA to receive it.

#include "eventrouter.h"

typedef struct
{
    MIXIN_ER_EVENT;
    int m_temperature_c;
    int m_lux;
} SensorDataEvent_t;

void SensorDataPublisher_Init();
ErEventHandlerRet_t SensorDataPublisher_EventHandler(ErEvent_t* a_event);

extern ErModule_t g_sensor_data_publisher_module;

#endif /* MODULE_SENSOR_DATA_PUBLISHER_H */
