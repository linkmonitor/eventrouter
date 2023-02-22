#ifndef EVENTROUTER_CONFIG_H
#define EVENTROUTER_CONFIG_H

/// NOTE: This configuration is made common to all implementations so tests and
/// example applications can refer to the same event types.

#define ER_EVENT_TYPE__ENTRIES \
    X(ER_EVENT_TYPE__1)        \
    X(ER_EVENT_TYPE__2)        \
    X(ER_EVENT_TYPE__3)        \
    X(ER_EVENT_TYPE__4)        \
    X(ER_EVENT_TYPE__5)        \
    X(ER_EVENT_TYPE__SENSOR_DATA)

#endif /* EVENTROUTER_CONFIG_H */
