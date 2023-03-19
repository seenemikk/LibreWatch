#include <zephyr/kernel.h>

#define MODULE ui_app_home
#include "sensors_event.h"

#include "date_time.h"

#include "ui_app.h"
#include "ui_assets.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define UPDATE_INTERVAL_MS  900
#define WIDGET_WIDTH        160

static lv_obj_t *screen;
static lv_obj_t *label_time;
static lv_obj_t *label_date;
static lv_obj_t *panel_steps;
static lv_obj_t *image_steps;
static lv_obj_t *label_steps;

static uint32_t step_count;

static void update();
static K_WORK_DELAYABLE_DEFINE(work, update);

static const char *day_str[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char *month_str[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

static const char *day_to_str(uint8_t day)
{
    return day < ARRAY_SIZE(day_str) ? day_str[day] : NULL;
}

static const char *month_to_str(uint8_t month)
{
    return month < ARRAY_SIZE(month_str) ? month_str[month] : NULL;
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    label_time = lv_label_create(screen);
    lv_obj_set_align(label_time, LV_ALIGN_CENTER);
    lv_obj_set_width(label_time, WIDGET_WIDTH);
    lv_obj_set_pos(label_time, 0, -10);
    lv_obj_set_align(label_time, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(label_time, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_time, &ui_assets_chivo_mono_48, LV_PART_MAIN);

    label_date = lv_label_create(screen);
    lv_obj_set_width(label_date, WIDGET_WIDTH);
    lv_obj_set_pos(label_date, 0, 20);
    lv_obj_set_align(label_date, LV_ALIGN_CENTER);
    lv_obj_set_style_text_letter_space(label_date, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_date, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    panel_steps = lv_obj_create(screen);
    lv_obj_set_size(panel_steps, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(panel_steps, 0, 40);
    lv_obj_set_align(panel_steps, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(panel_steps, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(panel_steps, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(panel_steps, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(panel_steps, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel_steps, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(panel_steps, 5, LV_PART_MAIN);

    image_steps = lv_img_create(panel_steps);
    lv_img_set_src(image_steps, &ui_assets_footprint);
    lv_obj_set_align(image_steps, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_steps, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_steps, 0xff, LV_PART_MAIN);

    label_steps = lv_label_create(panel_steps);
    lv_label_set_long_mode(label_steps, LV_LABEL_LONG_CLIP);

    update();
}

static void deinit(void)
{
    screen = NULL;
    label_time = NULL;
    label_date = NULL;
    panel_steps = NULL;
    image_steps = NULL;
    label_steps = NULL;
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

    lv_label_set_text_fmt(label_time, "%02d:%02d", tm->tm_hour, tm->tm_min);
    lv_label_set_text_fmt(label_date, "%s, %s %d", day_to_str(tm->tm_wday), month_to_str(tm->tm_mon), tm->tm_mday);
    lv_label_set_text_fmt(label_steps, "%d", step_count);

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
