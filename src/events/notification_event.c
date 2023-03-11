#include "notification_event.h"

static void log_notification_event(const struct app_event_header *aeh)
{
    const struct notification_event *event = cast_notification_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "UID: %d, Type: %d, Category: %d",
                          event->info.notif_uid, (uint8_t)event->info.evt_id, (uint8_t)event->info.category_id);
    APP_EVENT_MANAGER_LOG(aeh, "Title: %s, Message: %s, App name: %s", event->title, event->message, event->app_name);
}

static void profile_notification_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct notification_event *event = cast_notification_event(aeh);

    nrf_profiler_log_encode_uint32(buf, event->info.notif_uid);
    nrf_profiler_log_encode_uint8(buf, (uint8_t)event->info.evt_id);
    nrf_profiler_log_encode_uint8(buf, (uint8_t)event->info.category_id);
}

static void log_notification_action_event(const struct app_event_header *aeh)
{
    const struct notification_action_event *event = cast_notification_action_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "UID: %d, Action, %d", event->uid, event->action);
}

static void profile_notification_action_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct notification_action_event *event = cast_notification_action_event(aeh);

    nrf_profiler_log_encode_uint32(buf, event->uid);
    nrf_profiler_log_encode_uint8(buf, event->action);
}

APP_EVENT_INFO_DEFINE(notification_event,
                      ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
                      ENCODE("UID", "type", "category"),
                      profile_notification_event);

APP_EVENT_TYPE_DEFINE(notification_event,
                      log_notification_event,
                      &notification_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_SMARTWATCH_NOTIFICATION_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_INFO_DEFINE(notification_action_event,
                      ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8),
                      ENCODE("UID", "action"),
                      profile_notification_action_event);

APP_EVENT_TYPE_DEFINE(notification_action_event,
                      log_notification_action_event,
                      &notification_action_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_SMARTWATCH_NOTIFICATION_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
