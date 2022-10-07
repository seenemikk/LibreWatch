#include "passkey_event.h"

static void log_passkey_event(const struct app_event_header *aeh)
{
    const struct passkey_event *event = cast_passkey_event(aeh);

    if (event->show) {
        APP_EVENT_MANAGER_LOG(aeh, "Passkey: %06d", event->passkey);
    } else {
        APP_EVENT_MANAGER_LOG(aeh, "Success: %d", event->success);
    }
}

static void profile_passkey_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct passkey_event *event = cast_passkey_event(aeh);

    if (event->show) {
        nrf_profiler_log_encode_uint32(buf, event->passkey);
    } else {
        nrf_profiler_log_encode_uint32(buf, event->success);
    }
    nrf_profiler_log_encode_uint8(buf, event->show);
}

APP_EVENT_INFO_DEFINE(passkey_event,
                      ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8),
                      ENCODE("passkey/success", "show"),
                      profile_passkey_event);

APP_EVENT_TYPE_DEFINE(passkey_event,
                      log_passkey_event,
                      &passkey_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(SMARTWATCH_PASSKEY_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

