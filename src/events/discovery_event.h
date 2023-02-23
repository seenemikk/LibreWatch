#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#include <bluetooth/gatt_dm.h>

enum discovery_event_type {
    DISCOVERY_EVENT_START,
    DISCOVERY_EVENT_AMS,
    DISCOVERY_EVENT_CTS,
    DISCOVERY_EVENT_COUNT,
};

struct discovery_event {
    struct app_event_header header;

    struct bt_gatt_dm *gatt_dm;
    uint8_t type;   // enum discovery_event_type
};

APP_EVENT_TYPE_DECLARE(discovery_event);
