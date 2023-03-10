#include <ctype.h>
#include <stdlib.h>

#include <zephyr/kernel.h>

#include <bluetooth/services/ams_client.h>

#define MODULE ams
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>

#include "media_event.h"
#include "discovery_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_AMS_LOG_LEVEL);

enum ams_flag_type {
    AMS_FLAG_SUBSCRIBED_TO_TRACK,
    AMS_FLAG_SUBSCRIBED_TO_PLAYER,
};

static struct bt_ams_client ams_c;
static atomic_t ams_flags;

static const enum bt_ams_player_attribute_id player_attributes[] = {
    BT_AMS_PLAYER_ATTRIBUTE_ID_PLAYBACK_INFO,
};

static const enum bt_ams_track_attribute_id track_attributes[] = {
    BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST,
    BT_AMS_TRACK_ATTRIBUTE_ID_TITLE,
    BT_AMS_TRACK_ATTRIBUTE_ID_DURATION
};

static void ams_write_cb(struct bt_ams_client *ams_c, uint8_t err);

static void init_entity_update(void)
{
    struct bt_ams_entity_attribute_list attr_list = {0};
    int err = 0;

    if (!atomic_test_and_set_bit(&ams_flags, AMS_FLAG_SUBSCRIBED_TO_PLAYER)) {
        attr_list.entity = BT_AMS_ENTITY_ID_PLAYER;
        attr_list.attribute_count = ARRAY_SIZE(player_attributes);
        attr_list.attribute.player = player_attributes;
    } else if (!atomic_test_and_set_bit(&ams_flags, AMS_FLAG_SUBSCRIBED_TO_TRACK)) {
        attr_list.entity = BT_AMS_ENTITY_ID_TRACK;
        attr_list.attribute_count = ARRAY_SIZE(track_attributes);
        attr_list.attribute.track = track_attributes;
    } else {
        return;
    }

    err = bt_ams_write_entity_update(&ams_c, &attr_list, ams_write_cb);
    if (err) {
        LOG_ERR("Failed to write to entity update (%d)", err);
        return;
    }
}

static void handle_player_info(const struct bt_ams_entity_update_notif *notif)
{
    struct media_player_event *event = new_media_player_event();
    event->elapsed_time = 0;
    event->playback_rate = 0;
    event->state = MEDIA_PLAYER_STATE_PAUSED;
    event->timestamp = k_uptime_get();

    // TODO what should we do when we get weird data? Should we even send an event or send that the player is paused?
    if (notif->len <= 3) goto end; // "1,, or 0,, no idea what that means"

    char buf[100] = {0};
    snprintk(buf, MIN(sizeof(buf), notif->len + 1), "%s", notif->data);

    // State
    uint8_t state =  notif->data[0] - (uint8_t)'0';
    if (state >= MEDIA_PLAYER_STATE_COUNT) goto end;

    // Playback rate
    uint16_t playback_rate = 0;
    uint8_t *cur = (uint8_t *)strchr(buf, ',');
    if (cur == NULL) goto end;
    cur++;

    if (!isdigit(*cur)) goto end;
    playback_rate += (*cur - (uint8_t)'0') * 100U;
    cur++;

    if (*cur++ != (uint8_t)'.') goto end;

    if (!isdigit(*cur)) goto end;
    playback_rate += (*cur - (uint8_t)'0') * 10U;
    cur++;

    if (*cur != (uint8_t)',') {
        if (!isdigit(*cur)) goto end;
        playback_rate += (*cur - (uint8_t)'0');
    }

    // Elapsed time
    uint32_t elapsed_time = 0;
    cur = (uint8_t *)strchr((char *)cur, ',');
    if (cur == NULL) goto end;
    cur++;

    uint8_t *end = NULL;
    elapsed_time = strtoull((const char *)cur, (char **)&end, 10);
    if (end == cur) goto end;

    event->elapsed_time = elapsed_time;
    event->playback_rate = playback_rate;
    event->state = state;

end:
    APP_EVENT_SUBMIT(event);
}

