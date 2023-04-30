#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

enum media_command_type {
    MEDIA_COMMAND_PLAY,
    MEDIA_COMMAND_PAUSE,
    MEDIA_COMMAND_TOGGLE_PLAY_PAUSE,
    MEDIA_COMMAND_NEXT_TRACK,
    MEDIA_COMMAND_PREVIOUS_TRACK,
    MEDIA_COMMAND_VOLUME_UP,
    MEDIA_COMMAND_VOLUME_DOWN,
    MEDIA_COMMAND_COUNT,
};

#define MEDIA_CAPABILITIES_PLAY                 BIT(MEDIA_COMMAND_PLAY)
#define MEDIA_CAPABILITIES_PAUSE                BIT(MEDIA_COMMAND_PAUSE)
#define MEDIA_CAPABILITIES_PAUSE_PAUSE          BIT(MEDIA_COMMAND_TOGGLE_PLAY_PAUSE)
#define MEDIA_CAPABILITIES_NEXT_TRACK           BIT(MEDIA_COMMAND_NEXT_TRACK)
#define MEDIA_CAPABILITIES_PREVIOUS_TRACK       BIT(MEDIA_COMMAND_PREVIOUS_TRACK)
#define MEDIA_CAPABILITIES_VOLUME_UP            BIT(MEDIA_COMMAND_VOLUME_UP)
#define MEDIA_CAPABILITIES_VOLUME_DOWN          BIT(MEDIA_COMMAND_VOLUME_DOWN)

enum media_player_state_type {
    MEDIA_PLAYER_STATE_PAUSED,
    MEDIA_PLAYER_STATE_PLAYING,
    MEDIA_PLAYER_STATE_REWINDING,
    MEDIA_PLAYER_STATE_FORWARDING,
    MEDIA_PLAYER_STATE_DISCONNECTED,
    MEDIA_PLAYER_STATE_COUNT,
};

enum media_track_info_type {
    MEDIA_TRACK_INFO_TITLE,
    MEDIA_TRACK_INFO_ARTIST,
    MEDIA_TRACK_INFO_DURATION,
    MEDIA_TRACK_INFO_COUNT,
};

struct media_capabilities_event {
    struct app_event_header header;

    uint8_t capabilities; // MEDIA_CAPABILITIES_... ORed together
};

struct media_player_event {
    struct app_event_header header;

    uint64_t timestamp;     // System uptime in milliseconds when this information was received
    uint32_t elapsed_time;  // Elapsed time of the current track in seconds. Hopefully no one has a song that's 136 years long
    uint16_t playback_rate; // Media player's playback rate * 100 (e.g. 0.75 => 75, 1.5 => 150)
    uint8_t state;          // enum media_player_state_type

    // current_elapsed_time = elapsed_time + ((system_uptime - timestamp) * playback_rate / 100_000)
};

struct media_track_event {
    struct app_event_header header;

    uint8_t type;                                           // enum media_track_info_type
    union {
        uint32_t duration;                                  // Available when type is MEDIA_TRACK_INFO_DURATION
        char str[CONFIG_SMARTWATCH_AMS_MAX_STR_LEN + 1];    // Available when type is MEDIA_TRACK_INFO_TITLE or MEDIA_TRACK_INFO_ARTIST
    };
};

struct media_command_event {
    struct app_event_header header;

    uint8_t command; // enum media_command_type
};

APP_EVENT_TYPE_DECLARE(media_capabilities_event);
APP_EVENT_TYPE_DECLARE(media_player_event);
APP_EVENT_TYPE_DECLARE(media_track_event);
APP_EVENT_TYPE_DECLARE(media_command_event);
