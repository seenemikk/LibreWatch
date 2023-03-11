#include "dfu_event.h"

static void log_dfu_status_event(const struct app_event_header *aeh)
{
    const struct dfu_status_event *event = cast_dfu_status_event(aeh);
    APP_EVENT_MANAGER_LOG(aeh, "Status: %d%%", event->percentage);
}

static void profile_dfu_status_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct dfu_status_event *event = cast_dfu_status_event(aeh);
    nrf_profiler_log_encode_uint8(buf, event->percentage);
}

APP_EVENT_INFO_DEFINE(dfu_status_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("percentage"),
                      profile_dfu_status_event);

APP_EVENT_TYPE_DEFINE(dfu_status_event,
                      log_dfu_status_event,
                      &dfu_status_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_SMARTWATCH_DFU_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
