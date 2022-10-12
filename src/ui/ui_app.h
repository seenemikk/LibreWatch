#pragma once

#include "lvgl.h"

enum ui_app_type {
    UI_APP_HOME,
    UI_APP_TIMER,
    UI_APP_COUNT,
};
BUILD_ASSERT(UI_APP_COUNT > 0, "No apps are defined");

struct ui_app_api {
    void (*init)(lv_obj_t *screen);
    void (*deinit)(void);
};

struct ui_app {
    enum ui_app_type type;
    const struct ui_app_api *api;
    lv_obj_t *screen;
};

#define UI_APP_DEFINE(_type, _api) \
    STRUCT_SECTION_ITERABLE(ui_app, _CONCAT(ui_app, _type)) = { \
        .type = _type, \
        .api = _api, \
        .screen = NULL, \
    }

#define UI_APP_OPEN(_type) \
    do { \
        struct ui_app_show_event *evt = new_ui_app_show_event(); \
        evt->type = _type; \
        APP_EVENT_SUBMIT(evt); \
    } while(0)
