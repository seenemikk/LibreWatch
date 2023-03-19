#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define MODULE ui
#include <app_event_manager.h>
#include <caf/events/module_state_event.h>
#include <caf/events/power_event.h>

#include "lvgl.h"

#include "ui_popup.h"
#include "ui_app.h"
#include "ui_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SMARTWATCH_UI_LOG_LEVEL);

#define POPUP_ANIM_SPEED    200
#define APP_ANIM_SPEED      100

#define INACTIVE_DOT_DIAMETER   6
#define ACTIVE_DOT_DIAMETER     10
#define INACTIVE_DOT_COLOR      0x393839
#define ACTIVE_DOT_COLOR        0xffffff
#define DOT_ANGLE               10  // Must be even
#define DOT_DISTANCE            108

// TODO buzzer

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *touch = DEVICE_DT_GET(DT_CHOSEN(zephyr_keyboard_scan));

struct ui {
    struct {
        struct ui_popup *arr[UI_POPUP_COUNT];
        int active;
    } popups;

    struct {
        struct ui_app *arr[UI_APP_COUNT];
        int active;
        uint8_t count;
    } apps;

    struct k_work_delayable work;
    bool display_active;
    bool touch_active;
};

static struct ui ui = { .popups.active = UI_POPUP_COUNT, .apps.active = UI_APP_COUNT };

static void queue_ui_task(void)
{
    k_work_reschedule(&ui.work, K_MSEC(CONFIG_SMARTWATCH_UI_UPDATE_INTERVAL_MS));
}

static void cancel_ui_task(void)
{
    k_work_cancel_delayable(&ui.work);
}

static void init_menu_dots(struct ui_app *app)
{
    __ASSERT_NO_MSG(ui.apps.count > 0 && app && app->screen);

    uint32_t deg = 270 - (ui.apps.count - 1) * DOT_ANGLE / 2;
    for (uint8_t i = 0; i < ui.apps.count; i++) {
        uint8_t diameter = 0;
        uint32_t color = 0;
        if (i == app->type) {
            diameter = ACTIVE_DOT_DIAMETER;
            color = ACTIVE_DOT_COLOR;
        } else {
            diameter = INACTIVE_DOT_DIAMETER;
            color = INACTIVE_DOT_COLOR;
        }
        lv_coord_t x = lv_trigo_cos(deg) * DOT_DISTANCE / INT16_MAX;
        lv_coord_t y = -lv_trigo_sin(deg) * DOT_DISTANCE / INT16_MAX;

        lv_obj_t *dot = lv_obj_create(app->screen);
        lv_obj_set_size(dot, diameter, diameter);
        lv_obj_set_pos(dot, x, y);
        lv_obj_set_align(dot, LV_ALIGN_CENTER);
        lv_obj_set_style_radius(dot, 90, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);

        deg += DOT_ANGLE;
    }
}

static void ui_task(struct k_work *work)
{
    lv_task_handler();
    queue_ui_task();
}

static void send_wake_up(void)
{
    struct wake_up_event *evt = new_wake_up_event();
    APP_EVENT_SUBMIT(evt);
}

/////////////// POPUPS ///////////////

static void close_popup(enum ui_popup_type type)
{
    __ASSERT_NO_MSG(type < UI_POPUP_COUNT);
    if (type >= UI_POPUP_COUNT) return;

    struct ui_popup *popup = ui.popups.arr[type];
    if (popup == NULL || popup->screen == NULL) return;

    popup->api->deinit();

    if (type != ui.popups.active) {
        lv_obj_del(popup->screen);
        popup->screen = NULL;
        return;
    }

    enum ui_popup_type new_popup = type + 1;
    for (; new_popup < UI_POPUP_COUNT && (ui.popups.arr[new_popup] == NULL || ui.popups.arr[new_popup]->screen == NULL); new_popup++);

    lv_obj_t *new_screen = new_popup >= UI_POPUP_COUNT ? ui.apps.arr[ui.apps.active]->screen : ui.popups.arr[new_popup]->screen;
    lv_scr_load_anim(new_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, POPUP_ANIM_SPEED, 0, true);

    ui.popups.active = new_popup;
    popup->screen = NULL;
}

static void popup_event(lv_event_t *evt)
{
    lv_obj_t *target = lv_event_get_target(evt);

    if (target != ui.popups.arr[ui.popups.active]->screen || lv_indev_get_gesture_dir(lv_indev_get_act()) != LV_DIR_TOP) return;

    close_popup(ui.popups.active);
}

static void open_popup(enum ui_popup_type type)
{
    __ASSERT_NO_MSG(type < UI_POPUP_COUNT);
    if (type >= UI_POPUP_COUNT) return;

    struct ui_popup *popup = ui.popups.arr[type];
    if (popup == NULL || popup->screen != NULL) return;

    popup->screen = lv_obj_create(NULL);
    if (popup->screen == NULL) {
        LOG_ERR("Failed creating new popup");
        return;
    }
    if (!popup->persistent) lv_obj_add_event_cb(popup->screen, popup_event, LV_EVENT_GESTURE, NULL);

    popup->api->init(popup->screen);

    // New popup has lower priority
    if (popup->type >= ui.popups.active) return;

    lv_scr_load_anim(popup->screen, LV_SCR_LOAD_ANIM_OVER_BOTTOM, POPUP_ANIM_SPEED, 0, false);
    ui.popups.active = popup->type;
    send_wake_up();
}

