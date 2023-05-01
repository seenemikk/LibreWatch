#include <zephyr/mgmt/mcumgr/smp_bt.h>

#define MODULE dfu
#include <caf/events/module_state_event.h>

#include "dfu_event.h"

#include <img_mgmt/img_mgmt.h>
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include <os_mgmt/os_mgmt.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LIBREWATCH_DFU_LOG_LEVEL);

static int upload_cb(uint32_t offset, uint32_t size, void *arg)
{
    if (size == 0 || offset > size) return 0;

    static uint8_t prev_percentage = 0;
    uint8_t percentage = ((uint64_t)offset * 100 + size / 2) / size;
    if (prev_percentage == percentage) return 0;

    struct dfu_status_event *event = new_dfu_status_event();
    if (event == NULL) return 0;

    prev_percentage = percentage;
    event->percentage = percentage;
    APP_EVENT_SUBMIT(event);

    return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
            img_mgmt_set_upload_cb(upload_cb, NULL);
            img_mgmt_register_group();
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
            os_mgmt_register_group();
#endif
        } else if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
            int err = smp_bt_register();

            if (err) {
                LOG_ERR("Service init failed (%d)", err);
            } else {
                LOG_INF("MCUboot image version: %s", CONFIG_MCUBOOT_IMAGE_VERSION);
            }
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, dfu_status_event);
