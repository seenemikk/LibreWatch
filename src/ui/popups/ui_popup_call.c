#include <zephyr/kernel.h>

#include "ui_popup.h"
#include "ui_assets.h"

#define MODULE ui_popup_call
#include <caf/events/ble_common_event.h>

#include "notification_event.h"

static struct notification_event cur_notif;

static lv_obj_t *screen;
static lv_obj_t *label_category;
static lv_obj_t *label_name;
static lv_obj_t *button_positive;
static lv_obj_t *button_negative;

static void button_clicked_cb(lv_event_t *evt)
{
    lv_event_code_t event_code = lv_event_get_code(evt);
    lv_obj_t *target = lv_event_get_target(evt);

    if (event_code != LV_EVENT_CLICKED) return;

    struct notification_action_event *event = new_notification_action_event();
    if (event == NULL) return;

    event->uid = cur_notif.info.notif_uid;
    if (target == button_positive) {
        event->action = NOTIFICATION_ACTION_POSITIVE;
    } else if (target == button_negative) {
        event->action = NOTIFICATION_ACTION_NEGATIVE;
    } else {
        app_event_manager_free(event);
        return;
    }

    APP_EVENT_SUBMIT(event);
}

static void update(void)
{
    if (screen == NULL) return;

    uint8_t category = cur_notif.info.category_id;
    if (category != NOTIFICATION_CATEGORY_ACTIVE_CALL && category != NOTIFICATION_CATEGORY_INCOMING_CALL) return;

    lv_label_set_text(label_category, cur_notif.message);
    lv_label_set_text(label_name, cur_notif.title);

    lv_obj_add_flag(button_positive, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(button_negative, LV_OBJ_FLAG_HIDDEN);

    if (cur_notif.info.evt_flags.positive_action && cur_notif.info.evt_flags.negative_action) {
        lv_obj_set_x(button_positive, -45);
        lv_obj_set_y(button_positive, 55);
        lv_obj_clear_flag(button_positive, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_x(button_negative, 45);
        lv_obj_set_y(button_negative, 55);
        lv_obj_clear_flag(button_negative, LV_OBJ_FLAG_HIDDEN);

        return;
    }

    lv_obj_t *btn = cur_notif.info.evt_flags.positive_action ? button_positive : button_negative;
    lv_obj_set_x(btn, 0);
    lv_obj_set_y(btn, 55);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    label_category = lv_label_create(screen);
    lv_obj_set_y(label_category, -45);
    lv_obj_set_align(label_category, LV_ALIGN_CENTER);

    label_name = lv_label_create(screen);
    lv_obj_set_align(label_name, LV_ALIGN_CENTER);

    button_positive = lv_btn_create(screen);
    lv_obj_set_width(button_positive, 55);
    lv_obj_set_height(button_positive, 55);
    lv_obj_set_align(button_positive, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(button_positive, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button_positive, UI_ASSETS_COLOR_SUCCESS, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(button_positive, button_clicked_cb, LV_EVENT_CLICKED, NULL);

    button_negative = lv_btn_create(screen);
    lv_obj_set_width(button_negative, 55);
    lv_obj_set_height(button_negative, 55);
    lv_obj_set_align(button_negative, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(button_negative, 90, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button_negative, UI_ASSETS_COLOR_ERROR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(button_negative, button_clicked_cb, LV_EVENT_CLICKED, NULL);

    update();
}

static void deinit(void)
{
    screen = NULL;
    label_category = NULL;
    label_name = NULL;
    button_positive = NULL;
    button_negative = NULL;
    memset(&cur_notif, 0, sizeof(cur_notif));
}

static const struct ui_popup_api api = {
    .init = init,
    .deinit = deinit,
};

UI_POPUP_DEFINE(UI_POPUP_CALL, &api);

static void handle_notification_event(struct notification_event *event)
{
    uint8_t cur_category = cur_notif.info.category_id;
    uint8_t evt_category = event->info.category_id;

    if ((uint8_t)event->info.evt_id == NOTIFICATION_REMOVED) {
        if (event->info.notif_uid == cur_notif.info.notif_uid) {
            UI_POPUP_CLOSE(UI_POPUP_CALL);
        }
        return;
    }

    if (evt_category == NOTIFICATION_CATEGORY_INCOMING_CALL) {
        if (cur_category != NOTIFICATION_CATEGORY_INCOMING_CALL && cur_category != NOTIFICATION_CATEGORY_ACTIVE_CALL) {
            cur_notif = *event;
            UI_POPUP_OPEN(UI_POPUP_CALL);
            update();
        }
        return;
    }

    if (evt_category == NOTIFICATION_CATEGORY_ACTIVE_CALL) {
        if (cur_category != NOTIFICATION_CATEGORY_ACTIVE_CALL) {
            cur_notif = *event;
            UI_POPUP_OPEN(UI_POPUP_CALL);
            update();
        }
        return;
    }
}

static void handle_ble_peer_event(struct ble_peer_event *evt)
{
    if (evt->state != PEER_STATE_DISCONNECTED) return;
    UI_POPUP_CLOSE(UI_POPUP_CALL);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_notification_event(aeh)) {
        handle_notification_event(cast_notification_event(aeh));
        return false;
    }

    if (is_ble_peer_event(aeh)) {
        handle_ble_peer_event(cast_ble_peer_event(aeh));
        return false;
    }

     /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, notification_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event_event);