static void init_popups(void)
{
    STRUCT_SECTION_FOREACH(ui_popup, popup) {
        if (popup->api == NULL || popup->api->init == NULL || popup->api->deinit == NULL) {
            LOG_ERR("Popup %d doesn't have necessary apis", popup->type);
            continue;
        }
        ui.popups.arr[popup->type] = popup;
    }
}

/////////////// POPUPS ///////////////

//////////////// APPS ////////////////

static void app_event(lv_event_t *evt);

static void open_app(enum ui_app_type type)
{
    __ASSERT_NO_MSG(type < UI_APP_COUNT);
    if (type >= UI_APP_COUNT) return;

    struct ui_app *app = ui.apps.arr[type];
    struct ui_app *prev_app = ui.apps.active < UI_APP_COUNT ? ui.apps.arr[ui.apps.active] : NULL;
    if (app == NULL || app->screen != NULL) return;

    app->screen = lv_obj_create(NULL);
    if (app->screen == NULL) {
        LOG_ERR("Failed creating a screen for an app");
        return;
    }
    init_menu_dots(app);
    lv_obj_add_event_cb(app->screen, app_event, LV_EVENT_GESTURE, NULL);

    app->api->init(app->screen);
    if (prev_app) prev_app->api->deinit();
    ui.apps.active = app->type;

    // Popup is active
    if (ui.popups.active != UI_POPUP_COUNT) {
        lv_obj_del(prev_app->screen);
    } else {
        lv_scr_load_anim(app->screen, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, true);
    }

    if (prev_app) prev_app->screen = NULL;
}

static void app_event(lv_event_t *evt)
{
    lv_obj_t *target = lv_event_get_target(evt);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (dir != LV_DIR_LEFT && dir != LV_DIR_RIGHT) return;
    if (target != ui.apps.arr[ui.apps.active]->screen) return;

    int new_app = ui.apps.active;
    int next_dir = dir == LV_DIR_LEFT ? 1 : -1;
    for (int i = 0; i < UI_APP_COUNT - 1; i++) {
        new_app += next_dir;
        if (new_app >= UI_APP_COUNT) {
            new_app -= UI_APP_COUNT;
        } else if (new_app < 0) {
            new_app += UI_APP_COUNT;
        }
        if (ui.apps.arr[new_app] != NULL) break;
    }

    open_app(new_app);
}

static void init_apps(void)
{
    STRUCT_SECTION_FOREACH(ui_app, app) {
        if (app->api == NULL || app->api->init == NULL || app->api->deinit == NULL) {
            LOG_ERR("App %d doesn't have necessary apis", app->type);
            continue;
        }

        ui.apps.arr[app->type] = app;
        ui.apps.count++;
    }

    enum ui_app_type type = 0;
    for (; type < UI_APP_COUNT && ui.apps.arr[type] == NULL; type++);

    if (type < UI_APP_COUNT) open_app(type);
}

//////////////// APPS ////////////////

void touch_cb(const struct device *dev, uint32_t row, uint32_t column, bool pressed)
{
    send_wake_up();
}

static void ui_init(void)
{
    if (!device_is_ready(display)) {
        LOG_ERR("Display driver is not ready");
        goto err;
    }

    if (!device_is_ready(touch)) {
        LOG_ERR("Touchpad driver is not ready");
        goto err;
    }

    init_popups();
    init_apps();
    lv_task_handler();

    k_work_init_delayable(&ui.work, ui_task);
    queue_ui_task();

    pm_device_runtime_get(display);
    ui.display_active = true;

    kscan_config(touch, touch_cb);
    pm_device_runtime_get(touch);
    ui.touch_active = true;

    module_set_state(MODULE_STATE_READY);
    return;

err:
    module_set_state(MODULE_STATE_ERROR);
}

static void handle_app_show(struct ui_app_show_event *evt)
{
    open_app(evt->type);
}

static void handle_popup_show(struct ui_popup_show_event *evt)
{
    if (evt->show) {
        open_popup(evt->type);
    } else {
        close_popup(evt->type);
    }
}

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

static void handle_power_down(struct power_down_event *evt)
{
    if (ui.display_active) {
        pm_device_runtime_put(display);
        ui.display_active = false;
    }
    if (ui.touch_active) {
        pm_device_runtime_put(touch);
        ui.touch_active = false;
    }
    cancel_ui_task();
    module_set_state(MODULE_STATE_STANDBY);
}

static void handle_wake_up(struct wake_up_event *evt)
{
    if (!ui.display_active) {
        pm_device_runtime_get(display);
        ui.display_active = true;
    }
    if (!ui.touch_active) {
        pm_device_runtime_get(touch);
        ui.touch_active = true;
    }
    queue_ui_task();
    module_set_state(MODULE_STATE_READY);
}

#endif // IS_ENABLED(CONFIG_CAF_POWER_MANAGER)

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_module_state_event(aeh)) {
        struct module_state_event *event = cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) ui_init();

        return false;
    }

    if (is_ui_app_show_event(aeh)) {
        handle_app_show(cast_ui_app_show_event(aeh));
        return false;
    }

    if (is_ui_popup_show_event(aeh)) {
        handle_popup_show(cast_ui_popup_show_event(aeh));
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
APP_EVENT_SUBSCRIBE(MODULE, ui_app_show_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_popup_show_event);

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif // IS_ENABLED(CONFIG_CAF_POWER_MANAGER)
