#include "ui_app.h"

static void init(lv_obj_t *screen)
{
    lv_obj_t *hello_world_label = lv_label_create(screen);
    lv_label_set_text(hello_world_label, "Timer!");
    lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);
}

static void deinit(void)
{

}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

UI_APP_DEFINE(UI_APP_TIMER, &api);
