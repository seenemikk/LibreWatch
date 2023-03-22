#include <zephyr/kernel.h>

#include "ui_app.h"
#include "ui_assets.h"

#define MODULE ui_app_media
#include "media_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define REDRAW_INTERVAL_MS  1000

static lv_obj_t *screen;
static lv_obj_t *arc_elapsed_time;
static lv_obj_t *label_artist;
static lv_obj_t *label_title;
static lv_obj_t *button_next;
static lv_obj_t *button_previous;
static lv_obj_t *button_play_pause;
static lv_obj_t *image_next;
static lv_obj_t *image_previous;
static lv_obj_t *image_play_pause;

static char artist[CONFIG_SMARTWATCH_AMS_MAX_STR_LEN + 1];
static char title[CONFIG_SMARTWATCH_AMS_MAX_STR_LEN + 1];

static uint16_t playback_rate;
static uint32_t elapsed_time;
static uint32_t total_time;
static uint64_t elapsed_time_timestamp;

static bool playing;

static void redraw();
static K_WORK_DELAYABLE_DEFINE(redraw_work, redraw);

static uint8_t elapsed_time_percentage(void)
{
    if (total_time == 0) return 0;

    uint64_t current_elapsed_time = elapsed_time + ((k_uptime_get() - elapsed_time_timestamp) * playback_rate / 100000);
    return current_elapsed_time * 100 / total_time;
}

static void redraw()
{
    if (screen == NULL) return;

    lv_img_set_src(image_play_pause, playing ? &ui_assets_pause : &ui_assets_play);
    lv_label_set_text(label_title, title);
    lv_label_set_text(label_artist, artist);
    lv_arc_set_value(arc_elapsed_time, elapsed_time_percentage());

    if (playing) k_work_reschedule(&redraw_work, K_MSEC(REDRAW_INTERVAL_MS));
}

static void button_clicked_cb(lv_event_t *evt)
{
    lv_event_code_t event_code = lv_event_get_code(evt);
    lv_obj_t *target = lv_event_get_target(evt);

    if (event_code != LV_EVENT_CLICKED) return;

    struct media_command_event *event = new_media_command_event();

    if (target == button_play_pause) {
        event->command = playing ? MEDIA_COMMAND_PAUSE : MEDIA_COMMAND_PLAY;
    } else if (target == button_previous) {
        event->command = MEDIA_COMMAND_PREVIOUS_TRACK;
    } else if (target == button_next) {
        event->command = MEDIA_COMMAND_NEXT_TRACK;
    } else {
        app_event_manager_free(event);
        return;
    }

    APP_EVENT_SUBMIT(event);
}

