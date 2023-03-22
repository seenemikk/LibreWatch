#include <zephyr/kernel.h>

#define MODULE passkey
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>

#include "passkey_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_PASSKEY_LOG_LEVEL);

static atomic_t passkey_active = ATOMIC_INIT(false);

static void passkey_show(unsigned int passkey)
{
    if (atomic_get(&passkey_active)) return;
    atomic_set(&passkey_active, true);

    struct passkey_event *event = new_passkey_event();
    if (event != NULL) {
        event->passkey = passkey;
        event->show = true;
        APP_EVENT_SUBMIT(event);
    }
}

static void passkey_hide(bool success)
{
    if (!atomic_get(&passkey_active)) return;
    atomic_set(&passkey_active, false);

    struct passkey_event *event = new_passkey_event();
    if (event != NULL) {
        event->success = success;
        event->show = false;
        APP_EVENT_SUBMIT(event);
    }
}

static void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    passkey_show(passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    passkey_hide(false);
}

static struct bt_conn_auth_cb conn_auth_cb = {
    .passkey_display = passkey_display,
    .cancel = auth_cancel,
};

void pairing_complete(struct bt_conn *conn, bool bonded)
{
    passkey_hide(true);
}

void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    passkey_hide(false);
}

static struct bt_conn_auth_info_cb conn_auth_info_cb = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed,
};

static void passkey_init(void)
{
    static bool initialized;

    __ASSERT_NO_MSG(!initialized);
    initialized = true;

    int err = bt_conn_auth_cb_register(&conn_auth_cb);
    if (err) {
        LOG_ERR("Failed registering auth callback (%d)", err);
        goto error;
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_cb);
    if (err) {
        LOG_ERR("Failed registering auth info callback (%d)", err);
        goto error;
    }

    LOG_INF("BLE passkey initialized");
    module_set_state(MODULE_STATE_READY);
    return;

error:
    module_set_state(MODULE_STATE_ERROR);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) passkey_init();

        return false;
    }

    if (is_ble_peer_event(aeh)) {
        struct ble_peer_event *event = cast_ble_peer_event(aeh);

        if (event->state == PEER_STATE_DISCONNECTED) passkey_hide(false);

        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
