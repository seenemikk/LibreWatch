#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

void main(void)
{
    if (app_event_manager_init()) {
        LOG_ERR("Application Event Manager not initialized");
    } else {
        module_set_state(MODULE_STATE_READY);
    }
}