static void init(lv_obj_t *scr)
{
    screen = scr;

    arc_elapsed_time = lv_arc_create(screen);
    lv_obj_set_size(arc_elapsed_time, 240, 240);
    lv_obj_set_align(arc_elapsed_time, LV_ALIGN_CENTER);
    lv_obj_clear_flag(arc_elapsed_time, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arc_elapsed_time, 0, LV_PART_KNOB);
    lv_arc_set_bg_angles(arc_elapsed_time, 90, 89);
    lv_obj_set_style_arc_width(arc_elapsed_time, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_elapsed_time, UI_ASSETS_COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_elapsed_time, 3, LV_PART_INDICATOR);

    label_artist = lv_label_create(screen);
    lv_obj_set_size(label_artist, 160, LV_SIZE_CONTENT);
    lv_obj_set_pos(label_artist, 0, -45);
    lv_obj_set_align(label_artist, LV_ALIGN_CENTER);
    lv_label_set_long_mode(label_artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(label_artist, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_artist, &lv_font_montserrat_14, LV_PART_MAIN);

    label_title = lv_label_create(screen);
    lv_obj_set_size(label_title, 190, LV_SIZE_CONTENT);
    lv_obj_set_pos(label_title, 0, -15);
    lv_obj_set_align(label_title, LV_ALIGN_CENTER);
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_18, LV_PART_MAIN);

    button_previous = lv_btn_create(screen);
    lv_obj_set_size(button_previous, 55, 55);
    lv_obj_set_pos(button_previous, -70, 40);
    lv_obj_set_align(button_previous, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(button_previous, 90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button_previous, UI_ASSETS_COLOR_BLACK, LV_PART_MAIN);
    lv_obj_add_event_cb(button_previous, button_clicked_cb, LV_EVENT_CLICKED, NULL);

    image_previous = lv_img_create(button_previous);
    lv_img_set_src(image_previous, &ui_assets_previous);
    lv_obj_set_align(image_previous, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_previous, UI_ASSETS_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_previous, 0xff, LV_PART_MAIN);

    button_next = lv_btn_create(screen);
    lv_obj_set_size(button_next, 55, 55);
    lv_obj_set_pos(button_next, 70, 40);
    lv_obj_set_align(button_next, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(button_next, 90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button_next, UI_ASSETS_COLOR_BLACK, LV_PART_MAIN);
    lv_obj_add_event_cb(button_next, button_clicked_cb, LV_EVENT_CLICKED, NULL);

    image_next = lv_img_create(button_next);
    lv_img_set_src(image_next, &ui_assets_next);
    lv_obj_set_align(image_next, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_next, UI_ASSETS_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_next, 0xff, LV_PART_MAIN);

    button_play_pause = lv_btn_create(screen);
    lv_obj_set_size(button_play_pause, 55, 55);
    lv_obj_set_pos(button_play_pause, 0, 40);
    lv_obj_set_align(button_play_pause, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(button_play_pause, 90, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button_play_pause, UI_ASSETS_COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(button_play_pause, button_clicked_cb, LV_EVENT_CLICKED, NULL);

    image_play_pause = lv_img_create(button_play_pause);
    lv_obj_set_align(image_play_pause, LV_ALIGN_CENTER);
    lv_obj_set_style_img_recolor(image_play_pause, UI_ASSETS_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(image_play_pause, 0xff, LV_PART_MAIN);

    redraw();
}

static void deinit(void)
{
    screen = NULL;
    arc_elapsed_time = NULL;
    label_artist = NULL;
    label_title = NULL;
    button_next = NULL;
    button_previous = NULL;
    button_play_pause = NULL;
    image_next = NULL;
    image_previous = NULL;
    image_play_pause = NULL;
    k_work_cancel_delayable(&redraw_work);
}

static const struct ui_app_api api = {
    .init = init,
    .deinit = deinit,
};

static void handle_media_track_event(struct media_track_event *event)
{
    switch (event->type) {
        case MEDIA_TRACK_INFO_TITLE: {
            snprintf(title, sizeof(title), "%s", event->str);
            break;
        }

        case MEDIA_TRACK_INFO_ARTIST: {
            snprintf(artist, sizeof(artist), "%s", event->str);
            break;
        }

        case MEDIA_TRACK_INFO_DURATION: {
            total_time = event->duration;
            break;
        }

        default: {
            return;
        }
    }

    redraw();
}

static void handle_media_player_event(struct media_player_event *event)
{
    if (event->state == MEDIA_PLAYER_STATE_DISCONNECTED) {
        playing = false;
        playback_rate = 0;
        elapsed_time = 0;
        total_time = 0;
        artist[0] = 0;
        title[0] = 0;
    } else {
        playing = event->state == MEDIA_PLAYER_STATE_PLAYING;
        elapsed_time = event->elapsed_time;
        playback_rate = event->playback_rate;
        elapsed_time_timestamp = event->timestamp;
    }

    redraw();
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_media_track_event(aeh)) {
        handle_media_track_event(cast_media_track_event(aeh));
        return false;
    }

     if (is_media_player_event(aeh)) {
        handle_media_player_event(cast_media_player_event(aeh));
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

UI_APP_DEFINE(UI_APP_MEDIA, &api);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, media_track_event);
APP_EVENT_SUBSCRIBE(MODULE, media_player_event);
