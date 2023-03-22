#include <zephyr/kernel.h>

#include "ui_popup.h"
#include "ui_assets.h"

#define MODULE ui_popup_passkey
#include "passkey_event.h"

#define POPUP_CLOSE_DELAY_S     10

static lv_obj_t *label;
static lv_obj_t *screen;
static struct passkey_event state;

static void close_popup(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(work, close_popup);

static void update(void)
{
    if (screen == NULL) {
        UI_POPUP_OPEN(UI_POPUP_PASSKEY);
        return;
    }

    if (state.show) {
        lv_label_set_text_fmt(label, "%06d", state.passkey);
        lv_obj_set_style_text_font(label, &ui_assets_chivo_mono_48, LV_PART_MAIN);
        if (k_work_delayable_is_pending(&work)) k_work_cancel_delayable(&work);
    } else {
        lv_label_set_text(label, state.success ? "Pairing Successful" : "Pairing Failed");
        lv_obj_set_style_bg_color(screen, state.success ? UI_ASSETS_COLOR_SUCCESS : UI_ASSETS_COLOR_ERROR, LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN);
        if (!k_work_delayable_is_pending(&work)) k_work_reschedule(&work, K_SECONDS(POPUP_CLOSE_DELAY_S));
    }
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    label = lv_label_create(screen);
    lv_obj_set_align(label, LV_ALIGN_CENTER);

    update();
}

static void deinit(void)
{
    screen = NULL;
    label = NULL;
    k_work_cancel_delayable(&work);
}

static const struct ui_popup_api api = {
    .init = init,
    .deinit = deinit,
};

UI_POPUP_DEFINE(UI_POPUP_PASSKEY, &api);

static void close_popup(struct k_work *work)
{
    UI_POPUP_CLOSE(UI_POPUP_PASSKEY);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_passkey_event(aeh)) {
        struct passkey_event *event = cast_passkey_event(aeh);

        state = *event;
        update();

        return false;
    }

     /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, passkey_event);
