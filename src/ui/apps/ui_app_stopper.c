#include <zephyr/kernel.h>

#include "ui_app.h"
#include "ui_assets.h"

#define UPDATE_INTERVAL_MS  20

#define LABEL_TIME_FMT      "%02d:%02d:%02d:%03d"

static lv_obj_t *screen;
static lv_obj_t *label_time;
static lv_obj_t *button_start;
static lv_obj_t *button_stop;
static lv_obj_t *image_start;
static lv_obj_t *image_stop;

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
        lv_img_set_src(image_start, &ui_assets_pause);
        k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
    } else {
        total_time += k_uptime_get() - start_time;
        lv_img_set_src(image_start, &ui_assets_play);
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
    lv_img_set_src(image_start, &ui_assets_play);
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

    if (target == button_start) {
        start();
    } else if (target == button_stop) {
        stop();
    }
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    button_start = lv_btn_create(screen);
    lv_obj_set_size(button_start, 55, 55);
    lv_obj_set_align(button_start, LV_ALIGN_CENTER);
    lv_obj_set_pos(button_start, 40, 40);
    lv_obj_set_style_radius(button_start, 90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button_start, UI_ASSETS_COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(button_start, evt_handler, LV_EVENT_CLICKED, NULL);

    image_start = lv_img_create(button_start);
    lv_img_set_src(image_start, paused ? &ui_assets_play : &ui_assets_pause);
    lv_obj_set_align(image_start, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_start, UI_ASSETS_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_start, 0xff, LV_PART_MAIN);

    button_stop = lv_btn_create(screen);
    lv_obj_set_size(button_stop, 55, 55);
    lv_obj_set_align(button_stop, LV_ALIGN_CENTER);
    lv_obj_set_pos(button_stop, -40, 40);
    lv_obj_set_style_radius(button_stop, 90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button_stop, UI_ASSETS_COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(button_stop, evt_handler, LV_EVENT_CLICKED, NULL);

    image_stop = lv_img_create(button_stop);
    lv_img_set_src(image_stop, &ui_assets_stop);
    lv_obj_set_align(image_stop, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_stop, UI_ASSETS_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_stop, 0xff, LV_PART_MAIN);

    label_time = lv_label_create(screen);
    lv_obj_set_size(label_time, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(label_time, LV_ALIGN_CENTER);
    lv_obj_set_pos(label_time, 0, -15);
    lv_obj_set_style_text_font(label_time, &ui_assets_chivo_mono_24, LV_PART_MAIN);

    update_time_label();

    if (start_time && !paused) k_work_reschedule(&work, K_MSEC(UPDATE_INTERVAL_MS));
}

static void deinit(void)
{
    screen = NULL;
    label_time = NULL;
    button_start = NULL;
    button_stop = NULL;
    image_start = NULL;
    image_stop = NULL;
    k_work_cancel_delayable(&work);
}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

UI_APP_DEFINE(UI_APP_STOPPER, &api);
