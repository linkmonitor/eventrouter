# Event Router

A C library for event-based inter-task communication in [FreeRTOS](https://www.freertos.org/).

# Quickstart

The `example/` application demonstrates how to define events, initialize the Event Router, publish events, and subscribe to them.

Developers who wish to use the Event Router must compile `eventrouter.c` (with a C11-compatible compiler) and add `<repo>/include` to their list of include paths.

# Overview

This document models event-based RTOS applications as collections of Tasks, each of which contain Modules, each of which (may) generate events and post them to Queues. Each Task reads Events from its Queue and delivers them to the Modules it "contains" based on the Event's type.

```mermaid
graph LR
    subgraph Task_A[Task A]
        Module_A1[Module A.1]
        Module_A2[Module A.2]
        Queue_A[Queue]
    end

    Queue_A --> Module_A1
    Queue_A --> Module_A2
    Module_A1 -->|Event| Queue_B

    subgraph Task_B[Task B]
        Module_B1[Module B.1]
        Queue_B[Queue]
    end

    Queue_B --> Module_B1
```

Ideally, Modules only know about the Events types they publish and the Events types they want to receive; they should know nothing about the modules that consume the data they produce and nothing about the modules that produce what they consume. This results in a loosely-coupled application.

```mermaid
graph LR
    Event_A[Event Type A] -->|Requires| Module
    Event_B[Event Type B] -->|Requires| Module
    Event_C[Event Type C] -->|Requires| Module
    Module -->|Provides|Event_D[Event Type D]
```

The Event Router achieves this by sitting between Queues and Modules (and between Modules and Queues) to provide a uniform publisher-subscriber framework. When a Module sends an event, the Event Router delivers it to all Modules which subscribe to Events of that type and then returns it to the sending Module.

```mermaid
graph LR
    subgraph Task_A[Task A]
        Queue_A[Queue]
        Module_A1[ModulA.1]
        Module_A2[Module A.2]
        EventRouter_A[Event Router]
        style EventRouter_A color:green
    end

    EventRouter_Mid[Event Router]
    style EventRouter_Mid color:green

    subgraph Task_B[Task B]
        Queue_B[Queue]
        EventRouter_B[Event Router]
        style EventRouter_B color:green
        Module_B1[Module B.1]
    end

    subgraph Task_C[Task C]
        Queue_C[Queue]
        EventRouter_C[Event Router]
        style EventRouter_C color:green
        Module_C1[Module C.1]
        Module_C2[Module C.2]
    end

    Queue_A --> EventRouter_A
    EventRouter_A --> Module_A1
    EventRouter_A --> Module_A2
    Module_A1 --> EventRouter_Mid

    EventRouter_Mid --> Queue_B
    Queue_B --> EventRouter_B
    EventRouter_B --> Module_B1

    EventRouter_Mid --> Queue_C
    Queue_C --> EventRouter_C
    EventRouter_C --> Module_C1
    EventRouter_C -.->|Not Interested| Module_C2
```

The Event Router passes events by reference. This means Modules may not modify events after sending them until the Event Router returns them. In this way, the send-deliver-return flow lets Modules perform a primitive form of ownership tracking.
