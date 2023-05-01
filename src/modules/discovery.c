#include <zephyr/kernel.h>

#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/cts_client.h>
#include <bluetooth/services/ams_client.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/gatt_dm.h>

#define MODULE discovery
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>

#include "discovery_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LIBREWATCH_DISCOVERY_LOG_LEVEL);

// TODO how to handle errors?
// TODO subscribe to GATT service changed and rediscover if changed?

static uint8_t state;
static struct bt_conn *cur_conn;

static const struct bt_uuid *type_to_uuid[DISCOVERY_EVENT_COUNT] = {
    [DISCOVERY_EVENT_GATT] = BT_UUID_GATT,
    [DISCOVERY_EVENT_CTS] = BT_UUID_CTS,
    [DISCOVERY_EVENT_AMS] = BT_UUID_AMS,
    [DISCOVERY_EVENT_ANCS] = BT_UUID_ANCS,
};

static const char *type_to_str[DISCOVERY_EVENT_COUNT] = {
    [DISCOVERY_EVENT_GATT] = "GATT",
    [DISCOVERY_EVENT_CTS] = "CTS",
    [DISCOVERY_EVENT_AMS] = "AMS",
    [DISCOVERY_EVENT_ANCS] = "ANCS",
};

static void discovery_next(void);

static void init_conn(struct bt_conn *conn)
{
    if (cur_conn != NULL) return;
    cur_conn = conn;
    bt_conn_ref(cur_conn);
}

static void deinit_conn(void)
{
    if (cur_conn == NULL) return;
    bt_conn_unref(cur_conn);
    cur_conn = NULL;
}

static void send_event(struct bt_gatt_dm *dm, uint8_t type)
{
    struct discovery_event *event = new_discovery_event();
    if (event == NULL) return;

    event->gatt_dm = dm;
    event->type = type;
    APP_EVENT_SUBMIT(event);

}

static void discovery_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
    __ASSERT_NO_MSG(state < DISCOVERY_EVENT_COUNT);

    LOG_INF("%s service found", type_to_str[state]);
    send_event(dm, state);
}

static void discovery_service_not_found_cb(struct bt_conn *conn, void *ctx)
{
    LOG_WRN("%s service not found", type_to_str[state]);
    discovery_next();
}

static void discovery_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
    LOG_ERR("Discovery error (%d)", err);
    discovery_next();
}

static const struct bt_gatt_dm_cb discovery_cb = {
    .completed = discovery_completed_cb,
    .service_not_found = discovery_service_not_found_cb,
    .error_found = discovery_error_found_cb,
};

static void discovery_next(void)
{
    if (cur_conn == NULL || (state + 1) > DISCOVERY_EVENT_COUNT) return;

    if (++state == DISCOVERY_EVENT_COUNT) {
        send_event(NULL, state);
        deinit_conn();
        return;
    }

    int err = bt_gatt_dm_start(cur_conn, type_to_uuid[state], &discovery_cb, NULL);
    if (err) {
        LOG_ERR("Failed to start discovery (%d)", err);
        discovery_next();
    }
}

static void handle_ble_peer_event(struct ble_peer_event *event)
{
    switch (event->state) {
        case PEER_STATE_SECURED: {
            state = DISCOVERY_EVENT_START;
            init_conn(event->id);
            discovery_next();
            break;
        }

        case PEER_STATE_DISCONNECTED: {
            deinit_conn();
            break;
        }

        default: {
            break;
        }
    }
}

static void handle_discovery_event(struct discovery_event *event)
{
    if (cur_conn == NULL || event->type != state || state >= DISCOVERY_EVENT_COUNT) return;

    bt_gatt_dm_data_release(event->gatt_dm);

    discovery_next();
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_ble_peer_event(aeh)) {
        handle_ble_peer_event(cast_ble_peer_event(aeh));
        return false;
    }

    if (is_discovery_event(aeh)) {
        handle_discovery_event(cast_discovery_event(aeh));
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, discovery_event);
