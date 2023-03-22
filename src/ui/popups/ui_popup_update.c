#include <zephyr/kernel.h>

#include "ui_popup.h"
#include "ui_assets.h"

#define MODULE ui_popup_update
#include <caf/events/ble_common_event.h>
#include <caf/events/power_event.h>

#include "dfu_event.h"

#define TIMEOUT_MS  30000

static void timeout_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(timeout, timeout_cb);

static uint8_t progress;

static lv_obj_t *screen;
static lv_obj_t *label_status;
static lv_obj_t *bar_progress;

static void send_wake_up(void)
{
    struct wake_up_event *evt = new_wake_up_event();
    if (evt != NULL) APP_EVENT_SUBMIT(evt);
}

static void timeout_cb(struct k_work *work)
{
    UI_POPUP_CLOSE(UI_POPUP_UPDATE);
}

static void update(void)
{
    if (screen == NULL) return;

    lv_bar_set_value(bar_progress, progress, LV_ANIM_OFF);
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    label_status = lv_label_create(screen);
    lv_obj_set_pos(label_status, 0, -10);
    lv_obj_set_align(label_status, LV_ALIGN_CENTER);
    lv_label_set_text(label_status, "Device updating");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_18, LV_PART_MAIN);

    bar_progress = lv_bar_create(screen);
    lv_obj_set_size(bar_progress, 150, 15);
    lv_obj_set_align(bar_progress, LV_ALIGN_CENTER);
    lv_obj_set_pos(bar_progress, 0, 20);
    lv_obj_set_style_bg_color(bar_progress, UI_ASSETS_COLOR_GRAY, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, UI_ASSETS_COLOR_PRIMARY, LV_PART_INDICATOR);

    update();
}

static void deinit(void)
{
    screen = NULL;
    label_status = NULL;
    bar_progress = NULL;
}

static const struct ui_popup_api api = {
    .init = init,
    .deinit = deinit,
};

UI_POPUP_PERSISTENT_DEFINE(UI_POPUP_UPDATE, &api);

static void handle_dfu_status_event(struct dfu_status_event *event)
{
    progress = event->percentage;
    if (screen == NULL) UI_POPUP_OPEN(UI_POPUP_UPDATE);
    k_work_reschedule(&timeout, K_MSEC(TIMEOUT_MS));
    send_wake_up();
    update();
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_dfu_status_event(aeh)) {
        handle_dfu_status_event(cast_dfu_status_event(aeh));
        return false;
    }

     /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, dfu_status_event);
