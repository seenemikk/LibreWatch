#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/sensor.h>

#define MODULE sensors
#include <caf/events/module_state_event.h>
#include <caf/events/power_event.h>

#include "sensors_event.h"

#include "icm42605.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_SENSORS_LOG_LEVEL);

static const struct device *imu = DEVICE_DT_GET(DT_NODELABEL(icm42605));
static uint32_t step_count;
static bool fast_fetch = true;

static void sensors_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(work, sensors_work_handler);

static void imu_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
    switch ((int16_t)trigger->chan)
    {
        case SENSOR_CHAN_ICM42605_WAKE:
            APP_EVENT_SUBMIT(new_wake_up_event());
            break;

        case SENSOR_CHAN_ICM42605_SLEEP:
            APP_EVENT_SUBMIT(new_power_down_event());
            break;

        default:
            break;
    }
}

static void sensors_work_reschedule(void)
{
    k_timeout_t delay = K_MSEC(fast_fetch ? CONFIG_SMARTWATCH_SENSORS_FETCH_INTERVAL_FAST : CONFIG_SMARTWATCH_SENSORS_FETCH_INTERVAL_SLOW);
    k_work_reschedule(&work, delay);
}

static int sensors_update_steps(void)
{
    int ret = sensor_sample_fetch(imu);
    if (ret) {
        LOG_ERR("Failed to fetch IMU data! (%d)", ret);
        return ret;
    }

    struct sensor_value sensor_value = {0};
    ret = sensor_channel_get(imu, SENSOR_CHAN_ICM42605_STEPS, &sensor_value);
    if (ret) {
        LOG_ERR("Failed to get IMU data! (%d)", ret);
        return ret;
    }

    static uint16_t prev_value;
    uint16_t prev_step_count = step_count;
    uint16_t value = sensor_value.val1;
    step_count += value - prev_value;
    prev_value = value;

    if (prev_step_count != step_count) {
        struct sensors_event *evt = new_sensors_event();
        evt->type = SENSORS_EVENT_STEP_COUNT;
        evt->value = step_count;
        APP_EVENT_SUBMIT(evt);
    }

    return ret;
}

static void sensors_work_handler(struct k_work *work)
{
    if (sensors_update_steps()) {
        module_set_state(MODULE_STATE_ERROR);
        return;
    }

    sensors_work_reschedule();
}

static void sensors_init(void)
{
    static bool initialized;

    __ASSERT_NO_MSG(!initialized);
    initialized = true;

    if (!device_is_ready(imu)) {
        LOG_ERR("IMU is not ready");
        goto error;
    }

    struct sensor_trigger trig = { .type = SENSOR_TRIG_ICM42605_R2W, SENSOR_CHAN_ALL };
    if (sensor_trigger_set(imu, &trig, imu_trigger_handler)) {
        LOG_ERR("Failed to set IMU trigger handler");
        goto error;
    }

    sensors_work_reschedule();

    LOG_INF("Sensors module initialized");
    module_set_state(MODULE_STATE_READY);
    return;

error:
    module_set_state(MODULE_STATE_ERROR);
}

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

static void handle_power_down(struct power_down_event *evt)
{
    fast_fetch = false;
}

static void handle_wake_up(struct wake_up_event *evt)
{
    sensors_update_steps();
    sensors_work_reschedule();
    fast_fetch = true;
}

#endif // IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) sensors_init();

        return false;
    }

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

    if (is_power_down_event(aeh)) {
        handle_power_down(cast_power_down_event(aeh));
        return false;
    }

    if (is_wake_up_event(aeh)) {
        handle_wake_up(cast_wake_up_event(aeh));
        return false;
    }

#endif // IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif // IS_ENABLED(CONFIG_CAF_POWER_MANAGER)
