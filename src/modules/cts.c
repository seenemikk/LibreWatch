#include <zephyr/kernel.h>

#include <bluetooth/services/cts_client.h>

#include <time.h>
#include <date_time.h>

#define MODULE cts
#include <caf/events/module_state_event.h>

#include "discovery_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LIBREWATCH_CTS_LOG_LEVEL);

#define INITIAL_TIMESTAMP   1640995200  // 2022/01/01 00:00:00 GMT+0000
#define RESET_MAGIC         0xfeedbabe

#if IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)

#if CONFIG_LIBREWATCH_CTS_BACKUP_INTERVAL_SECONDS < 1
#error "Invalid backup interval"
#endif // CONFIG_LIBREWATCH_CTS_BACKUP_INTERVAL_SECONDS < 1

__attribute__((section(".noinit"))) static volatile uint32_t backup_magic;
__attribute__((section(".noinit"))) static volatile time_t backup_timestamp;
static struct k_work_delayable backup_work;

#endif // IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)

// TODO add date_updated/date_update event
// TODO is there a way to hook system reset, so we wouldn't need constant backups? Maybe mcumgr os reset hook?

#if IS_ENABLED(CONFIG_BT_CTS_CLIENT)

static struct bt_cts_client cts_c;

static void update_time(struct bt_cts_current_time *current_time)
{
    struct bt_cts_exact_time_256 *time = &current_time->exact_time_256;

    struct tm tm = {
        .tm_sec = time->seconds, .tm_min = time->minutes, .tm_hour = time->hours,
        .tm_mday = time->day, .tm_mon = time->month - 1, .tm_year = time->year - 1900
    };
    int err = date_time_set(&tm);
    if (err) {
        LOG_ERR("Failed setting datetime (%d)", err);
    } else {
        LOG_INF("Time updated");
    }
}

static void cts_subscribe_cb(struct bt_cts_client *cts_c, struct bt_cts_current_time *current_time)
{
    update_time(current_time);
}

static void cts_read_cb(struct bt_cts_client *cts_c, struct bt_cts_current_time *current_time, int err)
{
    if (err) {
        LOG_ERR("Failed to read current time characteristic (%d)", err);
        return;
    }

    update_time(current_time);
}

static void discovery_completed(struct bt_gatt_dm *dm)
{
    int err = bt_cts_handles_assign(dm, &cts_c);
    if (err) {
        LOG_ERR("Could not assign CTS client handles (%d)", err);
        return;
    }

    err = bt_cts_subscribe_current_time(&cts_c, cts_subscribe_cb);
    if (err) {
        LOG_ERR("Failed subscribing to CTS service (%d)", err);
        return;
    }

    err = bt_cts_read_current_time(&cts_c, cts_read_cb);
    if (err) {
        LOG_ERR("Failed reading current time (%d)", err);
        return;
    }
}

#endif // IS_ENABLED(CONFIG_BT_CTS_CLIENT)

#if IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)

static void backup_timestamp_fn(struct k_work *work)
{
    int64_t timestamp_ms = 0;
    if (date_time_now(&timestamp_ms) == 0) {
        backup_timestamp = timestamp_ms / 1000;
    }

    k_work_reschedule(&backup_work, K_SECONDS(CONFIG_LIBREWATCH_CTS_BACKUP_INTERVAL_SECONDS));
}

#endif // IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)

static void cts_init(void)
{
    static bool initialized;

    __ASSERT_NO_MSG(!initialized);
    initialized = true;

    int err = 0;
    time_t timestamp = INITIAL_TIMESTAMP;

#if IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)
    if (backup_magic != RESET_MAGIC) {
        LOG_WRN("Unable to restore previous datetime");
        backup_magic = RESET_MAGIC;
        backup_timestamp = INITIAL_TIMESTAMP;
    }
    timestamp = backup_timestamp;

    k_work_init_delayable(&backup_work, backup_timestamp_fn);
    k_work_reschedule(&backup_work, K_SECONDS(CONFIG_LIBREWATCH_CTS_BACKUP_INTERVAL_SECONDS));
#endif // IS_ENABLED(CONFIG_LIBREWATCH_CTS_BACKUP)

    struct tm *tm = gmtime(&timestamp);
    if (tm == NULL || (err = date_time_set(tm))) {
        LOG_ERR("Failed setting initial datetime (%d)", err);
        module_set_state(MODULE_STATE_ERROR);
        return;
    }

#if IS_ENABLED(CONFIG_BT_CTS_CLIENT)
    err = bt_cts_client_init(&cts_c);
    if (err) {
        LOG_ERR("Failed initializing CTS client (%d)", err);
        module_set_state(MODULE_STATE_ERROR);
        return;
    }
#endif // IS_ENABLED(CONFIG_BT_CTS_CLIENT)

    LOG_INF("CTS module initialized");
    module_set_state(MODULE_STATE_READY);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) cts_init();

        return false;
    }

#if IS_ENABLED(CONFIG_BT_CTS_CLIENT)
    if (is_discovery_event(aeh)) {
        struct discovery_event *event = cast_discovery_event(aeh);

        if (event->type == DISCOVERY_EVENT_CTS) discovery_completed(event->gatt_dm);

        return false;
    }
#endif // IS_ENABLED(CONFIG_BT_CTS_CLIENT)

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);

#if IS_ENABLED(CONFIG_BT_CTS_CLIENT)
APP_EVENT_SUBSCRIBE(MODULE, discovery_event);
#endif // IS_ENABLED(CONFIG_BT_CTS_CLIENT)
