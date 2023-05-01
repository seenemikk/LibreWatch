#include "media_event.h"

static void log_media_capabilities_event(const struct app_event_header *aeh)
{
    const struct media_capabilities_event *event = cast_media_capabilities_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "Capabilities: 0x%02x", event->capabilities);
}

static void profile_media_capabilities_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct media_capabilities_event *event = cast_media_capabilities_event(aeh);

    nrf_profiler_log_encode_uint8(buf, event->capabilities);
}

static void log_media_player_event(const struct app_event_header *aeh)
{
    const struct media_player_event *event = cast_media_player_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "Elapsed time: %d; Playback rate: %d; State: %d", event->elapsed_time, event->playback_rate, event->state);
}

static void profile_media_player_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct media_player_event *event = cast_media_player_event(aeh);

    nrf_profiler_log_encode_uint32(buf, event->elapsed_time);
    nrf_profiler_log_encode_uint16(buf, event->playback_rate);
    nrf_profiler_log_encode_uint8(buf, event->state);
}

static void log_media_track_event(const struct app_event_header *aeh)
{
    const struct media_track_event *event = cast_media_track_event(aeh);

    switch (event->type) {
        case MEDIA_TRACK_INFO_DURATION: {
            APP_EVENT_MANAGER_LOG(aeh, "Duration: %u", event->duration);
            break;
        }

        case MEDIA_TRACK_INFO_ARTIST:
        case MEDIA_TRACK_INFO_TITLE: {
            APP_EVENT_MANAGER_LOG(aeh, "Title/Artist: %s", event->str);
            break;
        }

        default: {
            break;
        }
    }
}

static void profile_media_track_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct media_track_event *event = cast_media_track_event(aeh);

    nrf_profiler_log_encode_uint8(buf, event->type);
}

static void log_media_command_event(const struct app_event_header *aeh)
{
    const struct media_command_event *event = cast_media_command_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh, "Command: %d", event->command);
}

static void profile_media_command_event(struct log_event_buf *buf, const struct app_event_header *aeh)
{
    const struct media_command_event *event = cast_media_command_event(aeh);

    nrf_profiler_log_encode_uint8(buf, event->command);
}

APP_EVENT_INFO_DEFINE(media_capabilities_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("capabilities"),
                      profile_media_capabilities_event);

APP_EVENT_TYPE_DEFINE(media_capabilities_event,
                      log_media_capabilities_event,
                      &media_capabilities_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_MEDIA_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_INFO_DEFINE(media_player_event,
                      ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U16, NRF_PROFILER_ARG_U8),
                      ENCODE("elapsed_time", "playback_rate", "state"),
                      profile_media_player_event);

APP_EVENT_TYPE_DEFINE(media_player_event,
                      log_media_player_event,
                      &media_player_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_MEDIA_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_INFO_DEFINE(media_track_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("type"),
                      profile_media_track_event);

APP_EVENT_TYPE_DEFINE(media_track_event,
                      log_media_track_event,
                      &media_track_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_MEDIA_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_INFO_DEFINE(media_command_event,
                      ENCODE(NRF_PROFILER_ARG_U8),
                      ENCODE("command"),
                      profile_media_command_event);

APP_EVENT_TYPE_DEFINE(media_command_event,
                      log_media_command_event,
                      &media_command_event_info,
                      APP_EVENT_FLAGS_CREATE(IF_ENABLED(CONFIG_LIBREWATCH_MEDIA_EVENT_INIT_LOG, (APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
