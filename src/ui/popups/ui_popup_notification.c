#include <zephyr/kernel.h>

#include "ui_popup.h"
#include "ui_assets.h"

#define MODULE ui_popup_notification
#include <caf/events/ble_common_event.h>
#include "notification_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_UI_LOG_LEVEL);

#define WIDGET_WIDTH    150

struct notification {
    struct notification_event data;
    bool valid;
    uint8_t id;
};

static struct notification notifications[CONFIG_SMARTWATCH_ANCS_MAX_NOTIFICATIONS];
static sys_slist_t notifications_list = SYS_SLIST_STATIC_INIT(&notifications);

static lv_obj_t *screen;
static lv_obj_t *tabview;
static lv_obj_t *label_tab_number;

static struct notification *node_to_notification(sys_snode_t *node)
{
    if (node == NULL) return NULL;
    struct app_event_header *header = CONTAINER_OF(node, struct app_event_header, node);
    struct notification_event *data = CONTAINER_OF(header, struct notification_event, header);
    return CONTAINER_OF(data, struct notification, data);
}

static struct notification *get_free_notification_slot(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(notifications); i++) {
        if (!notifications[i].valid) return &notifications[i];
    }
    return NULL;
}

static struct notification *get_notification(uint8_t id)
{
    for (size_t i = 0; i < ARRAY_SIZE(notifications); i++) {
        if (notifications[i].valid && notifications[i].id == id) return &notifications[i];
    }
    return NULL;
}

static void free_notification(struct notification *notif)
{
    __ASSERT_NO_MSG(notif);
    if (notif == NULL) return;

    sys_slist_find_and_remove(&notifications_list, &notif->data.header.node);
    notif->valid = false;

    struct notification_action_event *event = new_notification_action_event();
    if (event != NULL) {
        event->uid = notif->data.info.notif_uid;
        event->action = NOTIFICATION_ACTION_NEGATIVE;
        APP_EVENT_SUBMIT(event);
    }
}

static void free_notifications(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(notifications); i++) {
        free_notification(&notifications[i]);
    }
}

static void update_tab_number(void)
{
    if (tabview == NULL) return;

    uint32_t count = lv_obj_get_child_cnt(lv_tabview_get_content(tabview));
    uint32_t active = lv_tabview_get_tab_act(tabview);

    lv_label_set_text_fmt(label_tab_number, "%d/%d", active + 1, count);
}

static void draw_notification(struct notification *notif)
{
    __ASSERT_NO_MSG(notif);
    if (notif == NULL || screen == NULL || !notif->valid) return;

    lv_obj_t *tab = lv_tabview_add_tab(tabview, "");

    lv_obj_t *label_title = lv_label_create(tab);
    lv_label_set_text(label_title, notif->data.title);
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_DOT);
    lv_obj_set_align(label_title, LV_ALIGN_CENTER);
    lv_obj_set_size(label_title, WIDGET_WIDTH, 20);
    lv_obj_set_pos(label_title, 0, -60);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xde7918), LV_PART_MAIN);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_18, LV_PART_MAIN);

    lv_obj_t *label_message = lv_label_create(tab);
    lv_label_set_text(label_message, notif->data.message);
    lv_label_set_long_mode(label_message, LV_LABEL_LONG_DOT);
    lv_obj_set_align(label_message, LV_ALIGN_CENTER);
    lv_obj_set_size(label_message, WIDGET_WIDTH, 100);
    lv_obj_set_pos(label_message, 0, 10);
    lv_obj_set_style_text_align(label_message, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_message, &lv_font_montserrat_14, LV_PART_MAIN);

    notif->id = lv_obj_get_child_cnt(lv_tabview_get_content(tabview)) - 1;

    update_tab_number();
}

static void draw_notifications(void)
{
    struct notification *first_notif = NULL;
    sys_snode_t *node = sys_slist_peek_head(&notifications_list);

    while (node != NULL) {
        struct notification *notif = node_to_notification(node);
        if (first_notif == NULL) first_notif = notif;

        draw_notification(notif);

        node = sys_slist_peek_next_no_check(node);
    }

    // If we drew at least one notification, then remove the first one
    if (lv_obj_get_child_cnt(lv_tabview_get_content(tabview)) > 0) free_notification(first_notif);
}

static void tab_changed_cb(lv_event_t *evt)
{
    struct notification *notif = get_notification(lv_tabview_get_tab_act(tabview));
    if (notif != NULL) free_notification(notif);
    update_tab_number();
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    tabview = lv_tabview_create(screen, LV_DIR_TOP, 0);
    lv_obj_add_event_cb(tabview, tab_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    label_tab_number = lv_label_create(screen);
    lv_label_set_text(label_tab_number, "");
    lv_obj_set_align(label_tab_number, LV_ALIGN_CENTER);
    lv_obj_set_pos(label_tab_number, 0, 100);
    lv_obj_set_style_text_font(label_tab_number, &ui_assets_chivo_mono_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_tab_number, lv_color_hex(0xde7918), LV_PART_MAIN);

    draw_notifications();
}

static void deinit(void)
{
    screen = NULL;
    tabview = NULL;
    label_tab_number = NULL;
}

static const struct ui_popup_api api = {
    .init = init,
    .deinit = deinit,
};

UI_POPUP_DEFINE(UI_POPUP_NOTIFICATION, &api);

static void handle_notification_event(struct notification_event *event)
{
    if ((uint8_t)event->info.evt_id != NOTIFICATION_ADDED) return;
    if ((uint8_t)event->info.category_id == NOTIFICATION_CATEGORY_INCOMING_CALL ||
        (uint8_t)event->info.category_id == NOTIFICATION_CATEGORY_ACTIVE_CALL) return;

    struct notification *notif = get_free_notification_slot();
    if (notif == NULL) return;

    notif->data = *event;
    notif->valid = true;
    sys_slist_append(&notifications_list, &notif->data.header.node);
    draw_notification(notif);
    UI_POPUP_OPEN(UI_POPUP_NOTIFICATION);
}

static void handle_ble_peer_event(struct ble_peer_event *evt)
{
    if (evt->state != PEER_STATE_DISCONNECTED) return;
    free_notifications();
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