static void handle_track_info(const struct bt_ams_entity_update_notif *notif)
{
    // TODO handle truncation

    struct media_track_event *event = new_media_track_event();
    snprintk(event->str, MIN(sizeof(event->str), notif->len + 1), "%s", notif->data);

    switch (notif->ent_attr.attribute.track) {
        case BT_AMS_TRACK_ATTRIBUTE_ID_DURATION: {
            event->type = MEDIA_TRACK_INFO_DURATION;

            char *end = NULL;
            uint32_t duration = strtoull((const char *)event->str, (char **)&end, 10);
            if (end == event->str) goto err;
            event->duration = duration;

            break;
        }

        case BT_AMS_TRACK_ATTRIBUTE_ID_TITLE: {
            event->type = MEDIA_TRACK_INFO_TITLE;
            break;
        }

        case BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST: {
            event->type = MEDIA_TRACK_INFO_ARTIST;
            break;
        }

        default: {
            goto err;
        }
    }

    APP_EVENT_SUBMIT(event);
    return;

err:
    app_event_manager_free(event);
}

static void ams_eu_cb(struct bt_ams_client *ams_c, const struct bt_ams_entity_update_notif *notif, int err)
{
    if (err) {
        LOG_ERR("AMS entity update error (%d)", err);
        return;
    }

    switch (notif->ent_attr.entity) {
        case BT_AMS_ENTITY_ID_PLAYER: {
            handle_player_info(notif);
            break;
        }

        case BT_AMS_ENTITY_ID_TRACK: {
            handle_track_info(notif);
            break;
        }

        default: {
            break;
        }
    }
}

static void ams_rc_cb(struct bt_ams_client *ams_c, const uint8_t *data, size_t len)
{
    struct media_capabilities_event *event = new_media_capabilities_event();
    event->capabilities = 0;

    for (size_t i = 0; i < len; i++) {
        if (data[i] >= MEDIA_COMMAND_COUNT) continue;
        event->capabilities |= BIT(data[i]);
    }

    APP_EVENT_SUBMIT(event);
}

static void ams_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
    if (err) {
        // TODO how to handle entity update error?
        LOG_ERR("AMS write error (%d)", err);
        return;
    }

    init_entity_update();
}

static void discovery_completed(struct bt_gatt_dm *dm)
{
    bt_ams_client_init(&ams_c);
    atomic_set(&ams_flags, 0);

    int err = bt_ams_handles_assign(dm, &ams_c);
    if (err) {
        LOG_ERR("Could not assign AMS client handles (%d)", err);
        return;
    }

    err = bt_ams_subscribe_remote_command(&ams_c, ams_rc_cb);
    if (err) {
        LOG_ERR("Failed to subscribe to remote command (%d)", err);
        return;
    }

    err = bt_ams_subscribe_entity_update(&ams_c, ams_eu_cb);
    if (err) {
        LOG_ERR("Failed to subscribe to entity update (%d)", err);
        return;
    }

    init_entity_update();
}

static void init(void)
{
    static bool initialized;

    __ASSERT_NO_MSG(!initialized);
    initialized = true;

    module_set_state(MODULE_STATE_READY);
}

static void handle_media_command_event(struct media_command_event *evt)
{
    if (evt->command >= MEDIA_COMMAND_COUNT) return;

    int err = bt_ams_write_remote_command(&ams_c, evt->command, NULL);
    if (err) {
        LOG_ERR("Failed writing remote command (%d)", err);
    }
}

static void handle_ble_peer_event(struct ble_peer_event *evt)
{
    if (evt->state != PEER_STATE_DISCONNECTED) return;

    struct media_player_event *event = new_media_player_event();
    event->elapsed_time = 0;
    event->playback_rate = 0;
    event->timestamp = k_uptime_get();
    event->state = MEDIA_PLAYER_STATE_DISCONNECTED;
    APP_EVENT_SUBMIT(event);

    bt_ams_client_init(&ams_c);
}

static void handle_discovery_event(struct discovery_event *event)
{
    if (event->type != DISCOVERY_EVENT_AMS) return;
    discovery_completed(event->gatt_dm);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);
        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) init();
        return false;
    }

    if (is_ble_peer_event(aeh)) {
        handle_ble_peer_event(cast_ble_peer_event(aeh));
        return false;
    }

    if (is_discovery_event(aeh)) {
        handle_discovery_event(cast_discovery_event(aeh));
        return false;
    }

    if (is_media_command_event(aeh)) {
        handle_media_command_event(cast_media_command_event(aeh));
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event_event);
APP_EVENT_SUBSCRIBE(MODULE, discovery_event);
APP_EVENT_SUBSCRIBE(MODULE, media_command_event);
