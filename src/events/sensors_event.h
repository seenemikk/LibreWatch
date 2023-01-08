#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

enum sensors_event_type {
    SENSORS_EVENT_STEP_COUNT,
};

struct sensors_event {
    struct app_event_header header;

    enum sensors_event_type type;
    uint32_t value;
};

APP_EVENT_TYPE_DECLARE(sensors_event);
