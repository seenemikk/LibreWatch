#include <zephyr/kernel.h>
#include <bluetooth/conn.h>

#define MODULE bt_conn_params
#include <caf/events/ble_common_event.h>

#include "discovery_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_BT_CONN_PARAMS_LOG_LEVEL);

static struct bt_conn *conn;

static void update_conn_params(void)
{
    if (conn == NULL) return;

    struct bt_le_conn_param param = {
        .interval_min = CONFIG_SMARTWATCH_BT_CONN_PARAMS_INT_MIN,
        .interval_max = CONFIG_SMARTWATCH_BT_CONN_PARAMS_INT_MAX,
        .latency = CONFIG_SMARTWATCH_BT_CONN_PARAMS_LATENCY,
        .timeout = CONFIG_SMARTWATCH_BT_CONN_PARAMS_TIMEOUT,
    };
    int err = bt_conn_le_param_update(conn, &param);
    if (err) {
        LOG_ERR("Failed to update connection parameters");
    }
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_ble_peer_event(aeh)) {
        struct ble_peer_event *event = cast_ble_peer_event(aeh);
        if (event->state == PEER_STATE_CONNECTED) {
            conn = event->id;
#if !IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)
            update_conn_params();
#endif // IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)
        } else if (event->state == PEER_STATE_DISCONNECTED) {
            conn = NULL;
        }
        return false;
    }

#if IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)
    if (is_discovery_event(aeh)) {
        struct discovery_event *event = cast_discovery_event(aeh);
        if (event->type == DISCOVERY_EVENT_COUNT) update_conn_params();
        return false;
    }
#endif // IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);

#if IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)
APP_EVENT_SUBSCRIBE(MODULE, discovery_event);
#endif // IS_ENABLED(CONFIG_SMARTWATCH_DISCOVERY)
