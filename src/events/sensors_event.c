#include "sensors_event.h"

static void log_sensors_event(const struct app_event_header *aeh)
{
    const struct sensors_event *event = cast_sensors_event(aeh);
    APP_EVENT_MANAGER_LOG(aeh, "Type: %d; Value: %u", event->type, event->value);
}

static void profile_sensors_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct sensors_event *event = cast_sensors_event(aeh);
    nrf_profiler_log_encode_uint8(buf, event->type);
    nrf_profiler_log_encode_uint32(buf, event->value);
}

APP_EVENT_INFO_DEFINE(sensors_event,
                      ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U32),
                      ENCODE("type", "value"),
                      profile_sensors_event);

APP_EVENT_TYPE_DEFINE(sensors_event,
                      log_sensors_event,
                      &sensors_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_SENSORS_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
