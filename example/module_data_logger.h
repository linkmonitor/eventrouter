#ifndef MODULE_DATA_LOGGER_H
#define MODULE_DATA_LOGGER_H

#include "eventrouter/event_handler.h"
#include "eventrouter/module_id.h"

void DataLogger_Init(ErModuleId_t a_id);
ErEventHandlerRet_t DataLogger_EventHandler(ErEvent_t* a_event);

#endif /* MODULE_DATA_LOGGER_H */
