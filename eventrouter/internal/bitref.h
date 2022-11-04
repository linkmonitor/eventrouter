#ifndef EVENTROUTER_BITREF_H
#define EVENTROUTER_BITREF_H

#include <limits.h>

#include "atomic.h"
#include "checked_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// A reference to a specific bit within an array of bytes.
    typedef struct
    {
        atomic_char *m_byte;
        uint8_t m_bit_mask;
    } BitRef_t;

    static inline BitRef_t GetBitRef(atomic_char *a_data, size_t a_bit)
    {
        ER_STATIC_ASSERT(
            sizeof(uint8_t) == sizeof(atomic_char),
            "The Event Router must be able to access raw bytes atomically");

        return (BitRef_t){
            .m_byte     = &a_data[a_bit / CHAR_BIT],
            .m_bit_mask = (1 << (a_bit % CHAR_BIT)),
        };
    }

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_BITREF_H */
