#include "ui_event.h"

static void log_ui_app_show_event(const struct app_event_header *aeh)
{
    const struct ui_app_show_event *event = cast_ui_app_show_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "Type: %d", event->type);
}

static void profile_ui_app_show_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct ui_app_show_event *event = cast_ui_app_show_event(aeh);

    nrf_profiler_log_encode_uint8(buf, event->type);
}

APP_EVENT_INFO_DEFINE(ui_app_show_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("type"),
                      profile_ui_app_show_event);

APP_EVENT_TYPE_DEFINE(ui_app_show_event,
                      log_ui_app_show_event,
                      &ui_app_show_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_UI_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_ui_popup_show_event(const struct app_event_header *aeh)
{
    const struct ui_popup_show_event *event = cast_ui_popup_show_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "Show %d, Type: %d", event->show, event->type);
}

static void profile_ui_popup_show_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct ui_popup_show_event *event = cast_ui_popup_show_event(aeh);

    nrf_profiler_log_encode_uint8(buf, event->show);
    nrf_profiler_log_encode_uint8(buf, event->type);
}

APP_EVENT_INFO_DEFINE(ui_popup_show_event,
                      ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
                      ENCODE("show", "type"),
                      profile_ui_popup_show_event);

APP_EVENT_TYPE_DEFINE(ui_popup_show_event,
                      log_ui_popup_show_event,
                      &ui_popup_show_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_UI_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
