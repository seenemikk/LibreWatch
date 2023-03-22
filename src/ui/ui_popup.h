#pragma once

#include "ui_event.h"
#include "lvgl.h"

// Order counts. Types with lower values have higher priority.
enum ui_popup_type {
    UI_POPUP_UPDATE,
    UI_POPUP_PASSKEY,
    UI_POPUP_CALL,
    UI_POPUP_NOTIFICATION,
    UI_POPUP_COUNT,
};

struct ui_popup_api {
    void (*init)(lv_obj_t *screen);
    void (*deinit)(void);
    // Pause and resume when popup is in the background
};

struct ui_popup {
    enum ui_popup_type type;
    const struct ui_popup_api *api;
    lv_obj_t *screen;
    bool persistent;    // if true, then user can't close the popup
};

#define UI_POPUP_DEFINE(_type, _api) \
    STRUCT_SECTION_ITERABLE(ui_popup, _CONCAT(ui_popup_, _type)) = { \
        .type = _type, \
        .api = _api, \
        .screen = NULL, \
        .persistent = false, \
    }

#define UI_POPUP_PERSISTENT_DEFINE(_type, _api) \
    STRUCT_SECTION_ITERABLE(ui_popup, _CONCAT(ui_popup_, _type)) = { \
        .type = _type, \
        .api = _api, \
        .screen = NULL, \
        .persistent = true, \
    }

#define UI_POPUP_OPEN(_type) \
    do { \
        struct ui_popup_show_event *evt = new_ui_popup_show_event(); \
        if (evt != NULL) { \
            evt->show = true; \
            evt->type = _type; \
            APP_EVENT_SUBMIT(evt); \
        } \
    } while(0)

#define UI_POPUP_CLOSE(_type) \
    do { \
        struct ui_popup_show_event *evt = new_ui_popup_show_event(); \
        if (evt != NULL) { \
            evt->show = false; \
            evt->type = _type; \
            APP_EVENT_SUBMIT(evt); \
        } \
    } while(0)
