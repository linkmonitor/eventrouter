#ifndef EVENTROUTER_DEFS_H
#define EVENTROUTER_DEFS_H

#define ER_UNUSED(statement) (void)(statement);

/// Returns the address of the structure containing the member.
#ifndef container_of
#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })
#endif

#endif /* EVENTROUTER_DEFS_H */
