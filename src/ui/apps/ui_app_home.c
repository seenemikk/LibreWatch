#include <zephyr/kernel.h>

#define MODULE ui_app_home
#include "sensors_event.h"

#include "date_time.h"

#include "ui_app.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define UPDATE_INTERVAL_MS  900

static lv_obj_t *screen;
static lv_obj_t *label_time;
static lv_obj_t *label_date;
static lv_obj_t *label_step_count;

static uint32_t step_count;

static void update();
static K_WORK_DELAYABLE_DEFINE(work, update);

static void init(lv_obj_t *scr)
{
    screen = scr;

    label_time = lv_label_create(screen);
    lv_obj_set_align(label_time, LV_ALIGN_CENTER);

    label_date = lv_label_create(screen);
    lv_obj_set_align(label_date, LV_ALIGN_CENTER);
    lv_obj_set_pos(label_date, 0, 30);

    label_step_count = lv_label_create(screen);
    lv_obj_set_align(label_step_count, LV_ALIGN_CENTER);
    lv_obj_set_pos(label_step_count, 0, -30);

    update();
}

static void deinit(void)
{
    screen = NULL;
    label_time = NULL;
    label_date = NULL;
    label_step_count = NULL;
    k_work_cancel_delayable(&work);
}

static void update()
{
    if (screen == NULL) return;

    int64_t msec;
    if (date_time_now(&msec) != 0) return;

    time_t sec = msec / 1000;
    struct tm *tm = gmtime(&sec);
    if (tm == NULL) return;

    lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    lv_label_set_text_fmt(label_date, "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    lv_label_set_text_fmt(label_step_count, "Steps: %d", step_count);

    k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
}

static void handle_sensors_event(struct sensors_event *evt)
{
    switch ((int)evt->type) {
        case SENSORS_EVENT_STEP_COUNT: {
            step_count = evt->value;
            break;
        }

        default: {
            LOG_DBG("Unhandled sensors event %d", evt->type);
            break;
        }
    }
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_sensors_event(aeh)) {
        handle_sensors_event(cast_sensors_event(aeh));
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

UI_APP_DEFINE(UI_APP_HOME, &api);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, sensors_event);
