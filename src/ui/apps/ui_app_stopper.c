#include <zephyr/kernel.h>

#include "ui_app.h"

#define UPDATE_INTERVAL_MS  20

#define LABEL_TIME_FMT      "%02d:%02d:%02d:%03d"

static lv_obj_t *screen;
static lv_obj_t *label_start;
static lv_obj_t *label_stop;
static lv_obj_t *label_time;
static lv_obj_t *btn_start;
static lv_obj_t *btn_stop;

static int64_t total_time;
static int64_t start_time;
static bool paused = true;

static void update();
static K_WORK_DELAYABLE_DEFINE(work, update);

static void update_time_label(void)
{
    int64_t ms = (!paused ? (k_uptime_get() - start_time) : 0) + total_time;
    uint32_t hours = ms / (1000 * 60 * 60);
    ms -= hours * 1000 * 60 * 60;
    uint32_t minutes = ms / (1000 * 60);
    ms -= minutes * 1000 * 60;
    uint32_t seconds = ms / 1000;
    ms -= seconds * 1000;

    lv_label_set_text_fmt(label_time, LABEL_TIME_FMT,
        hours,
        minutes,
        seconds,
        (uint32_t)ms
    );
}

static void start(void)
{
    if (paused) {
        start_time = k_uptime_get();
        k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
    } else {
        total_time += k_uptime_get() - start_time;
        k_work_cancel_delayable(&work);
    }
    paused = !paused;
    update_time_label();
}

static void stop(void)
{
    paused = true;
    start_time = 0;
    total_time = 0;
    update_time_label();
    k_work_cancel_delayable(&work);
}

static void update()
{
    if (screen == NULL) return;

    update_time_label();

    k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
}

static void evt_handler(lv_event_t *evt)
{
    lv_event_code_t event_code = lv_event_get_code(evt);
    lv_obj_t *target = lv_event_get_target(evt);

    if (event_code != LV_EVENT_CLICKED) return;

    if (target == btn_start) {
        start();
    } else if (target == btn_stop) {
        stop();
    }
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    btn_start = lv_btn_create(screen);
    lv_obj_set_width(btn_start, 55);
    lv_obj_set_height(btn_start, 55);
    lv_obj_set_x(btn_start, 40);
    lv_obj_set_y(btn_start, -40);
    lv_obj_set_align(btn_start, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_style_radius(btn_start, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_start, evt_handler, LV_EVENT_CLICKED, NULL);

    label_start = lv_label_create(btn_start);
    lv_obj_set_width(label_start, LV_SIZE_CONTENT);
    lv_obj_set_height(label_start, LV_SIZE_CONTENT);
    lv_obj_set_align(label_start, LV_ALIGN_CENTER);
    lv_label_set_text(label_start, "START");

    btn_stop = lv_btn_create(screen);
    lv_obj_set_width(btn_stop, 55);
    lv_obj_set_height(btn_stop, 55);
    lv_obj_set_x(btn_stop, -40);
    lv_obj_set_y(btn_stop, -40);
    lv_obj_set_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_style_radius(btn_stop, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_stop, evt_handler, LV_EVENT_CLICKED, NULL);

    label_stop = lv_label_create(btn_stop);
    lv_obj_set_width(label_stop, LV_SIZE_CONTENT);
    lv_obj_set_height(label_stop, LV_SIZE_CONTENT);
    lv_obj_set_align(label_stop, LV_ALIGN_CENTER);
    lv_label_set_text(label_stop, "STOP");

    label_time = lv_label_create(screen);
    lv_obj_set_width(label_time, LV_SIZE_CONTENT);
    lv_obj_set_height(label_time, LV_SIZE_CONTENT);
    lv_obj_set_y(label_time, -37);
    lv_obj_set_align(label_time, LV_ALIGN_CENTER);

    update_time_label();

    if (start_time && !paused) k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
}

static void deinit(void)
{
    screen = NULL;
    label_time = NULL;
    label_start = NULL;
    label_stop = NULL;
    btn_start = NULL;
    btn_stop = NULL;
    k_work_cancel_delayable(&work);
}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

UI_APP_DEFINE(UI_APP_STOPPER, &api);
