#ifndef MODULE_DATA_LOGGER_H
#define MODULE_DATA_LOGGER_H

#include "eventrouter.h"

void DataLogger_Init(void);
ErEventHandlerRet_t DataLogger_EventHandler(ErEvent_t* a_event);

extern ErModule_t g_data_logger_module;

#endif /* MODULE_DATA_LOGGER_H */
