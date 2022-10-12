#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

struct ui_app_show_event {
    struct app_event_header header;

    uint8_t type; // enum ui_app_type
};

APP_EVENT_TYPE_DECLARE(ui_app_show_event);

struct ui_popup_show_event {
    struct app_event_header header;

    uint8_t type; // enum ui_popup_type
    bool show;
};

APP_EVENT_TYPE_DECLARE(ui_popup_show_event);
