#include "ui_popup.h"

static void init(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xEEE6BD), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *hello_world_label = lv_label_create(screen);
    lv_label_set_text(hello_world_label, "Notification!");
    lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);
}

static void deinit(void)
{

}

static const struct ui_popup_api api = {
    .init = init,
    .deinit = deinit,
};

UI_POPUP_DEFINE(UI_POPUP_NOTIFICATION, &api);
