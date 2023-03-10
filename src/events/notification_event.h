#pragma once

#include <bluetooth/services/ancs_client.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

enum notification_type {
    NOTIFICATION_ADDED,
    NOTIFICATION_MODIFIED,
    NOTIFICATION_REMOVED,
};

enum notification_category_type {
    NOTIFICATION_CATEGORY_OTHER,
    NOTIFICATION_CATEGORY_INCOMING_CALL,
    NOTIFICATION_CATEGORY_MISSED_CALL,
    NOTIFICATION_CATEGORY_VOICEMAIL,
    NOTIFICATION_CATEGORY_SOCIAL,
    NOTIFICATION_CATEGORY_SCHEDULE,
    NOTIFICATION_CATEGORY_EMAIL,
    NOTIFICATION_CATEGORY_NEWS,
    NOTIFICATION_CATEGORY_HEALTH_AND_FITNESS,
    NOTIFICATION_CATEGORY_BUSINESS_AND_FINANCE,
    NOTIFICATION_CATEGORY_LOCATION,
    NOTIFICATION_CATEGORY_ENTERTAINMENT,
    NOTIFICATION_CATEGORY_ACTIVE_CALL,
};

// Modules should also subscribe to ble_peer_event and delete all stored notification data when disconnect message is received.
struct notification_event {
    struct app_event_header header;

    struct bt_ancs_evt_notif info;
    uint8_t app_id[BT_ANCS_ATTR_DATA_MAX];
    uint8_t app_name[BT_ANCS_ATTR_DATA_MAX];
    uint8_t title[CONFIG_SMARTWATCH_ANCS_MAX_TITLE_SIZE];
    uint8_t message[CONFIG_SMARTWATCH_ANCS_MAX_MESSAGE_SIZE];

    // INTERNAL
    uint8_t pending_notif_attributes;
    uint8_t pending_app_attributes;
};

APP_EVENT_TYPE_DECLARE(notification_event);
