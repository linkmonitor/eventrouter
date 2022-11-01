# Overview

This example application defines three modules in two FreeRTOS tasks, where each
task draws from a FreeRTOS queue that passes `ErEvent_t*`s.

```mermaid
graph LR
    subgraph app_task[App Task]
        mdl[Data Logger Module]
        mdu[Data Uploader Module]
    end

    subgraph sensor_task[Sensor Task]
        msdp[Sensor Data Publisher Module]
    end

    er[Event Router]

    msdp -->|1| er
    er   -->|2| mdl
    mdl  -->|3| er
    er   -->|4| mdu
    mdu  -->|5| er
    er   -->|6| msdp
```

The Sensor Data Publisher Module sends an Event of type `ER_EVENT__SENSOR_DATA`
every two seconds. The Data Logger Module and Data Uploader Module subscribe to
`ER_EVENT__SENSOR_DATA` and receive the data that the publisher module sends.
