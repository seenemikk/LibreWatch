#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#include "passkey_event.h"

void main(void)
{
    if (app_event_manager_init()) {
        LOG_ERR("Application Event Manager not initialized");
    } else {
        module_set_state(MODULE_STATE_READY);
    }
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_passkey_event(aeh)) {
        struct passkey_event *event = cast_passkey_event(aeh);

        if (event->show) {
            LOG_WRN("Passkey %06d", event->passkey);
        } else {
            LOG_WRN("Success: %d", event->success);
        }

        return false;
    }
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, passkey_event);
