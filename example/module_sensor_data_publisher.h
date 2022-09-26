#ifndef MODULE_SENSOR_DATA_PUBLISHER_H
#define MODULE_SENSOR_DATA_PUBLISHER_H

/// This module periodically publishes instances of SensorDataEvent_t. Subscribe
/// to ER_EVENT_TYPE__SENSOR_DATA to receive it.

#include "eventrouter/event.h"
#include "eventrouter/event_handler.h"
#include "eventrouter/module_id.h"

typedef struct
{
    MIXIN_ER_EVENT;
    int m_temperature_c;
    int m_lux;
} SensorDataEvent_t;

void SensorDataPublisher_Init(ErModuleId_t a_id);
ErEventHandlerRet_t SensorDataPublisher_EventHandler(ErEvent_t* a_event);

#endif /* MODULE_SENSOR_DATA_PUBLISHER_H */
