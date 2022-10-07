#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

struct passkey_event {
    struct app_event_header header;

    union {
        uint32_t passkey;   // If show
        uint32_t success;   // If !show
    };
    bool show;
};

APP_EVENT_TYPE_DECLARE(passkey_event);
