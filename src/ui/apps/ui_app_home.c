#include <zephyr/kernel.h>

#include "date_time.h"

#include "ui_app.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ui_app_home);

#define UPDATE_INTERVAL_MS  900

static lv_obj_t *screen;
static lv_obj_t *label_time;
static lv_obj_t *label_date;

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

    update();
}

static void deinit(void)
{
    screen = NULL;
    label_time = NULL;
    label_date = NULL;
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

    k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

UI_APP_DEFINE(UI_APP_HOME, &api);
