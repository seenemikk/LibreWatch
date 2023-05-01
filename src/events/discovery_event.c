#include "discovery_event.h"

static void log_discovery_event(const struct app_event_header *aeh)
{
    const struct discovery_event *event = cast_discovery_event(aeh);
    APP_EVENT_MANAGER_LOG(aeh, "Type: %d", event->type);
}

static void profile_discovery_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct discovery_event *event = cast_discovery_event(aeh);
    nrf_profiler_log_encode_uint8(buf, event->type);
}

APP_EVENT_INFO_DEFINE(discovery_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("type"),
                      profile_discovery_event);

APP_EVENT_TYPE_DEFINE(discovery_event,
                      log_discovery_event,
                      &discovery_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_DISCOVERY_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
