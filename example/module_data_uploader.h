#ifndef MODULE_DATA_UPLOADER_H
#define MODULE_DATA_UPLOADER_H

#include "eventrouter.h"

void DataUploader_Init(void);
ErEventHandlerRet_t DataUploader_EventHandler(ErEvent_t* a_event);

extern ErModule_t g_data_uploader_module;

#endif /* MODULE_DATA_UPLOADER_H */
