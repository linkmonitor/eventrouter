#ifndef EVENTROUTER_DEFS_H
#define EVENTROUTER_DEFS_H

#define ER_UNUSED(statement) (void)(statement)

/// Returns the address of the structure containing the member.

/// The "?:" conditional operator requires its second and third operands to
/// be of compatible type, so by writing "1 ? (ptr) : &((type *)0)->member"
/// instead of simply writing "(ptr)", we ensure that `ptr` actually does
/// point to a type compatible with `member` (compilation halts if it doesn't).

#define er_container_of(ptr, type, member)                 \
    ({                                                     \
        void *__mptr = (1 ? (ptr) : &((type *)0)->member); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

#endif /* EVENTROUTER_DEFS_H */
