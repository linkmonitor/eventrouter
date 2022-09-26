#ifndef EVENTROUTER_MODULE_ID_H
#define EVENTROUTER_MODULE_ID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// An integer which uniquely identifies a module. Software which is
    /// module-aware accepts these IDs to direct module-specific changes.
    typedef intptr_t ErModuleId_t;

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_MODULE_ID_H */
