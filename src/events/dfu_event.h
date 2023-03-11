#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

struct dfu_status_event {
    struct app_event_header header;

    uint8_t percentage;
};

APP_EVENT_TYPE_DECLARE(dfu_status_event);
