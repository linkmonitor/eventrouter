#ifndef MODULE_DATA_UPLOADER_H
#define MODULE_DATA_UPLOADER_H

#include "eventrouter/event_handler.h"
#include "eventrouter/module_id.h"

void DataUploader_Init(ErModuleId_t a_id);
ErEventHandlerRet_t DataUploader_EventHandler(ErEvent_t* a_event);

#endif /* MODULE_DATA_UPLOADER_H */
